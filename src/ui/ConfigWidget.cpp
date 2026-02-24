#include "ui/ConfigWidget.h"

#include "core/DatabaseManager.h"
#include "models/Categoria.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

ConfigWidget::ConfigWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ── Título ────────────────────────────────────────────────────────────────
    auto *titulo = new QLabel("Categorias de gastos");
    titulo->setStyleSheet("font-size: 15px; font-weight: 600;");
    root->addWidget(titulo);

    auto *subtitulo = new QLabel(
        "Categorias são compartilhadas entre Gastos Fixos e Gastos Variáveis.");
    subtitulo->setProperty("secondary", true);
    subtitulo->setStyleSheet("font-size: 12px; color: palette(mid);");
    subtitulo->setWordWrap(true);
    root->addWidget(subtitulo);

    // ── Lista ─────────────────────────────────────────────────────────────────
    m_lista = new QListWidget;
    m_lista->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    root->addWidget(m_lista);

    // ── Input + botão Adicionar ───────────────────────────────────────────────
    auto *addRow = new QHBoxLayout;
    addRow->setSpacing(8);

    m_inputNova = new QLineEdit;
    m_inputNova->setPlaceholderText("Nome da nova categoria…");
    addRow->addWidget(m_inputNova);

    auto *btnAdicionar = new QPushButton("Adicionar");
    btnAdicionar->setFixedWidth(100);
    addRow->addWidget(btnAdicionar);

    root->addLayout(addRow);

    // ── Botão Remover ─────────────────────────────────────────────────────────
    auto *btnRemover = new QPushButton("Remover selecionada");
    btnRemover->setProperty("secondary", true);
    root->addWidget(btnRemover);

    root->addStretch();

    connect(btnAdicionar,  &QPushButton::clicked,  this, &ConfigWidget::adicionarCategoria);
    connect(m_inputNova,   &QLineEdit::returnPressed, this, &ConfigWidget::adicionarCategoria);
    connect(btnRemover,    &QPushButton::clicked,  this, &ConfigWidget::removerCategoria);

    carregar();
}

// ── Carga ─────────────────────────────────────────────────────────────────────

void ConfigWidget::carregar()
{
    m_lista->clear();
    for (const Categoria &cat : DatabaseManager::instance().listarCategorias()) {
        auto *item = new QListWidgetItem(cat.nome);
        item->setData(Qt::UserRole, cat.id);
        m_lista->addItem(item);
    }
}

// ── Adicionar ─────────────────────────────────────────────────────────────────

void ConfigWidget::adicionarCategoria()
{
    const QString nome = m_inputNova->text().trimmed();
    if (nome.isEmpty()) return;

    Categoria cat;
    cat.nome = nome;

    if (!DatabaseManager::instance().inserirCategoria(cat)) {
        QMessageBox::warning(this, "Erro",
            "Não foi possível adicionar a categoria.\n"
            "Verifique se o nome já existe.");
        return;
    }

    auto *item = new QListWidgetItem(cat.nome);
    item->setData(Qt::UserRole, cat.id);
    m_lista->addItem(item);
    m_lista->setCurrentItem(item);
    m_inputNova->clear();

    emit categoriasAlteradas();
}

// ── Remover ───────────────────────────────────────────────────────────────────

void ConfigWidget::removerCategoria()
{
    auto *item = m_lista->currentItem();
    if (!item) {
        QMessageBox::information(this, "Remover", "Selecione uma categoria para remover.");
        return;
    }

    const auto resp = QMessageBox::question(
        this, "Confirmar remoção",
        QString("Remover a categoria \"%1\"?\n\n"
                "Gastos vinculados a ela também serão removidos.")
            .arg(item->text()),
        QMessageBox::Yes | QMessageBox::No
    );
    if (resp != QMessageBox::Yes) return;

    const int id = item->data(Qt::UserRole).toInt();
    if (!DatabaseManager::instance().removerCategoria(id)) {
        QMessageBox::warning(this, "Erro",
            "Não foi possível remover a categoria.");
        return;
    }

    delete m_lista->takeItem(m_lista->row(item));
    emit categoriasAlteradas();
}
