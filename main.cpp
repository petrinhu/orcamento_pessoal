// main.cpp
// App Orçamento Pessoal – versão monolítica com encriptação AES-256-CBC (OpenSSL)
// Copyright (c) 2026 Petrus Silva Costa - Licença MIT

#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QHeaderView>
#include <QLineEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QFont>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QComboBox>
#include <QListWidget>
#include <QInputDialog>
#include <QHBoxLayout>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QPainter>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/err.h>

// Constantes de criptografia (valores seguros para 2026)
constexpr int AES_KEY_LENGTH    = 32;
constexpr int AES_IV_LENGTH     = 16;
constexpr int SALT_LENGTH       = 16;
constexpr int DERIVED_LENGTH    = AES_KEY_LENGTH + AES_IV_LENGTH;
constexpr int PBKDF2_ITERATIONS = 600000;

class PasswordDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PasswordDialog(QWidget *parent = nullptr)
    : QDialog(parent)
    {
        setWindowTitle("Senha do Orçamento");
        setFixedSize(400, 450);
        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *label = new QLabel("Digite a senha:", this);
        layout->addWidget(label);
        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setEchoMode(QLineEdit::Password);
        m_lineEdit->setPlaceholderText("Senha");
        layout->addWidget(m_lineEdit);
        m_checkMostrar = new QCheckBox("Mostrar senha", this);
        QObject::connect(m_checkMostrar, &QCheckBox::toggled, this, [this](bool checked) {
            m_lineEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        });
        layout->addWidget(m_checkMostrar);
        layout->addWidget(new QLabel("Requisitos:", this));
        m_labelTamanho = new QLabel("• Mínimo 8 caracteres");
        m_labelMaiuscula = new QLabel("• Pelo menos uma maiúscula");
        m_labelMinuscula = new QLabel("• Pelo menos uma minúscula");
        m_labelNumero = new QLabel("• Pelo menos um número");
        m_labelEspecial = new QLabel("• Pelo menos um símbolo (!@#$%^&*)");
        layout->addWidget(m_labelTamanho);
        layout->addWidget(m_labelMaiuscula);
        layout->addWidget(m_labelMinuscula);
        layout->addWidget(m_labelNumero);
        layout->addWidget(m_labelEspecial);
        QPushButton *btnOk = new QPushButton("OK", this);
        QObject::connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(btnOk);
        QObject::connect(m_lineEdit, &QLineEdit::textChanged, this, &PasswordDialog::validarSenha);
        m_lineEdit->setFocus();
    }
    QString password() const { return m_lineEdit->text(); }
private slots:
    void validarSenha(const QString &senha)
    {
        bool tamanho = senha.length() >= 8;
        bool maiuscula = senha.contains(QRegularExpression("[A-Z]"));
        bool minuscula = senha.contains(QRegularExpression("[a-z]"));
        bool numero = senha.contains(QRegularExpression("[0-9]"));
        bool especial = senha.contains(QRegularExpression("[!@#$%^&*()_+]"));
        atualizarLabel(m_labelTamanho, tamanho);
        atualizarLabel(m_labelMaiuscula, maiuscula);
        atualizarLabel(m_labelMinuscula, minuscula);
        atualizarLabel(m_labelNumero, numero);
        atualizarLabel(m_labelEspecial, especial);
    }
    void atualizarLabel(QLabel *label, bool ok)
    {
        QString texto = label->text();
        label->setText(ok ? "✓ " + texto.mid(2) : "✗ " + texto.mid(2));
        label->setStyleSheet(ok ? "color: green;" : "color: red;");
    }
private:
    QLineEdit *m_lineEdit;
    QCheckBox *m_checkMostrar;
    QLabel *m_labelTamanho;
    QLabel *m_labelMaiuscula;
    QLabel *m_labelMinuscula;
    QLabel *m_labelNumero;
    QLabel *m_labelEspecial;
};

class DataManager
{
public:
    static DataManager& instance()
    {
        static DataManager inst;
        return inst;
    }
    bool abrirDados(const QString &senha)
    {
        m_senha = senha;
        QString caminhoDados = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        QDir dir;
        if (!dir.mkpath(caminhoDados)) {
            qDebug() << "Erro ao criar pasta" << caminhoDados;
            return false;
        }
        m_caminhoArquivo = caminhoDados + "/orcamento.enc";
        QFile file(m_caminhoArquivo);
        if (!file.exists()) {
            m_dados = QJsonObject();
            QJsonArray cats = {"Aluguel/Moradia", "Internet", "Luz/Água/Gás", "Transporte", "Alimentação", "Educação", "Saúde", "Streaming/TV/Telefone", "Academia", "Outros", "Crédito"};
            m_dados["categorias_gastos_fixos"] = cats;
            salvarDados();
            return true;
        }
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Erro ao abrir arquivo:" << file.errorString();
            return false;
        }
        QByteArray data = file.readAll();
        file.close();
        QByteArray salt = data.left(SALT_LENGTH);
        QByteArray ciphertext = data.mid(SALT_LENGTH);
        QByteArray chave, iv;
        if (!derivarChaveEIV(salt, senha, chave, iv)) return false;
        QByteArray decrypted = decryptAES(ciphertext, chave, iv);
        if (decrypted.isEmpty()) return false;
        QJsonDocument doc = QJsonDocument::fromJson(decrypted);
        if (doc.isNull()) {
            qDebug() << "JSON inválido";
            return false;
        }
        m_dados = doc.object();
        // Garante categorias padrão
        QJsonArray cats = m_dados["categorias_gastos_fixos"].toArray();
        QJsonArray defaults = {"Aluguel/Moradia", "Internet", "Luz/Água/Gás", "Transporte", "Alimentação", "Educação", "Saúde", "Streaming/TV/Telefone", "Academia", "Outros", "Crédito"};
        bool mudou = false;
        for (const QJsonValue &def : defaults) {
            if (!cats.contains(def)) {
                cats.append(def);
                mudou = true;
            }
        }
        if (mudou || cats.isEmpty()) {
            m_dados["categorias_gastos_fixos"] = cats;
            salvarDados();
        }
        return true;
    }
    void salvarDados()
    {
        QJsonDocument doc(m_dados);
        QByteArray data = doc.toJson(QJsonDocument::Indented);
        QByteArray salt(SALT_LENGTH, 0);
        RAND_bytes(reinterpret_cast<unsigned char*>(salt.data()), SALT_LENGTH);
        QByteArray chave, iv;
        if (!derivarChaveEIV(salt, m_senha, chave, iv)) return;
        QByteArray encrypted = encryptAES(data, chave, iv);
        if (encrypted.isEmpty()) return;
        QFile file(m_caminhoArquivo);
        if (!file.open(QIODevice::WriteOnly)) return;
        file.write(salt);
        file.write(encrypted);
        file.close();
    }
    QJsonObject dados() const { return m_dados; }
    void setDados(const QJsonObject &dados) { m_dados = dados; }
private:
    DataManager() = default;
    QJsonObject m_dados;
    QString m_senha;
    QString m_caminhoArquivo;
    bool derivarChaveEIV(const QByteArray &salt, const QString &senha, QByteArray &chave, QByteArray &iv)
    {
        QByteArray derived(DERIVED_LENGTH, 0);
        int res = PKCS5_PBKDF2_HMAC(senha.toUtf8().constData(), senha.toUtf8().length(),
                                    reinterpret_cast<const unsigned char*>(salt.constData()), salt.length(),
                                    PBKDF2_ITERATIONS, EVP_sha256(), DERIVED_LENGTH,
                                    reinterpret_cast<unsigned char*>(derived.data()));
        if (res != 1) return false;
        chave = derived.left(AES_KEY_LENGTH);
        iv = derived.mid(AES_KEY_LENGTH, AES_IV_LENGTH);
        return true;
    }
    QByteArray encryptAES(const QByteArray &plaintext, const QByteArray &key, const QByteArray &iv)
    {
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};
        QByteArray ciphertext(plaintext.size() + 32, 0);
        int len = 0, totalLen = 0;
        EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData()));
        EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &len,
                          reinterpret_cast<const unsigned char*>(plaintext.constData()), plaintext.size());
        totalLen += len;
        EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len);
        totalLen += len;
        EVP_CIPHER_CTX_free(ctx);
        ciphertext.resize(totalLen);
        return ciphertext;
    }
    QByteArray decryptAES(const QByteArray &ciphertext, const QByteArray &key, const QByteArray &iv)
    {
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};
        QByteArray plaintext(ciphertext.size() + 32, 0);
        int len = 0, totalLen = 0;
        EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData()));
        EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &len,
                          reinterpret_cast<const unsigned char*>(ciphertext.constData()), ciphertext.size());
        totalLen += len;
        if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        totalLen += len;
        EVP_CIPHER_CTX_free(ctx);
        plaintext.resize(totalLen);
        return plaintext;
    }
};
// Função auxiliar: converte texto digitado para centavos (inteiro)
static qint64 textoParaCentavos(const QString &texto)
{
    QString limpo = texto;
    limpo.remove('.').remove(',').remove('R').remove('$').remove(' ');
    if (limpo.isEmpty()) return 0;
    QString str = limpo.rightJustified(3, '0');
    QString cents = str.right(2);
    QString reais = str.left(str.length() - 2);
    if (reais.isEmpty()) reais = "0";
    return reais.toLongLong() * 100 + cents.toLongLong();
}
// Função auxiliar: converte centavos para texto com R$ e vírgula
static QString centavosParaTexto(qint64 centavos)
{
    if (centavos == 0) return "R$ 0,00";
    QString str = QString::number(qAbs(centavos));
    while (str.length() < 3) str.prepend('0');
    QString reais = str.left(str.length() - 2);
    QString cents = str.right(2);
    if (reais.isEmpty()) reais = "0";
    return (centavos < 0 ? "-R$ " : "R$ ") + reais + "," + cents;
}
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    PasswordDialog dlg;
    while (true) {
        if (dlg.exec() != QDialog::Accepted) {
            return 0;
        }
        QString senha = dlg.password();
        if (DataManager::instance().abrirDados(senha)) {
            break;
        } else {
            QMessageBox::critical(nullptr, "Senha incorreta", "A senha está errada. Tente novamente.");
        }
    }
    QJsonObject dados = DataManager::instance().dados();
    QMainWindow window;
    window.setWindowTitle("Orçamento Pessoal v0.1");
    window.resize(1200, 800);
    QTabWidget *tabWidget = new QTabWidget(&window);
    QWidget *dashboardPage = new QWidget();
    QVBoxLayout *dashLayout = new QVBoxLayout(dashboardPage);
    QLabel *saldoLabel = new QLabel();
    dashLayout->addWidget(saldoLabel);
    QChartView *chartView = new QChartView(dashboardPage);
    chartView->setRenderHint(QPainter::Antialiasing);
    dashLayout->addWidget(chartView);
    dashLayout->addStretch();
    tabWidget->addTab(dashboardPage, "Dashboard");
    // Atualiza dashboard
    auto atualizarDashboard = [&]() {
        qint64 saldoCentavos = 0;
        QJsonArray entradasArray = dados.value("entradas").toArray();
        for (const QJsonValue &value : entradasArray) {
            QJsonObject obj = value.toObject();
            saldoCentavos += obj["valor_centavos"].toVariant().toLongLong();
        }
        QJsonArray gastosFixosArray = dados.value("gastos_fixos").toArray();
        qint64 gastosFixosCentavos = 0;
        for (const QJsonValue &value : gastosFixosArray) {
            QJsonObject obj = value.toObject();
            gastosFixosCentavos += obj["valor_centavos"].toVariant().toLongLong();
        }
        QJsonArray gastosVariaveisArray = dados.value("gastos_variaveis").toArray();
        qint64 gastosVariaveisCentavos = 0;
        for (const QJsonValue &value : gastosVariaveisArray) {
            QJsonObject obj = value.toObject();
            gastosVariaveisCentavos += obj["valor_centavos"].toVariant().toLongLong();
        }
        qint64 saldoFinal = saldoCentavos - gastosFixosCentavos - gastosVariaveisCentavos;
        saldoLabel->setText(QString("<h2>Saldo Atual: <span style='color:%1;font-weight:bold;'>%2</span></h2>")
        .arg(saldoFinal >= 0 ? "green" : "red")
        .arg(centavosParaTexto(saldoFinal)));
        QPieSeries *series = new QPieSeries();
        if (saldoCentavos > 0) series->append("Entradas", saldoCentavos / 100.0);
        if (gastosFixosCentavos > 0) series->append("Gastos Fixos", gastosFixosCentavos / 100.0);
        if (gastosVariaveisCentavos > 0) series->append("Gastos Variáveis", gastosVariaveisCentavos / 100.0);
        series->append("Outros (exemplo)", 20);
        for (QPieSlice *slice : series->slices()) {
            slice->setLabelVisible(true);
        }
        QChart *chart = new QChart();
        chart->addSeries(series);
        chart->setTitle("Distribuição Atual");
        chart->setTheme(QChart::ChartThemeQt);
        chartView->setChart(chart);
    };
    atualizarDashboard();
    // Aba Entradas
    QWidget *entradasPage = new QWidget();
    QVBoxLayout *entradasLayout = new QVBoxLayout(entradasPage);
    QTableWidget *tabelaEntradas = new QTableWidget(0, 3, entradasPage);
    tabelaEntradas->setHorizontalHeaderLabels({"Data", "Origem", "Valor (R$)"});
    tabelaEntradas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tabelaEntradas->verticalHeader()->setVisible(false);
    QPushButton *btnAdicionarEntradas = new QPushButton("Adicionar Entrada", entradasPage);
    QPushButton *btnRemoverEntradas = new QPushButton("Remover linha selecionada", entradasPage);
    tabelaEntradas->blockSignals(true);
    QObject::connect(btnAdicionarEntradas, &QPushButton::clicked, [&]() {
        tabelaEntradas->blockSignals(true);
        int row = tabelaEntradas->rowCount() - 1;
        tabelaEntradas->insertRow(row);
        tabelaEntradas->setItem(row, 0, new QTableWidgetItem(QDate::currentDate().toString("yyyy-MM-dd")));
        tabelaEntradas->setItem(row, 1, new QTableWidgetItem("Particular"));
        tabelaEntradas->setItem(row, 2, new QTableWidgetItem("R$ 0,00"));
        tabelaEntradas->blockSignals(false);
        QJsonArray array;
        for (int r = 0; r < tabelaEntradas->rowCount() - 1; ++r) {
            QJsonObject obj;
            obj["data"] = tabelaEntradas->item(r, 0)->text();
            obj["origem"] = tabelaEntradas->item(r, 1)->text();
            obj["valor_centavos"] = textoParaCentavos(tabelaEntradas->item(r, 2)->text());
            array.append(obj);
        }
        dados["entradas"] = array;
        DataManager::instance().setDados(dados);
        DataManager::instance().salvarDados();
        tabelaEntradas->blockSignals(true);
        qint64 total = 0;
        for (int r = 0; r < tabelaEntradas->rowCount() - 1; ++r) {
            total += textoParaCentavos(tabelaEntradas->item(r, 2)->text());
        }
        tabelaEntradas->item(tabelaEntradas->rowCount() - 1, 2)->setText(centavosParaTexto(total));
        tabelaEntradas->blockSignals(false);
        atualizarDashboard();
    });
    QObject::connect(btnRemoverEntradas, &QPushButton::clicked, [&]() {
        int row = tabelaEntradas->currentRow();
        if (row < 0 || row == tabelaEntradas->rowCount() - 1) {
            QMessageBox::information(nullptr, "Remover", "Selecione uma linha válida para remover.");
            return;
        }
        if (QMessageBox::question(nullptr, "Confirmar", "Remover a linha selecionada?") == QMessageBox::Yes) {
            tabelaEntradas->removeRow(row);
            QJsonArray array;
            for (int r = 0; r < tabelaEntradas->rowCount() - 1; ++r) {
                QJsonObject obj;
                obj["data"] = tabelaEntradas->item(r, 0)->text();
                obj["origem"] = tabelaEntradas->item(r, 1)->text();
                obj["valor_centavos"] = textoParaCentavos(tabelaEntradas->item(r, 2)->text());
                array.append(obj);
            }
            dados["entradas"] = array;
            DataManager::instance().setDados(dados);
            DataManager::instance().salvarDados();
            tabelaEntradas->blockSignals(true);
            qint64 total = 0;
            for (int r = 0; r < tabelaEntradas->rowCount() - 1; ++r) {
                total += textoParaCentavos(tabelaEntradas->item(r, 2)->text());
            }
            tabelaEntradas->item(tabelaEntradas->rowCount() - 1, 2)->setText(centavosParaTexto(total));
            tabelaEntradas->blockSignals(false);
            atualizarDashboard();
        }
    });
    QObject::connect(tabelaEntradas, &QTableWidget::itemChanged, [&]() {
        QTableWidgetItem *item = tabelaEntradas->currentItem();
        if (item && item->column() == 2) {
            qint64 centavos = textoParaCentavos(item->text());
            item->setText(centavosParaTexto(centavos));
        }
        QJsonArray array;
        for (int r = 0; r < tabelaEntradas->rowCount() - 1; ++r) {
            QJsonObject obj;
            obj["data"] = tabelaEntradas->item(r, 0)->text();
            obj["origem"] = tabelaEntradas->item(r, 1)->text();
            obj["valor_centavos"] = textoParaCentavos(tabelaEntradas->item(r, 2)->text());
            array.append(obj);
        }
        dados["entradas"] = array;
        DataManager::instance().setDados(dados);
        DataManager::instance().salvarDados();
        tabelaEntradas->blockSignals(true);
        qint64 total = 0;
        for (int r = 0; r < tabelaEntradas->rowCount() - 1; ++r) {
            total += textoParaCentavos(tabelaEntradas->item(r, 2)->text());
        }
        tabelaEntradas->item(tabelaEntradas->rowCount() - 1, 2)->setText(centavosParaTexto(total));
        tabelaEntradas->blockSignals(false);
        atualizarDashboard();
    });
    // Load das entradas
    QJsonArray arrayEntradas = dados.value("entradas").toArray();
    for (const QJsonValue &value : arrayEntradas) {
        QJsonObject obj = value.toObject();
        int row = tabelaEntradas->rowCount();
        tabelaEntradas->insertRow(row);
        tabelaEntradas->setItem(row, 0, new QTableWidgetItem(obj["data"].toString()));
        tabelaEntradas->setItem(row, 1, new QTableWidgetItem(obj["origem"].toString()));
        qint64 centavos = obj["valor_centavos"].toVariant().toLongLong();
        tabelaEntradas->setItem(row, 2, new QTableWidgetItem(centavosParaTexto(centavos)));
    }
    // Cria linha TOTAL
    int totalRowEntradas = tabelaEntradas->rowCount();
    tabelaEntradas->insertRow(totalRowEntradas);
    QTableWidgetItem *labelTotalEntradas = new QTableWidgetItem("TOTAL");
    labelTotalEntradas->setFont(QFont("Arial", 10, QFont::Bold));
    labelTotalEntradas->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    labelTotalEntradas->setFlags(labelTotalEntradas->flags() & ~Qt::ItemIsEditable);
    tabelaEntradas->setItem(totalRowEntradas, 0, labelTotalEntradas);
    QTableWidgetItem *emptyEntradas = new QTableWidgetItem("");
    emptyEntradas->setFlags(emptyEntradas->flags() & ~Qt::ItemIsEditable);
    tabelaEntradas->setItem(totalRowEntradas, 1, emptyEntradas);
    qint64 totalEntradas = 0;
    for (const QJsonValue &value : arrayEntradas) {
        totalEntradas += value.toObject()["valor_centavos"].toVariant().toLongLong();
    }
    QTableWidgetItem *totalItemEntradas = new QTableWidgetItem(centavosParaTexto(totalEntradas));
    totalItemEntradas->setFont(QFont("Arial", 10, QFont::Bold));
    totalItemEntradas->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totalItemEntradas->setFlags(totalItemEntradas->flags() & ~Qt::ItemIsEditable);
    tabelaEntradas->setItem(totalRowEntradas, 2, totalItemEntradas);
    tabelaEntradas->blockSignals(false);
    QHBoxLayout *buttonsEntradas = new QHBoxLayout();
    buttonsEntradas->addWidget(btnAdicionarEntradas);
    buttonsEntradas->addWidget(btnRemoverEntradas);
    entradasLayout->addWidget(tabelaEntradas);
    entradasLayout->addLayout(buttonsEntradas);
    tabWidget->addTab(entradasPage, "Entradas");
    // Aba Gastos Fixos
    QWidget *gastosFixosPage = new QWidget();
    QVBoxLayout *gastosFixosLayout = new QVBoxLayout(gastosFixosPage);
    QTableWidget *tabelaGastosFixos = new QTableWidget(0, 4, gastosFixosPage);
    tabelaGastosFixos->setHorizontalHeaderLabels({"Data", "Histórico", "Valor (R$)", "Tipo"});
    tabelaGastosFixos->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tabelaGastosFixos->verticalHeader()->setVisible(false);
    QPushButton *btnAdicionarGastosFixos = new QPushButton("Adicionar Gasto Fixo", gastosFixosPage);
    QPushButton *btnRemoverGastosFixos = new QPushButton("Remover linha selecionada", gastosFixosPage);
    QPushButton *btnRepetirMesAnterior = new QPushButton("Repetir do mês anterior", gastosFixosPage);
    QJsonArray categorias = dados.value("categorias_gastos_fixos").toArray();
    tabelaGastosFixos->blockSignals(true);
    auto preencherCombo = [categorias](QComboBox *combo) {
        combo->clear();
        for (const QJsonValue &cat : categorias) {
            combo->addItem(cat.toString());
        }
    };
    QObject::connect(btnAdicionarGastosFixos, &QPushButton::clicked, [&]() {
        tabelaGastosFixos->blockSignals(true);
        int row = tabelaGastosFixos->rowCount() - 1;
        tabelaGastosFixos->insertRow(row);
        tabelaGastosFixos->setItem(row, 0, new QTableWidgetItem(QDate::currentDate().toString("yyyy-MM-dd")));
        tabelaGastosFixos->setItem(row, 1, new QTableWidgetItem(""));
        tabelaGastosFixos->setItem(row, 2, new QTableWidgetItem("R$ 0,00"));
        QComboBox *combo = new QComboBox();
        preencherCombo(combo);
        tabelaGastosFixos->setCellWidget(row, 3, combo);
        tabelaGastosFixos->blockSignals(false);
        QJsonArray array;
        for (int r = 0; r < tabelaGastosFixos->rowCount() - 1; ++r) {
            QJsonObject obj;
            obj["data"] = tabelaGastosFixos->item(r, 0)->text();
            obj["historico"] = tabelaGastosFixos->item(r, 1)->text();
            obj["valor_centavos"] = textoParaCentavos(tabelaGastosFixos->item(r, 2)->text());
            QComboBox *combo = qobject_cast<QComboBox*>(tabelaGastosFixos->cellWidget(r, 3));
            obj["tipo"] = combo ? combo->currentText() : "Outros";
            array.append(obj);
        }
        dados["gastos_fixos"] = array;
        DataManager::instance().setDados(dados);
        DataManager::instance().salvarDados();
        tabelaGastosFixos->blockSignals(true);
        qint64 total = 0;
        for (int r = 0; r < tabelaGastosFixos->rowCount() - 1; ++r) {
            total += textoParaCentavos(tabelaGastosFixos->item(r, 2)->text());
        }
        tabelaGastosFixos->item(tabelaGastosFixos->rowCount() - 1, 2)->setText(centavosParaTexto(total));
        tabelaGastosFixos->blockSignals(false);
        atualizarDashboard();
    });
    QObject::connect(btnRemoverGastosFixos, &QPushButton::clicked, [&]() {
        int row = tabelaGastosFixos->currentRow();
        if (row < 0 || row == tabelaGastosFixos->rowCount() - 1) {
            QMessageBox::information(nullptr, "Remover", "Selecione uma linha válida para remover.");
            return;
        }
        if (QMessageBox::question(nullptr, "Confirmar", "Remover a linha selecionada?") == QMessageBox::Yes) {
            tabelaGastosFixos->removeRow(row);
            QJsonArray array;
            for (int r = 0; r < tabelaGastosFixos->rowCount() - 1; ++r) {
                QJsonObject obj;
                obj["data"] = tabelaGastosFixos->item(r, 0)->text();
                obj["historico"] = tabelaGastosFixos->item(r, 1)->text();
                obj["valor_centavos"] = textoParaCentavos(tabelaGastosFixos->item(r, 2)->text());
                QComboBox *combo = qobject_cast<QComboBox*>(tabelaGastosFixos->cellWidget(r, 3));
                obj["tipo"] = combo ? combo->currentText() : "Outros";
                array.append(obj);
            }
            dados["gastos_fixos"] = array;
            DataManager::instance().setDados(dados);
            DataManager::instance().salvarDados();
            tabelaGastosFixos->blockSignals(true);
            qint64 total = 0;
            for (int r = 0; r < tabelaGastosFixos->rowCount() - 1; ++r) {
                total += textoParaCentavos(tabelaGastosFixos->item(r, 2)->text());
            }
            tabelaGastosFixos->item(tabelaGastosFixos->rowCount() - 1, 2)->setText(centavosParaTexto(total));
            tabelaGastosFixos->blockSignals(false);
            atualizarDashboard();
        }
    });
    QObject::connect(btnRepetirMesAnterior, &QPushButton::clicked, [&]() {
        QJsonArray arrayAtual = dados.value("gastos_fixos").toArray();
        QJsonArray arrayNovo;
        QDate hoje = QDate::currentDate();
        QDate mesAnterior = hoje.addMonths(-1);
        for (const QJsonValue &value : arrayAtual) {
            QJsonObject obj = value.toObject();
            QDate dataAntiga = QDate::fromString(obj["data"].toString(), "yyyy-MM-dd");
            if (dataAntiga.month() == mesAnterior.month() && dataAntiga.year() == mesAnterior.year()) {
                QJsonObject novoObj = obj;
                novoObj["data"] = hoje.toString("yyyy-MM-dd");
                novoObj["valor_centavos"] = 0;
                arrayNovo.append(novoObj);
            }
        }
        if (arrayNovo.isEmpty()) {
            QMessageBox::information(nullptr, "Repetir", "Não há gastos fixos no mês anterior para repetir.");
            return;
        }
        tabelaGastosFixos->blockSignals(true);
        for (const QJsonValue &value : arrayNovo) {
            QJsonObject obj = value.toObject();
            int row = tabelaGastosFixos->rowCount() - 1;
            tabelaGastosFixos->insertRow(row);
            tabelaGastosFixos->setItem(row, 0, new QTableWidgetItem(obj["data"].toString()));
            tabelaGastosFixos->setItem(row, 1, new QTableWidgetItem(obj["historico"].toString()));
            tabelaGastosFixos->setItem(row, 2, new QTableWidgetItem("R$ 0,00"));
            QComboBox *combo = new QComboBox();
            preencherCombo(combo);
            combo->setCurrentText(obj["tipo"].toString());
            tabelaGastosFixos->setCellWidget(row, 3, combo);
        }
        tabelaGastosFixos->blockSignals(false);
        dados["gastos_fixos"] = dados.value("gastos_fixos").toArray() + arrayNovo;
        DataManager::instance().setDados(dados);
        DataManager::instance().salvarDados();
        tabelaGastosFixos->blockSignals(true);
        qint64 total = 0;
        for (int r = 0; r < tabelaGastosFixos->rowCount() - 1; ++r) {
            total += textoParaCentavos(tabelaGastosFixos->item(r, 2)->text());
        }
        tabelaGastosFixos->item(tabelaGastosFixos->rowCount() - 1, 2)->setText(centavosParaTexto(total));
        tabelaGastosFixos->blockSignals(false);
        atualizarDashboard();
        QMessageBox::information(nullptr, "Repetir", "Gastos fixos do mês anterior repetidos com sucesso (histórico e tipo copiados, valor aberto).");
    });
    QObject::connect(tabelaGastosFixos, &QTableWidget::itemChanged, [&]() {
        QTableWidgetItem *item = tabelaGastosFixos->currentItem();
        if (item && item->column() == 2) {
            qint64 centavos = textoParaCentavos(item->text());
            item->setText(centavosParaTexto(centavos));
        }
        QJsonArray array;
        for (int r = 0; r < tabelaGastosFixos->rowCount() - 1; ++r) {
            QJsonObject obj;
            obj["data"] = tabelaGastosFixos->item(r, 0)->text();
            obj["historico"] = tabelaGastosFixos->item(r, 1)->text();
            obj["valor_centavos"] = textoParaCentavos(tabelaGastosFixos->item(r, 2)->text());
            QComboBox *combo = qobject_cast<QComboBox*>(tabelaGastosFixos->cellWidget(r, 3));
            obj["tipo"] = combo ? combo->currentText() : "Outros";
            array.append(obj);
        }
        dados["gastos_fixos"] = array;
        DataManager::instance().setDados(dados);
        DataManager::instance().salvarDados();
        tabelaGastosFixos->blockSignals(true);
        qint64 total = 0;
        for (int r = 0; r < tabelaGastosFixos->rowCount() - 1; ++r) {
            total += textoParaCentavos(tabelaGastosFixos->item(r, 2)->text());
        }
        tabelaGastosFixos->item(tabelaGastosFixos->rowCount() - 1, 2)->setText(centavosParaTexto(total));
        tabelaGastosFixos->blockSignals(false);
        atualizarDashboard();
    });
    QJsonArray arrayGastosFixos = dados.value("gastos_fixos").toArray();
    for (const QJsonValue &value : arrayGastosFixos) {
        QJsonObject obj = value.toObject();
        int row = tabelaGastosFixos->rowCount();
        tabelaGastosFixos->insertRow(row);
        tabelaGastosFixos->setItem(row, 0, new QTableWidgetItem(obj["data"].toString()));
        tabelaGastosFixos->setItem(row, 1, new QTableWidgetItem(obj["historico"].toString()));
        qint64 centavos = obj["valor_centavos"].toVariant().toLongLong();
        tabelaGastosFixos->setItem(row, 2, new QTableWidgetItem(centavosParaTexto(centavos)));
        QComboBox *combo = new QComboBox();
        preencherCombo(combo);
        combo->setCurrentText(obj["tipo"].toString());
        tabelaGastosFixos->setCellWidget(row, 3, combo);
    }
    int totalRowFixos = tabelaGastosFixos->rowCount();
    tabelaGastosFixos->insertRow(totalRowFixos);
    QTableWidgetItem *labelTotalFixos = new QTableWidgetItem("TOTAL");
    labelTotalFixos->setFont(QFont("Arial", 10, QFont::Bold));
    labelTotalFixos->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    labelTotalFixos->setFlags(labelTotalFixos->flags() & ~Qt::ItemIsEditable);
    tabelaGastosFixos->setItem(totalRowFixos, 0, labelTotalFixos);
    QTableWidgetItem *emptyFixos = new QTableWidgetItem("");
    emptyFixos->setFlags(emptyFixos->flags() & ~Qt::ItemIsEditable);
    tabelaGastosFixos->setItem(totalRowFixos, 1, emptyFixos);
    QTableWidgetItem *empty2Fixos = new QTableWidgetItem("");
    empty2Fixos->setFlags(empty2Fixos->flags() & ~Qt::ItemIsEditable);
    tabelaGastosFixos->setItem(totalRowFixos, 3, empty2Fixos);
    qint64 totalGastosFixos = 0;
    for (const QJsonValue &value : arrayGastosFixos) {
        totalGastosFixos += value.toObject()["valor_centavos"].toVariant().toLongLong();
    }
    QTableWidgetItem *totalItemFixos = new QTableWidgetItem(centavosParaTexto(totalGastosFixos));
    totalItemFixos->setFont(QFont("Arial", 10, QFont::Bold));
    totalItemFixos->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totalItemFixos->setFlags(totalItemFixos->flags() & ~Qt::ItemIsEditable);
    tabelaGastosFixos->setItem(totalRowFixos, 2, totalItemFixos);
    tabelaGastosFixos->blockSignals(false);
    QHBoxLayout *buttonsFixosLayout = new QHBoxLayout();
    buttonsFixosLayout->addWidget(btnAdicionarGastosFixos);
    buttonsFixosLayout->addWidget(btnRemoverGastosFixos);
    buttonsFixosLayout->addWidget(btnRepetirMesAnterior);
    gastosFixosLayout->addWidget(tabelaGastosFixos);
    gastosFixosLayout->addLayout(buttonsFixosLayout);
    tabWidget->addTab(gastosFixosPage, "Gastos Fixos");
    // Aba Gastos Variáveis
    QWidget *gastosVariaveisPage = new QWidget();
    QVBoxLayout *gastosVariaveisLayout = new QVBoxLayout(gastosVariaveisPage);
    QTableWidget *tabelaGastosVariaveis = new QTableWidget(0, 4, gastosVariaveisPage);
    tabelaGastosVariaveis->setHorizontalHeaderLabels({"Data", "Histórico", "Valor (R$)", "Tipo"});
    tabelaGastosVariaveis->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tabelaGastosVariaveis->verticalHeader()->setVisible(false);
    QPushButton *btnAdicionarGastosVariaveis = new QPushButton("Adicionar Gasto Variável", gastosVariaveisPage);
    QPushButton *btnRemoverGastosVariaveis = new QPushButton("Remover linha selecionada", gastosVariaveisPage);
    tabelaGastosVariaveis->blockSignals(true);
    QObject::connect(btnAdicionarGastosVariaveis, &QPushButton::clicked, [&]() {
        tabelaGastosVariaveis->blockSignals(true);
        int row = tabelaGastosVariaveis->rowCount() - 1;
        tabelaGastosVariaveis->insertRow(row);
        tabelaGastosVariaveis->setItem(row, 0, new QTableWidgetItem(QDate::currentDate().toString("yyyy-MM-dd")));
        tabelaGastosVariaveis->setItem(row, 1, new QTableWidgetItem(""));
        tabelaGastosVariaveis->setItem(row, 2, new QTableWidgetItem("R$ 0,00"));
        QComboBox *combo = new QComboBox();
        preencherCombo(combo);
        tabelaGastosVariaveis->setCellWidget(row, 3, combo);
        tabelaGastosVariaveis->blockSignals(false);
        QJsonArray array;
        for (int r = 0; r < tabelaGastosVariaveis->rowCount() - 1; ++r) {
            QJsonObject obj;
            obj["data"] = tabelaGastosVariaveis->item(r, 0)->text();
            obj["historico"] = tabelaGastosVariaveis->item(r, 1)->text();
            obj["valor_centavos"] = textoParaCentavos(tabelaGastosVariaveis->item(r, 2)->text());
            QComboBox *combo = qobject_cast<QComboBox*>(tabelaGastosVariaveis->cellWidget(r, 3));
            obj["tipo"] = combo ? combo->currentText() : "Outros";
            array.append(obj);
        }
        dados["gastos_variaveis"] = array;
        DataManager::instance().setDados(dados);
        DataManager::instance().salvarDados();
        tabelaGastosVariaveis->blockSignals(true);
        qint64 total = 0;
        for (int r = 0; r < tabelaGastosVariaveis->rowCount() - 1; ++r) {
            total += textoParaCentavos(tabelaGastosVariaveis->item(r, 2)->text());
        }
        QTableWidgetItem *totalItem = tabelaGastosVariaveis->item(tabelaGastosVariaveis->rowCount() - 1, 2);
        if (totalItem) totalItem->setText(centavosParaTexto(total));
        tabelaGastosVariaveis->blockSignals(false);
        atualizarDashboard();
    });
    QObject::connect(btnRemoverGastosVariaveis, &QPushButton::clicked, [&]() {
        int row = tabelaGastosVariaveis->currentRow();
        if (row < 0 || row == tabelaGastosVariaveis->rowCount() - 1) {
            QMessageBox::information(nullptr, "Remover", "Selecione uma linha válida para remover.");
            return;
        }
        if (QMessageBox::question(nullptr, "Confirmar", "Remover a linha selecionada?") == QMessageBox::Yes) {
            tabelaGastosVariaveis->removeRow(row);
            QJsonArray array;
            for (int r = 0; r < tabelaGastosVariaveis->rowCount() - 1; ++r) {
                QJsonObject obj;
                obj["data"] = tabelaGastosVariaveis->item(r, 0)->text();
                obj["historico"] = tabelaGastosVariaveis->item(r, 1)->text();
                obj["valor_centavos"] = textoParaCentavos(tabelaGastosVariaveis->item(r, 2)->text());
                QComboBox *combo = qobject_cast<QComboBox*>(tabelaGastosVariaveis->cellWidget(r, 3));
                obj["tipo"] = combo ? combo->currentText() : "Outros";
                array.append(obj);
            }
            dados["gastos_variaveis"] = array;
            DataManager::instance().setDados(dados);
            DataManager::instance().salvarDados();
            tabelaGastosVariaveis->blockSignals(true);
            qint64 total = 0;
            for (int r = 0; r < tabelaGastosVariaveis->rowCount() - 1; ++r) {
                total += textoParaCentavos(tabelaGastosVariaveis->item(r, 2)->text());
            }
            QTableWidgetItem *totalItem = tabelaGastosVariaveis->item(tabelaGastosVariaveis->rowCount() - 1, 2);
            if (totalItem) totalItem->setText(centavosParaTexto(total));
            tabelaGastosVariaveis->blockSignals(false);
            atualizarDashboard();
        }
    });
    QObject::connect(tabelaGastosVariaveis, &QTableWidget::itemChanged, [&]() {
        QTableWidgetItem *item = tabelaGastosVariaveis->currentItem();
        if (item && item->column() == 2) {
            qint64 centavos = textoParaCentavos(item->text());
            item->setText(centavosParaTexto(centavos));
        }
        QJsonArray array;
        for (int r = 0; r < tabelaGastosVariaveis->rowCount() - 1; ++r) {
            QJsonObject obj;
            obj["data"] = tabelaGastosVariaveis->item(r, 0)->text();
            obj["historico"] = tabelaGastosVariaveis->item(r, 1)->text();
            obj["valor_centavos"] = textoParaCentavos(tabelaGastosVariaveis->item(r, 2)->text());
            QComboBox *combo = qobject_cast<QComboBox*>(tabelaGastosVariaveis->cellWidget(r, 3));
            obj["tipo"] = combo ? combo->currentText() : "Outros";
            array.append(obj);
        }
        dados["gastos_variaveis"] = array;
        DataManager::instance().setDados(dados);
        DataManager::instance().salvarDados();
        tabelaGastosVariaveis->blockSignals(true);
        qint64 total = 0;
        for (int r = 0; r < tabelaGastosVariaveis->rowCount() - 1; ++r) {
            total += textoParaCentavos(tabelaGastosVariaveis->item(r, 2)->text());
        }
        QTableWidgetItem *totalItem = tabelaGastosVariaveis->item(tabelaGastosVariaveis->rowCount() - 1, 2);
        if (totalItem) totalItem->setText(centavosParaTexto(total));
        tabelaGastosVariaveis->blockSignals(false);
        atualizarDashboard();
    });
    QJsonArray arrayGastosVariaveis = dados.value("gastos_variaveis").toArray();
    for (const QJsonValue &value : arrayGastosVariaveis) {
        QJsonObject obj = value.toObject();
        int row = tabelaGastosVariaveis->rowCount();
        tabelaGastosVariaveis->insertRow(row);
        tabelaGastosVariaveis->setItem(row, 0, new QTableWidgetItem(obj["data"].toString()));
        tabelaGastosVariaveis->setItem(row, 1, new QTableWidgetItem(obj["historico"].toString()));
        qint64 centavos = obj["valor_centavos"].toVariant().toLongLong();
        tabelaGastosVariaveis->setItem(row, 2, new QTableWidgetItem(centavosParaTexto(centavos)));
        QComboBox *combo = new QComboBox();
        preencherCombo(combo);
        combo->setCurrentText(obj["tipo"].toString());
        tabelaGastosVariaveis->setCellWidget(row, 3, combo);
    }
    int totalRowVariaveis = tabelaGastosVariaveis->rowCount();
    tabelaGastosVariaveis->insertRow(totalRowVariaveis);
    QTableWidgetItem *labelTotalVariaveis = new QTableWidgetItem("TOTAL");
    labelTotalVariaveis->setFont(QFont("Arial", 10, QFont::Bold));
    labelTotalVariaveis->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    labelTotalVariaveis->setFlags(labelTotalVariaveis->flags() & ~Qt::ItemIsEditable);
    tabelaGastosVariaveis->setItem(totalRowVariaveis, 0, labelTotalVariaveis);
    QTableWidgetItem *emptyVariaveis = new QTableWidgetItem("");
    emptyVariaveis->setFlags(emptyVariaveis->flags() & ~Qt::ItemIsEditable);
    tabelaGastosVariaveis->setItem(totalRowVariaveis, 1, emptyVariaveis);
    QTableWidgetItem *empty2Variaveis = new QTableWidgetItem("");
    empty2Variaveis->setFlags(empty2Variaveis->flags() & ~Qt::ItemIsEditable);
    tabelaGastosVariaveis->setItem(totalRowVariaveis, 3, empty2Variaveis);
    qint64 totalGastosVariaveis = 0;
    for (const QJsonValue &value : arrayGastosVariaveis) {
        totalGastosVariaveis += value.toObject()["valor_centavos"].toVariant().toLongLong();
    }
    QTableWidgetItem *totalItemVariaveis = new QTableWidgetItem(centavosParaTexto(totalGastosVariaveis));
    totalItemVariaveis->setFont(QFont("Arial", 10, QFont::Bold));
    totalItemVariaveis->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totalItemVariaveis->setFlags(totalItemVariaveis->flags() & ~Qt::ItemIsEditable);
    tabelaGastosVariaveis->setItem(totalRowVariaveis, 2, totalItemVariaveis);
    tabelaGastosVariaveis->blockSignals(false);
    QHBoxLayout *buttonsVariaveisLayout = new QHBoxLayout();
    buttonsVariaveisLayout->addWidget(btnAdicionarGastosVariaveis);
    buttonsVariaveisLayout->addWidget(btnRemoverGastosVariaveis);
    gastosVariaveisLayout->addWidget(tabelaGastosVariaveis);
    gastosVariaveisLayout->addLayout(buttonsVariaveisLayout);
    tabWidget->addTab(gastosVariaveisPage, "Gastos Variáveis");
    // Aba Configurações (última)
    QWidget *configPage = new QWidget();
    QVBoxLayout *configLayout = new QVBoxLayout(configPage);
    QLabel *labelCats = new QLabel("Categorias de Gastos (edição):", configPage);
    configLayout->addWidget(labelCats);
    QListWidget *listCats = new QListWidget(configPage);
    for (const QJsonValue &cat : categorias) {
        listCats->addItem(cat.toString());
    }
    configLayout->addWidget(listCats);
    QPushButton *btnAddCat = new QPushButton("Adicionar categoria", configPage);
    QObject::connect(btnAddCat, &QPushButton::clicked, [listCats]() {
        bool ok;
        QString nova = QInputDialog::getText(nullptr, "Nova categoria", "Nome:", QLineEdit::Normal, "", &ok);
        if (ok && !nova.isEmpty()) {
            listCats->addItem(nova);
            QJsonArray newCats;
            for (int i = 0; i < listCats->count(); ++i) {
                newCats.append(listCats->item(i)->text());
            }
            QJsonObject dados = DataManager::instance().dados();
            dados["categorias_gastos_fixos"] = newCats;
            DataManager::instance().setDados(dados);
            DataManager::instance().salvarDados();
        }
    });
    configLayout->addWidget(btnAddCat);
    QPushButton *btnDelCat = new QPushButton("Remover selecionada", configPage);
    QObject::connect(btnDelCat, &QPushButton::clicked, [listCats]() {
        QListWidgetItem *item = listCats->currentItem();
        if (item) {
            delete item;
            QJsonArray newCats;
            for (int i = 0; i < listCats->count(); ++i) {
                newCats.append(listCats->item(i)->text());
            }
            QJsonObject dados = DataManager::instance().dados();
            dados["categorias_gastos_fixos"] = newCats;
            DataManager::instance().setDados(dados);
            DataManager::instance().salvarDados();
        }
    });
    configLayout->addWidget(btnDelCat);
    tabWidget->addTab(configPage, "Configurações");
    window.setCentralWidget(tabWidget);
    window.show();
    return app.exec();
}
#include "main.moc"
