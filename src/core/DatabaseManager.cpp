#include "core/DatabaseManager.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

// Categorias padrão semeadas na primeira execução
static const QStringList CATEGORIAS_PADRAO = {
    "Aluguel/Moradia", "Internet", "Luz/Água/Gás", "Transporte",
    "Alimentação", "Educação", "Saúde", "Streaming/TV/Telefone",
    "Academia", "Outros", "Crédito"
};

// ── Singleton ─────────────────────────────────────────────────────────────────

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

// ── Conexão ───────────────────────────────────────────────────────────────────

bool DatabaseManager::conectar(const QString &host, int porta, const QString &banco,
                               const QString &usuario, const QString &senha)
{
    m_db = QSqlDatabase::addDatabase("QMYSQL");
    m_db.setHostName(host);
    m_db.setPort(porta);
    m_db.setDatabaseName(banco);
    m_db.setUserName(usuario);
    m_db.setPassword(senha);

    if (!m_db.open()) {
        qDebug() << "DatabaseManager: falha na conexão:" << m_db.lastError().text();
        return false;
    }
    return criarEsquema();
}

void DatabaseManager::desconectar()
{
    if (m_db.isOpen())
        m_db.close();
}

bool DatabaseManager::isConectado() const
{
    return m_db.isOpen();
}

// ── Esquema ───────────────────────────────────────────────────────────────────

bool DatabaseManager::criarEsquema()
{
    QSqlQuery q(m_db);

    bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS categorias ("
        "  id   INT AUTO_INCREMENT PRIMARY KEY,"
        "  nome VARCHAR(100) NOT NULL UNIQUE"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
    );
    if (!ok) { qDebug() << "criarEsquema categorias:" << q.lastError().text(); return false; }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS entradas ("
        "  id             INT AUTO_INCREMENT PRIMARY KEY,"
        "  origem         VARCHAR(200) NOT NULL DEFAULT '',"
        "  valor_centavos BIGINT       NOT NULL DEFAULT 0,"
        "  data           DATE         NOT NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
    );
    if (!ok) { qDebug() << "criarEsquema entradas:" << q.lastError().text(); return false; }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS gastos_fixos ("
        "  id             INT AUTO_INCREMENT PRIMARY KEY,"
        "  historico      VARCHAR(200) NOT NULL DEFAULT '',"
        "  valor_centavos BIGINT       NOT NULL DEFAULT 0,"
        "  data           DATE         NOT NULL,"
        "  categoria_id   INT          NOT NULL,"
        "  FOREIGN KEY (categoria_id) REFERENCES categorias(id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
    );
    if (!ok) { qDebug() << "criarEsquema gastos_fixos:" << q.lastError().text(); return false; }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS gastos_variaveis ("
        "  id             INT AUTO_INCREMENT PRIMARY KEY,"
        "  historico      VARCHAR(200) NOT NULL DEFAULT '',"
        "  valor_centavos BIGINT       NOT NULL DEFAULT 0,"
        "  data           DATE         NOT NULL,"
        "  categoria_id   INT          NOT NULL,"
        "  FOREIGN KEY (categoria_id) REFERENCES categorias(id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
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
    while (q.next()) {
        lista.append({q.value(0).toInt(), q.value(1).toString()});
    }
    return lista;
}

bool DatabaseManager::inserirCategoria(Categoria &cat)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO categorias (nome) VALUES (:nome)");
    q.bindValue(":nome", cat.nome);
    if (!q.exec()) {
        qDebug() << "inserirCategoria:" << q.lastError().text();
        return false;
    }
    cat.id = q.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::removerCategoria(int id)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM categorias WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        qDebug() << "removerCategoria:" << q.lastError().text();
        return false;
    }
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
        e.data          = q.value(3).toDate();
        lista.append(e);
    }
    return lista;
}

bool DatabaseManager::inserirEntrada(Entrada &entrada)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO entradas (origem, valor_centavos, data)"
        " VALUES (:origem, :valor, :data)"
    );
    q.bindValue(":origem", entrada.origem);
    q.bindValue(":valor",  entrada.valorCentavos);
    q.bindValue(":data",   entrada.data.toString("yyyy-MM-dd"));
    if (!q.exec()) {
        qDebug() << "inserirEntrada:" << q.lastError().text();
        return false;
    }
    entrada.id = q.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::atualizarEntrada(const Entrada &entrada)
{
    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE entradas SET origem = :origem, valor_centavos = :valor, data = :data"
        " WHERE id = :id"
    );
    q.bindValue(":origem", entrada.origem);
    q.bindValue(":valor",  entrada.valorCentavos);
    q.bindValue(":data",   entrada.data.toString("yyyy-MM-dd"));
    q.bindValue(":id",     entrada.id);
    if (!q.exec()) {
        qDebug() << "atualizarEntrada:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removerEntrada(int id)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM entradas WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        qDebug() << "removerEntrada:" << q.lastError().text();
        return false;
    }
    return true;
}

qint64 DatabaseManager::totalEntradas()
{
    QSqlQuery q(m_db);
    q.exec("SELECT COALESCE(SUM(valor_centavos), 0) FROM entradas");
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
        g.data          = q.value(3).toDate();
        g.categoriaId   = q.value(4).toInt();
        g.categoriaNome = q.value(5).toString();
        lista.append(g);
    }
    return lista;
}

bool DatabaseManager::inserirGastoFixo(GastoFixo &gasto)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO gastos_fixos (historico, valor_centavos, data, categoria_id)"
        " VALUES (:hist, :valor, :data, :catId)"
    );
    q.bindValue(":hist",  gasto.historico);
    q.bindValue(":valor", gasto.valorCentavos);
    q.bindValue(":data",  gasto.data.toString("yyyy-MM-dd"));
    q.bindValue(":catId", gasto.categoriaId);
    if (!q.exec()) {
        qDebug() << "inserirGastoFixo:" << q.lastError().text();
        return false;
    }
    gasto.id = q.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::atualizarGastoFixo(const GastoFixo &gasto)
{
    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE gastos_fixos"
        " SET historico = :hist, valor_centavos = :valor, data = :data, categoria_id = :catId"
        " WHERE id = :id"
    );
    q.bindValue(":hist",  gasto.historico);
    q.bindValue(":valor", gasto.valorCentavos);
    q.bindValue(":data",  gasto.data.toString("yyyy-MM-dd"));
    q.bindValue(":catId", gasto.categoriaId);
    q.bindValue(":id",    gasto.id);
    if (!q.exec()) {
        qDebug() << "atualizarGastoFixo:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removerGastoFixo(int id)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM gastos_fixos WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        qDebug() << "removerGastoFixo:" << q.lastError().text();
        return false;
    }
    return true;
}

qint64 DatabaseManager::totalGastosFixos()
{
    QSqlQuery q(m_db);
    q.exec("SELECT COALESCE(SUM(valor_centavos), 0) FROM gastos_fixos");
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
        g.data          = q.value(3).toDate();
        g.categoriaId   = q.value(4).toInt();
        g.categoriaNome = q.value(5).toString();
        lista.append(g);
    }
    return lista;
}

bool DatabaseManager::inserirGastoVariavel(GastoVariavel &gasto)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO gastos_variaveis (historico, valor_centavos, data, categoria_id)"
        " VALUES (:hist, :valor, :data, :catId)"
    );
    q.bindValue(":hist",  gasto.historico);
    q.bindValue(":valor", gasto.valorCentavos);
    q.bindValue(":data",  gasto.data.toString("yyyy-MM-dd"));
    q.bindValue(":catId", gasto.categoriaId);
    if (!q.exec()) {
        qDebug() << "inserirGastoVariavel:" << q.lastError().text();
        return false;
    }
    gasto.id = q.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::atualizarGastoVariavel(const GastoVariavel &gasto)
{
    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE gastos_variaveis"
        " SET historico = :hist, valor_centavos = :valor, data = :data, categoria_id = :catId"
        " WHERE id = :id"
    );
    q.bindValue(":hist",  gasto.historico);
    q.bindValue(":valor", gasto.valorCentavos);
    q.bindValue(":data",  gasto.data.toString("yyyy-MM-dd"));
    q.bindValue(":catId", gasto.categoriaId);
    q.bindValue(":id",    gasto.id);
    if (!q.exec()) {
        qDebug() << "atualizarGastoVariavel:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removerGastoVariavel(int id)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM gastos_variaveis WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        qDebug() << "removerGastoVariavel:" << q.lastError().text();
        return false;
    }
    return true;
}

qint64 DatabaseManager::totalGastosVariaveis()
{
    QSqlQuery q(m_db);
    q.exec("SELECT COALESCE(SUM(valor_centavos), 0) FROM gastos_variaveis");
    return q.next() ? q.value(0).toLongLong() : 0;
}
