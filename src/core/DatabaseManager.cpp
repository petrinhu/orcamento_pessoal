#include "core/DatabaseManager.h"
#include "core/CryptoHelper.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>

static const QStringList CATEGORIAS_PADRAO = {
    "Aluguel/Moradia", "Internet", "Luz/Água/Gás", "Transporte",
    "Alimentação", "Educação", "Saúde", "Streaming/TV/Telefone",
    "Academia", "Outros", "Crédito"
};

static QString sanitizarNome(const QString &nome)
{
    QString s = nome.trimmed().toLower();
    s.replace(' ', '_');
    s.remove(QRegularExpression("[^a-z0-9_]"));
    return s.isEmpty() ? "usuario" : s.left(50);
}

// ── Singleton ─────────────────────────────────────────────────────────────────

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

// ── Conexão ───────────────────────────────────────────────────────────────────

bool DatabaseManager::conectar(const QString &nome, const QString &senha)
{
    m_senha = senha;

    const QString appDir  = QCoreApplication::applicationDirPath();
    const QString dataDir = appDir + "/data";
    QDir().mkpath(dataDir);

    const QString slug = sanitizarNome(nome);
    m_arquivoEnc = dataDir + "/" + slug + ".enc";
    m_arquivoTmp = dataDir + "/." + slug + ".db";

    // Usuário existente: decripta o arquivo
    if (QFile::exists(m_arquivoEnc)) {
        if (!decriptarParaTemp()) {
            qDebug() << "DatabaseManager: senha incorreta ou arquivo corrompido";
            return false;
        }
    }
    // Novo usuário: arquivo temporário vazio será criado pelo SQLite

    m_db = QSqlDatabase::addDatabase("QSQLITE", "main");
    m_db.setDatabaseName(m_arquivoTmp);
    if (!m_db.open()) {
        qDebug() << "DatabaseManager: erro ao abrir SQLite:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery q(m_db);
    q.exec("PRAGMA foreign_keys = ON");
    q.exec("PRAGMA journal_mode = MEMORY");  // sem arquivos de journal em disco
    q.exec("PRAGMA synchronous = NORMAL");

    if (!criarEsquema()) return false;

    // Primeiro acesso: gera o .enc inicial
    if (!QFile::exists(m_arquivoEnc))
        salvarEEncriptar();

    // Garante limpeza ao fechar o app
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     [this]() { desconectar(); });

    return true;
}

void DatabaseManager::desconectar()
{
    if (!m_db.isOpen()) return;
    m_db.close();
    QSqlDatabase::removeDatabase("main");
    salvarEEncriptar();
    QFile::remove(m_arquivoTmp);
}

bool DatabaseManager::isConectado() const
{
    return m_db.isOpen();
}

// ── Cripto ────────────────────────────────────────────────────────────────────

bool DatabaseManager::decriptarParaTemp()
{
    QFile enc(m_arquivoEnc);
    if (!enc.open(QIODevice::ReadOnly)) return false;
    const QByteArray dados = enc.readAll();
    enc.close();

    if (dados.size() <= CryptoHelper::SALT_LENGTH) return false;

    const QByteArray salt       = dados.left(CryptoHelper::SALT_LENGTH);
    const QByteArray ciphertext = dados.mid(CryptoHelper::SALT_LENGTH);

    QByteArray chave, iv;
    if (!CryptoHelper::derivarChaveEIV(salt, m_senha, chave, iv)) return false;

    const QByteArray plaintext = CryptoHelper::decrypt(ciphertext, chave, iv);
    if (plaintext.isEmpty()) return false;

    QFile tmp(m_arquivoTmp);
    if (!tmp.open(QIODevice::WriteOnly)) return false;
    tmp.write(plaintext);
    tmp.close();
    return true;
}

bool DatabaseManager::salvarEEncriptar()
{
    QFile tmp(m_arquivoTmp);
    if (!tmp.open(QIODevice::ReadOnly)) return false;
    const QByteArray dbData = tmp.readAll();
    tmp.close();
    if (dbData.isEmpty()) return false;

    const QByteArray salt = CryptoHelper::gerarSalt();
    QByteArray chave, iv;
    if (!CryptoHelper::derivarChaveEIV(salt, m_senha, chave, iv)) return false;

    const QByteArray encrypted = CryptoHelper::encrypt(dbData, chave, iv);
    if (encrypted.isEmpty()) return false;

    // Escrita atômica: .new → rename
    const QString encNew = m_arquivoEnc + ".new";
    QFile enc(encNew);
    if (!enc.open(QIODevice::WriteOnly)) return false;
    enc.write(salt);
    enc.write(encrypted);
    enc.close();

    QFile::remove(m_arquivoEnc);
    return QFile::rename(encNew, m_arquivoEnc);
}

// ── Esquema ───────────────────────────────────────────────────────────────────

bool DatabaseManager::criarEsquema()
{
    QSqlQuery q(m_db);

    bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS categorias ("
        "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  nome TEXT NOT NULL UNIQUE"
        ")"
    );
    if (!ok) { qDebug() << "criarEsquema categorias:" << q.lastError().text(); return false; }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS entradas ("
        "  id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  origem         TEXT    NOT NULL DEFAULT '',"
        "  valor_centavos INTEGER NOT NULL DEFAULT 0,"
        "  data           TEXT    NOT NULL"
        ")"
    );
    if (!ok) { qDebug() << "criarEsquema entradas:" << q.lastError().text(); return false; }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS gastos_fixos ("
        "  id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  historico      TEXT    NOT NULL DEFAULT '',"
        "  valor_centavos INTEGER NOT NULL DEFAULT 0,"
        "  data           TEXT    NOT NULL,"
        "  categoria_id   INTEGER NOT NULL,"
        "  FOREIGN KEY (categoria_id) REFERENCES categorias(id) ON DELETE CASCADE"
        ")"
    );
    if (!ok) { qDebug() << "criarEsquema gastos_fixos:" << q.lastError().text(); return false; }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS gastos_variaveis ("
        "  id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  historico      TEXT    NOT NULL DEFAULT '',"
        "  valor_centavos INTEGER NOT NULL DEFAULT 0,"
        "  data           TEXT    NOT NULL,"
        "  categoria_id   INTEGER NOT NULL,"
        "  FOREIGN KEY (categoria_id) REFERENCES categorias(id) ON DELETE CASCADE"
        ")"
    );
    if (!ok) { qDebug() << "criarEsquema gastos_variaveis:" << q.lastError().text(); return false; }

    // Semeia categorias padrão apenas se a tabela estiver vazia
    q.exec("SELECT COUNT(*) FROM categorias");
    if (q.next() && q.value(0).toInt() == 0) {
        for (const QString &nome : CATEGORIAS_PADRAO) {
            Categoria cat;
            cat.nome = nome;
            inserirCategoria(cat);
        }
    }

    return true;
}

// ── Categorias ────────────────────────────────────────────────────────────────

QList<Categoria> DatabaseManager::listarCategorias()
{
    QList<Categoria> lista;
    QSqlQuery q(m_db);
    q.exec("SELECT id, nome FROM categorias ORDER BY nome");
    while (q.next())
        lista.append({q.value(0).toInt(), q.value(1).toString()});
    return lista;
}

bool DatabaseManager::inserirCategoria(Categoria &cat)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO categorias (nome) VALUES (:nome)");
    q.bindValue(":nome", cat.nome);
    if (!q.exec()) { qDebug() << "inserirCategoria:" << q.lastError().text(); return false; }
    cat.id = q.lastInsertId().toInt();
    salvarEEncriptar();
    return true;
}

bool DatabaseManager::removerCategoria(int id)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM categorias WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) { qDebug() << "removerCategoria:" << q.lastError().text(); return false; }
    salvarEEncriptar();
    return true;
}

// ── Entradas ──────────────────────────────────────────────────────────────────

QList<Entrada> DatabaseManager::listarEntradas()
{
    QList<Entrada> lista;
    QSqlQuery q(m_db);
    q.exec("SELECT id, origem, valor_centavos, data FROM entradas ORDER BY data DESC");
    while (q.next()) {
        Entrada e;
        e.id            = q.value(0).toInt();
        e.origem        = q.value(1).toString();
        e.valorCentavos = q.value(2).toLongLong();
        e.data          = QDate::fromString(q.value(3).toString(), "yyyy-MM-dd");
        lista.append(e);
    }
    return lista;
}

bool DatabaseManager::inserirEntrada(Entrada &entrada)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO entradas (origem, valor_centavos, data) VALUES (:o, :v, :d)");
    q.bindValue(":o", entrada.origem);
    q.bindValue(":v", entrada.valorCentavos);
    q.bindValue(":d", entrada.data.toString("yyyy-MM-dd"));
    if (!q.exec()) { qDebug() << "inserirEntrada:" << q.lastError().text(); return false; }
    entrada.id = q.lastInsertId().toInt();
    salvarEEncriptar();
    return true;
}

bool DatabaseManager::atualizarEntrada(const Entrada &entrada)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE entradas SET origem=:o, valor_centavos=:v, data=:d WHERE id=:id");
    q.bindValue(":o",  entrada.origem);
    q.bindValue(":v",  entrada.valorCentavos);
    q.bindValue(":d",  entrada.data.toString("yyyy-MM-dd"));
    q.bindValue(":id", entrada.id);
    if (!q.exec()) { qDebug() << "atualizarEntrada:" << q.lastError().text(); return false; }
    salvarEEncriptar();
    return true;
}

bool DatabaseManager::removerEntrada(int id)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM entradas WHERE id=:id");
    q.bindValue(":id", id);
    if (!q.exec()) { qDebug() << "removerEntrada:" << q.lastError().text(); return false; }
    salvarEEncriptar();
    return true;
}

qint64 DatabaseManager::totalEntradas()
{
    QSqlQuery q(m_db);
    q.exec("SELECT COALESCE(SUM(valor_centavos),0) FROM entradas");
    return q.next() ? q.value(0).toLongLong() : 0;
}

// ── Gastos Fixos ──────────────────────────────────────────────────────────────

QList<GastoFixo> DatabaseManager::listarGastosFixos()
{
    QList<GastoFixo> lista;
    QSqlQuery q(m_db);
    q.exec(
        "SELECT gf.id, gf.historico, gf.valor_centavos, gf.data,"
        "       gf.categoria_id, c.nome"
        " FROM gastos_fixos gf"
        " JOIN categorias c ON gf.categoria_id = c.id"
        " ORDER BY gf.data DESC"
    );
    while (q.next()) {
        GastoFixo g;
        g.id            = q.value(0).toInt();
        g.historico     = q.value(1).toString();
        g.valorCentavos = q.value(2).toLongLong();
        g.data          = QDate::fromString(q.value(3).toString(), "yyyy-MM-dd");
        g.categoriaId   = q.value(4).toInt();
        g.categoriaNome = q.value(5).toString();
        lista.append(g);
    }
    return lista;
}

bool DatabaseManager::inserirGastoFixo(GastoFixo &gasto)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO gastos_fixos (historico,valor_centavos,data,categoria_id)"
              " VALUES (:h,:v,:d,:c)");
    q.bindValue(":h", gasto.historico);
    q.bindValue(":v", gasto.valorCentavos);
    q.bindValue(":d", gasto.data.toString("yyyy-MM-dd"));
    q.bindValue(":c", gasto.categoriaId);
    if (!q.exec()) { qDebug() << "inserirGastoFixo:" << q.lastError().text(); return false; }
    gasto.id = q.lastInsertId().toInt();
    salvarEEncriptar();
    return true;
}

bool DatabaseManager::atualizarGastoFixo(const GastoFixo &gasto)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE gastos_fixos SET historico=:h,valor_centavos=:v,data=:d,categoria_id=:c"
              " WHERE id=:id");
    q.bindValue(":h",  gasto.historico);
    q.bindValue(":v",  gasto.valorCentavos);
    q.bindValue(":d",  gasto.data.toString("yyyy-MM-dd"));
    q.bindValue(":c",  gasto.categoriaId);
    q.bindValue(":id", gasto.id);
    if (!q.exec()) { qDebug() << "atualizarGastoFixo:" << q.lastError().text(); return false; }
    salvarEEncriptar();
    return true;
}

bool DatabaseManager::removerGastoFixo(int id)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM gastos_fixos WHERE id=:id");
    q.bindValue(":id", id);
    if (!q.exec()) { qDebug() << "removerGastoFixo:" << q.lastError().text(); return false; }
    salvarEEncriptar();
    return true;
}

qint64 DatabaseManager::totalGastosFixos()
{
    QSqlQuery q(m_db);
    q.exec("SELECT COALESCE(SUM(valor_centavos),0) FROM gastos_fixos");
    return q.next() ? q.value(0).toLongLong() : 0;
}

// ── Gastos Variáveis ──────────────────────────────────────────────────────────

QList<GastoVariavel> DatabaseManager::listarGastosVariaveis()
{
    QList<GastoVariavel> lista;
    QSqlQuery q(m_db);
    q.exec(
        "SELECT gv.id, gv.historico, gv.valor_centavos, gv.data,"
        "       gv.categoria_id, c.nome"
        " FROM gastos_variaveis gv"
        " JOIN categorias c ON gv.categoria_id = c.id"
        " ORDER BY gv.data DESC"
    );
    while (q.next()) {
        GastoVariavel g;
        g.id            = q.value(0).toInt();
        g.historico     = q.value(1).toString();
        g.valorCentavos = q.value(2).toLongLong();
        g.data          = QDate::fromString(q.value(3).toString(), "yyyy-MM-dd");
        g.categoriaId   = q.value(4).toInt();
        g.categoriaNome = q.value(5).toString();
        lista.append(g);
    }
    return lista;
}

bool DatabaseManager::inserirGastoVariavel(GastoVariavel &gasto)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO gastos_variaveis (historico,valor_centavos,data,categoria_id)"
              " VALUES (:h,:v,:d,:c)");
    q.bindValue(":h", gasto.historico);
    q.bindValue(":v", gasto.valorCentavos);
    q.bindValue(":d", gasto.data.toString("yyyy-MM-dd"));
    q.bindValue(":c", gasto.categoriaId);
    if (!q.exec()) { qDebug() << "inserirGastoVariavel:" << q.lastError().text(); return false; }
    gasto.id = q.lastInsertId().toInt();
    salvarEEncriptar();
    return true;
}

bool DatabaseManager::atualizarGastoVariavel(const GastoVariavel &gasto)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE gastos_variaveis SET historico=:h,valor_centavos=:v,data=:d,categoria_id=:c"
              " WHERE id=:id");
    q.bindValue(":h",  gasto.historico);
    q.bindValue(":v",  gasto.valorCentavos);
    q.bindValue(":d",  gasto.data.toString("yyyy-MM-dd"));
    q.bindValue(":c",  gasto.categoriaId);
    q.bindValue(":id", gasto.id);
    if (!q.exec()) { qDebug() << "atualizarGastoVariavel:" << q.lastError().text(); return false; }
    salvarEEncriptar();
    return true;
}

bool DatabaseManager::removerGastoVariavel(int id)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM gastos_variaveis WHERE id=:id");
    q.bindValue(":id", id);
    if (!q.exec()) { qDebug() << "removerGastoVariavel:" << q.lastError().text(); return false; }
    salvarEEncriptar();
    return true;
}

qint64 DatabaseManager::totalGastosVariaveis()
{
    QSqlQuery q(m_db);
    q.exec("SELECT COALESCE(SUM(valor_centavos),0) FROM gastos_variaveis");
    return q.next() ? q.value(0).toLongLong() : 0;
}
