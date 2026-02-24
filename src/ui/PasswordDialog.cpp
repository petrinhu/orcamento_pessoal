#include "ui/PasswordDialog.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QVBoxLayout>

// ── Helpers internos ──────────────────────────────────────────────────────────

static QLabel *makeReqLabel(const QString &texto)
{
    auto *l = new QLabel("· " + texto);
    l->setStyleSheet("font-size: 12px; color: #9A9895;");
    return l;
}

// ── Construtor ────────────────────────────────────────────────────────────────

PasswordDialog::PasswordDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Orçamento Pessoal");
    setFixedWidth(400);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(14);
    root->setContentsMargins(32, 32, 32, 32);

    // ── Cabeçalho ─────────────────────────────────────────────────────────────
    auto *titulo = new QLabel("Orçamento Pessoal");
    titulo->setStyleSheet("font-size: 20px; font-weight: 600;");
    titulo->setAlignment(Qt::AlignCenter);
    root->addWidget(titulo);

    auto *subtitulo = new QLabel("Identifique-se para acessar seus dados.");
    subtitulo->setStyleSheet("font-size: 12px;");
    subtitulo->setProperty("secondary", true);
    subtitulo->setAlignment(Qt::AlignCenter);
    subtitulo->setWordWrap(true);
    root->addWidget(subtitulo);

    root->addSpacing(6);

    // ── Campo nome ────────────────────────────────────────────────────────────
    auto *labelNome = new QLabel("Nome");
    labelNome->setStyleSheet("font-size: 12px; font-weight: 500;");
    root->addWidget(labelNome);

    m_nome = new QLineEdit;
    m_nome->setPlaceholderText("Seu nome ou identificador");
    m_nome->setMinimumHeight(38);
    root->addWidget(m_nome);

    // ── Campo senha ───────────────────────────────────────────────────────────
    auto *labelSenha = new QLabel("Senha");
    labelSenha->setStyleSheet("font-size: 12px; font-weight: 500;");
    root->addWidget(labelSenha);

    auto *senhaRow = new QHBoxLayout;
    senhaRow->setSpacing(8);

    m_senha = new QLineEdit;
    m_senha->setEchoMode(QLineEdit::Password);
    m_senha->setPlaceholderText("Mínimo 8 caracteres");
    m_senha->setMinimumHeight(38);
    senhaRow->addWidget(m_senha);

    m_btnMostrar = new QPushButton("👁");
    m_btnMostrar->setFixedSize(38, 38);
    m_btnMostrar->setCheckable(true);
    m_btnMostrar->setToolTip("Mostrar/ocultar senha");
    m_btnMostrar->setStyleSheet(
        "QPushButton { background: transparent; border: 1px solid #D0CEC8;"
        " border-radius: 6px; font-size: 14px; padding: 0; }"
        "QPushButton:hover { background: rgba(0,0,0,0.05); }"
        "QPushButton:checked { background: rgba(0,0,0,0.08); }"
    );
    connect(m_btnMostrar, &QPushButton::toggled, this, [this](bool vis) {
        m_senha->setEchoMode(vis ? QLineEdit::Normal : QLineEdit::Password);
    });
    senhaRow->addWidget(m_btnMostrar);
    root->addLayout(senhaRow);

    // Barra de força
    m_barraForca = new QProgressBar;
    m_barraForca->setRange(0, 5);
    m_barraForca->setValue(0);
    m_barraForca->setTextVisible(false);
    m_barraForca->setFixedHeight(4);
    m_barraForca->setStyleSheet(
        "QProgressBar { border: none; border-radius: 2px; background: #E0DED8; }"
        "QProgressBar::chunk { border-radius: 2px; background: #E0DED8; }"
    );
    root->addWidget(m_barraForca);

    m_labelForca = new QLabel(" ");
    m_labelForca->setStyleSheet("font-size: 11px; color: #9A9895;");
    root->addWidget(m_labelForca);

    // ── Checklist de requisitos ────────────────────────────────────────────────
    auto *reqBox = new QWidget;
    reqBox->setObjectName("reqBox");
    reqBox->setStyleSheet("#reqBox { border: 1px solid #E0DED8; border-radius: 8px; }");

    auto *reqLayout = new QVBoxLayout(reqBox);
    reqLayout->setSpacing(4);
    reqLayout->setContentsMargins(12, 10, 12, 10);

    m_reqTamanho.label   = makeReqLabel("Mínimo 8 caracteres");
    m_reqMaiuscula.label = makeReqLabel("Pelo menos uma maiúscula (A–Z)");
    m_reqMinuscula.label = makeReqLabel("Pelo menos uma minúscula (a–z)");
    m_reqNumero.label    = makeReqLabel("Pelo menos um número (0–9)");
    m_reqEspecial.label  = makeReqLabel("Pelo menos um símbolo (!@#$%...)");

    reqLayout->addWidget(m_reqTamanho.label);
    reqLayout->addWidget(m_reqMaiuscula.label);
    reqLayout->addWidget(m_reqMinuscula.label);
    reqLayout->addWidget(m_reqNumero.label);
    reqLayout->addWidget(m_reqEspecial.label);

    root->addWidget(reqBox);

    // ── Botão Entrar ──────────────────────────────────────────────────────────
    m_btnOk = new QPushButton("Entrar");
    m_btnOk->setEnabled(false);
    m_btnOk->setMinimumHeight(42);
    m_btnOk->setStyleSheet(
        "QPushButton { font-size: 14px; font-weight: 600; border-radius: 8px; }"
    );
    connect(m_btnOk, &QPushButton::clicked, this, &QDialog::accept);
    root->addWidget(m_btnOk);

    // ── Sinais ────────────────────────────────────────────────────────────────
    connect(m_nome,  &QLineEdit::textChanged, this, [this](const QString &) { atualizarBotao(); });
    connect(m_senha, &QLineEdit::textChanged, this, &PasswordDialog::onSenhaChanged);
    m_nome->setFocus();

    adjustSize();
}

// ── Getters ───────────────────────────────────────────────────────────────────

QString PasswordDialog::nome()  const { return m_nome->text().trimmed(); }
QString PasswordDialog::senha() const { return m_senha->text(); }

// ── Lógica de validação ───────────────────────────────────────────────────────

void PasswordDialog::atualizarRequisito(Requisito &req, bool ok, bool ativo)
{
    req.ok = ok;
    const QString texto = req.label->text().mid(2); // remove prefixo de 2 chars

    if (!ativo) {
        req.label->setText("· " + texto);
        req.label->setStyleSheet("font-size: 12px; color: #9A9895;");
    } else {
        req.label->setText((ok ? "✓ " : "✗ ") + texto);
        req.label->setStyleSheet(ok
            ? "font-size: 12px; color: #166F4A;"
            : "font-size: 12px; color: #C94040;"
        );
    }
}

void PasswordDialog::atualizarBotao()
{
    const bool nomeOk  = !m_nome->text().trimmed().isEmpty();
    const bool senhaOk = m_reqTamanho.ok && m_reqMaiuscula.ok
                      && m_reqMinuscula.ok && m_reqNumero.ok && m_reqEspecial.ok;
    m_btnOk->setEnabled(nomeOk && senhaOk);
}

int PasswordDialog::calcularForca(const QString &senha) const
{
    int p = 0;
    if (senha.length() >= 8)                                               p++;
    if (senha.contains(QRegularExpression("[A-Z]")))                       p++;
    if (senha.contains(QRegularExpression("[a-z]")))                       p++;
    if (senha.contains(QRegularExpression("[0-9]")))                       p++;
    if (senha.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]]")))   p++;
    return p;
}

void PasswordDialog::onSenhaChanged(const QString &senha)
{
    const bool ativo   = !senha.isEmpty();
    const bool tamanho  = senha.length() >= 8;
    const bool maiusc   = senha.contains(QRegularExpression("[A-Z]"));
    const bool minusc   = senha.contains(QRegularExpression("[a-z]"));
    const bool numero   = senha.contains(QRegularExpression("[0-9]"));
    const bool especial = senha.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]]"));

    atualizarRequisito(m_reqTamanho,   tamanho,  ativo);
    atualizarRequisito(m_reqMaiuscula, maiusc,   ativo);
    atualizarRequisito(m_reqMinuscula, minusc,   ativo);
    atualizarRequisito(m_reqNumero,    numero,   ativo);
    atualizarRequisito(m_reqEspecial,  especial, ativo);

    const int forca = calcularForca(senha);
    m_barraForca->setValue(forca);

    if (!ativo) {
        m_barraForca->setStyleSheet(
            "QProgressBar { border: none; border-radius: 2px; background: #E0DED8; }"
            "QProgressBar::chunk { border-radius: 2px; background: #E0DED8; }"
        );
        m_labelForca->setText(" ");
        m_labelForca->setStyleSheet("font-size: 11px; color: #9A9895;");
    } else {
        QString cor, textoForca;
        switch (forca) {
            case 0: case 1: cor = "#C94040"; textoForca = "Muito fraca";  break;
            case 2:         cor = "#E07B30"; textoForca = "Fraca";        break;
            case 3:         cor = "#D4A017"; textoForca = "Razoável";     break;
            case 4:         cor = "#2D9B6E"; textoForca = "Boa";          break;
            default:        cor = "#166F4A"; textoForca = "Forte";        break;
        }
        m_barraForca->setStyleSheet(QString(
            "QProgressBar { border: none; border-radius: 2px; background: #E0DED8; }"
            "QProgressBar::chunk { border-radius: 2px; background: %1; }"
        ).arg(cor));
        m_labelForca->setText(textoForca);
        m_labelForca->setStyleSheet(QString("font-size: 11px; color: %1;").arg(cor));
    }

    atualizarBotao();
}
