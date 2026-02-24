#include "ui/PasswordDialog.h"

#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QVBoxLayout>

// ── Helpers internos ──────────────────────────────────────────────────────────

static QLabel *makeReqLabel(const QString &texto)
{
    auto *l = new QLabel("· " + texto);
    l->setStyleSheet("font-size: 12px;");
    return l;
}

static QFrame *makeSeparator()
{
    auto *f = new QFrame;
    f->setFrameShape(QFrame::HLine);
    f->setStyleSheet("color: #E0DED8;");
    return f;
}

// ── Construtor ────────────────────────────────────────────────────────────────

PasswordDialog::PasswordDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Orçamento Pessoal — Conexão");
    setFixedWidth(400);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(16);
    root->setContentsMargins(28, 28, 28, 28);

    // ── Título ────────────────────────────────────────────────────────────────
    auto *titulo = new QLabel("Conectar ao banco de dados");
    titulo->setStyleSheet("font-size: 16px; font-weight: 600;");
    root->addWidget(titulo);

    auto *subtitulo = new QLabel("Informe as credenciais do MySQL e a senha do app.");
    subtitulo->setStyleSheet("font-size: 12px;");
    subtitulo->setProperty("secondary", true);
    subtitulo->setWordWrap(true);
    root->addWidget(subtitulo);

    root->addWidget(makeSeparator());

    // ── Formulário de conexão ─────────────────────────────────────────────────
    auto *form = new QFormLayout;
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto labelStyle = "font-size: 12px; font-weight: 500;";

    m_host = new QLineEdit("localhost");
    auto *lHost = new QLabel("Host"); lHost->setStyleSheet(labelStyle);
    form->addRow(lHost, m_host);

    auto *portaLayout = new QHBoxLayout;
    m_porta = new QLineEdit("3306");
    m_porta->setValidator(new QIntValidator(1, 65535, this));
    m_porta->setFixedWidth(80);
    portaLayout->addWidget(m_porta);
    portaLayout->addStretch();
    auto *lPorta = new QLabel("Porta"); lPorta->setStyleSheet(labelStyle);
    form->addRow(lPorta, portaLayout);

    m_banco = new QLineEdit("orcamento");
    auto *lBanco = new QLabel("Banco"); lBanco->setStyleSheet(labelStyle);
    form->addRow(lBanco, m_banco);

    m_usuario = new QLineEdit("root");
    auto *lUser = new QLabel("Usuário"); lUser->setStyleSheet(labelStyle);
    form->addRow(lUser, m_usuario);

    root->addLayout(form);

    root->addWidget(makeSeparator());

    // ── Campo senha ───────────────────────────────────────────────────────────
    auto *labelSenha = new QLabel("Senha do app");
    labelSenha->setStyleSheet("font-size: 12px; font-weight: 500;");
    root->addWidget(labelSenha);

    auto *senhaRow = new QHBoxLayout;
    m_senha = new QLineEdit;
    m_senha->setEchoMode(QLineEdit::Password);
    m_senha->setPlaceholderText("Mínimo 8 caracteres");
    senhaRow->addWidget(m_senha);

    m_btnMostrar = new QPushButton("👁");
    m_btnMostrar->setFixedSize(36, 36);
    m_btnMostrar->setCheckable(true);
    m_btnMostrar->setStyleSheet(
        "QPushButton { background: transparent; border: 1px solid #E0DED8;"
        " border-radius: 6px; font-size: 14px; padding: 0; }"
        "QPushButton:hover { background: rgba(0,0,0,0.05); }"
    );
    connect(m_btnMostrar, &QPushButton::toggled, this, [this](bool vis) {
        m_senha->setEchoMode(vis ? QLineEdit::Normal : QLineEdit::Password);
    });
    senhaRow->addWidget(m_btnMostrar);
    root->addLayout(senhaRow);

    // Barra de força
    m_barraForca = new QProgressBar;
    m_barraForca->setRange(0, 4);
    m_barraForca->setValue(0);
    m_barraForca->setTextVisible(false);
    m_barraForca->setFixedHeight(4);
    m_barraForca->setStyleSheet(
        "QProgressBar { border: none; border-radius: 2px; background: #E0DED8; }"
        "QProgressBar::chunk { border-radius: 2px; background: #C94040; }"
    );
    root->addWidget(m_barraForca);

    m_labelForca = new QLabel("Digite uma senha forte");
    m_labelForca->setStyleSheet("font-size: 11px; color: #9A9895;");
    root->addWidget(m_labelForca);

    // ── Requisitos ────────────────────────────────────────────────────────────
    auto *grpRequisitos = new QWidget;
    grpRequisitos->setStyleSheet(
        "background: transparent; border: 1px solid #E0DED8; border-radius: 8px; padding: 4px;"
    );
    auto *reqLayout = new QVBoxLayout(grpRequisitos);
    reqLayout->setSpacing(4);
    reqLayout->setContentsMargins(12, 10, 12, 10);

    m_reqTamanho.label  = makeReqLabel("Mínimo 8 caracteres");
    m_reqMaiuscula.label = makeReqLabel("Pelo menos uma maiúscula");
    m_reqMinuscula.label = makeReqLabel("Pelo menos uma minúscula");
    m_reqNumero.label   = makeReqLabel("Pelo menos um número");
    m_reqEspecial.label = makeReqLabel("Pelo menos um símbolo (!@#$%^&*)");

    reqLayout->addWidget(m_reqTamanho.label);
    reqLayout->addWidget(m_reqMaiuscula.label);
    reqLayout->addWidget(m_reqMinuscula.label);
    reqLayout->addWidget(m_reqNumero.label);
    reqLayout->addWidget(m_reqEspecial.label);

    root->addWidget(grpRequisitos);

    // ── Botão OK ──────────────────────────────────────────────────────────────
    m_btnOk = new QPushButton("Conectar");
    m_btnOk->setEnabled(false);
    m_btnOk->setMinimumHeight(40);
    m_btnOk->setStyleSheet(
        "QPushButton { font-size: 14px; font-weight: 600; border-radius: 8px; }"
    );
    connect(m_btnOk, &QPushButton::clicked, this, &QDialog::accept);
    root->addWidget(m_btnOk);

    // ── Sinais ────────────────────────────────────────────────────────────────
    connect(m_senha, &QLineEdit::textChanged, this, &PasswordDialog::onSenhaChanged);
    m_senha->setFocus();

    adjustSize();
}

// ── Getters ───────────────────────────────────────────────────────────────────

QString PasswordDialog::host()    const { return m_host->text().trimmed(); }
int     PasswordDialog::porta()   const { return m_porta->text().toInt(); }
QString PasswordDialog::banco()   const { return m_banco->text().trimmed(); }
QString PasswordDialog::usuario() const { return m_usuario->text().trimmed(); }
QString PasswordDialog::senha()   const { return m_senha->text(); }

// ── Lógica de validação ───────────────────────────────────────────────────────

void PasswordDialog::atualizarRequisito(Requisito &req, bool ok)
{
    req.ok = ok;
    QString texto = req.label->text().mid(2); // remove "· " ou "✓ "
    req.label->setText((ok ? "✓ " : "· ") + texto);
    req.label->setStyleSheet(ok
        ? "font-size: 12px; color: #166F4A;"
        : "font-size: 12px; color: #9A9895;"
    );
}

int PasswordDialog::calcularForca(const QString &senha) const
{
    int pontos = 0;
    if (senha.length() >= 8)                                          pontos++;
    if (senha.contains(QRegularExpression("[A-Z]")))                  pontos++;
    if (senha.contains(QRegularExpression("[a-z]")))                  pontos++;
    if (senha.contains(QRegularExpression("[0-9]")))                  pontos++;
    if (senha.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]]"))) pontos++;
    return pontos;
}

void PasswordDialog::onSenhaChanged(const QString &senha)
{
    const bool tamanho  = senha.length() >= 8;
    const bool maiusc   = senha.contains(QRegularExpression("[A-Z]"));
    const bool minusc   = senha.contains(QRegularExpression("[a-z]"));
    const bool numero   = senha.contains(QRegularExpression("[0-9]"));
    const bool especial = senha.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]]"));

    atualizarRequisito(m_reqTamanho,  tamanho);
    atualizarRequisito(m_reqMaiuscula, maiusc);
    atualizarRequisito(m_reqMinuscula, minusc);
    atualizarRequisito(m_reqNumero,   numero);
    atualizarRequisito(m_reqEspecial, especial);

    const int forca = calcularForca(senha);
    m_barraForca->setValue(forca);

    // Cor da barra conforme força
    QString cor;
    QString textoForca;
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
    m_labelForca->setText(senha.isEmpty() ? "Digite uma senha forte" : textoForca);
    m_labelForca->setStyleSheet(QString("font-size: 11px; color: %1;").arg(forca >= 3 ? cor : "#9A9895"));

    const bool valido = tamanho && maiusc && minusc && numero && especial;
    m_btnOk->setEnabled(valido);
}
