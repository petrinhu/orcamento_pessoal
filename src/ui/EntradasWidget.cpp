#include "ui/EntradasWidget.h"

#include "core/DatabaseManager.h"
#include "models/Entrada.h"
#include "utils/CurrencyUtils.h"

#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QVBoxLayout>

static constexpr int COL_DATA   = 0;
static constexpr int COL_ORIGEM = 1;
static constexpr int COL_VALOR  = 2;
static constexpr int ID_ROLE    = Qt::UserRole;

// ── Helpers de item ───────────────────────────────────────────────────────────

static QTableWidgetItem *makeItem(const QString &texto, bool editavel = true)
{
    auto *item = new QTableWidgetItem(texto);
    if (!editavel)
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

static QTableWidgetItem *makeTotalItem(const QString &texto)
{
    auto *item = makeItem(texto, false);
    item->setFont(QFont("Inter", 13, QFont::DemiBold));
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

// ── Construtor ────────────────────────────────────────────────────────────────

EntradasWidget::EntradasWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 16);
    root->setSpacing(12);

    // ── Tabela ────────────────────────────────────────────────────────────────
    m_tabela = new QTableWidget(0, 3, this);
    m_tabela->setHorizontalHeaderLabels({"Data", "Origem / Descrição", "Valor"});
    m_tabela->horizontalHeader()->setSectionResizeMode(COL_DATA,   QHeaderView::ResizeToContents);
    m_tabela->horizontalHeader()->setSectionResizeMode(COL_ORIGEM, QHeaderView::Stretch);
    m_tabela->horizontalHeader()->setSectionResizeMode(COL_VALOR,  QHeaderView::ResizeToContents);
    m_tabela->verticalHeader()->setVisible(false);
    m_tabela->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabela->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tabela->setAlternatingRowColors(true);
    m_tabela->setShowGrid(false);
    root->addWidget(m_tabela);

    // ── Botões ────────────────────────────────────────────────────────────────
    auto *btnAdicionar = new QPushButton("+ Adicionar entrada");
    auto *btnRemover   = new QPushButton("Remover selecionada");
    btnRemover->setProperty("secondary", true);

    auto *botoesRow = new QHBoxLayout;
    botoesRow->setSpacing(8);
    botoesRow->addWidget(btnAdicionar);
    botoesRow->addWidget(btnRemover);
    botoesRow->addStretch();
    root->addLayout(botoesRow);

    connect(btnAdicionar, &QPushButton::clicked, this, &EntradasWidget::adicionarEntrada);
    connect(btnRemover,   &QPushButton::clicked, this, &EntradasWidget::removerEntrada);
    connect(m_tabela, &QTableWidget::itemChanged, this, &EntradasWidget::onItemChanged);

    carregar();
}

// ── Carga inicial ─────────────────────────────────────────────────────────────

void EntradasWidget::carregar()
{
    m_carregando = true;
    m_tabela->setRowCount(0);

    for (const Entrada &e : DatabaseManager::instance().listarEntradas())
        adicionarLinha(e.id, e.data, e.origem, e.valorCentavos);

    inserirLinhaTotalVazia();
    atualizarTotal();
    m_carregando = false;
}

void EntradasWidget::adicionarLinha(int id, const QDate &data,
                                     const QString &origem, qint64 valorCentavos)
{
    const int row = m_tabela->rowCount();
    m_tabela->insertRow(row);
    m_tabela->setRowHeight(row, 36);

    auto *itemData = makeItem(data.toString("dd/MM/yyyy"));
    itemData->setData(ID_ROLE, id);  // armazena o id do banco nesta célula
    m_tabela->setItem(row, COL_DATA,   itemData);
    m_tabela->setItem(row, COL_ORIGEM, makeItem(origem));
    auto *itemValor = makeItem(centavosParaTexto(valorCentavos));
    itemValor->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tabela->setItem(row, COL_VALOR, itemValor);
}

void EntradasWidget::inserirLinhaTotalVazia()
{
    const int row = m_tabela->rowCount();
    m_tabela->insertRow(row);
    m_tabela->setRowHeight(row, 36);

    auto *labelTotal = makeTotalItem("TOTAL");
    labelTotal->setData(ID_ROLE, -1);  // id = -1 marca a linha de total
    m_tabela->setItem(row, COL_DATA,   labelTotal);
    m_tabela->setItem(row, COL_ORIGEM, makeItem("", false));
    auto *itemTotal = makeTotalItem("R$ 0,00");
    m_tabela->setItem(row, COL_VALOR, itemTotal);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

int EntradasWidget::idDaLinha(int row) const
{
    auto *item = m_tabela->item(row, COL_DATA);
    return item ? item->data(ID_ROLE).toInt() : -1;
}

bool EntradasWidget::isTotalRow(int row) const
{
    return idDaLinha(row) == -1;
}

void EntradasWidget::atualizarTotal()
{
    const int totalRow = m_tabela->rowCount() - 1;
    qint64 total = 0;
    for (int r = 0; r < totalRow; ++r)
        total += textoParaCentavos(m_tabela->item(r, COL_VALOR)->text());

    m_carregando = true;
    m_tabela->item(totalRow, COL_VALOR)->setText(centavosParaTexto(total));
    m_carregando = false;
}

// ── Adicionar ─────────────────────────────────────────────────────────────────

void EntradasWidget::adicionarEntrada()
{
    Entrada e;
    e.data          = QDate::currentDate();
    e.origem        = "Entrada";
    e.valorCentavos = 0;

    if (!DatabaseManager::instance().inserirEntrada(e))
        return;

    m_carregando = true;
    // Insere antes da linha de total
    const int totalRow = m_tabela->rowCount() - 1;
    m_tabela->insertRow(totalRow);
    m_tabela->setRowHeight(totalRow, 36);

    auto *itemData = makeItem(e.data.toString("dd/MM/yyyy"));
    itemData->setData(ID_ROLE, e.id);
    m_tabela->setItem(totalRow, COL_DATA,   itemData);
    m_tabela->setItem(totalRow, COL_ORIGEM, makeItem(e.origem));
    auto *itemValor = makeItem(centavosParaTexto(e.valorCentavos));
    itemValor->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tabela->setItem(totalRow, COL_VALOR,  itemValor);
    m_carregando = false;

    atualizarTotal();
    m_tabela->selectRow(totalRow);
    m_tabela->scrollToItem(m_tabela->item(totalRow, COL_ORIGEM));
    emit dadosAlterados();
}

// ── Remover ───────────────────────────────────────────────────────────────────

void EntradasWidget::removerEntrada()
{
    const int row = m_tabela->currentRow();
    if (row < 0 || isTotalRow(row)) {
        QMessageBox::information(this, "Remover", "Selecione uma entrada para remover.");
        return;
    }

    const auto resp = QMessageBox::question(
        this, "Confirmar remoção",
        "Remover a entrada selecionada?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (resp != QMessageBox::Yes) return;

    const int id = idDaLinha(row);
    if (!DatabaseManager::instance().removerEntrada(id)) return;

    m_carregando = true;
    m_tabela->removeRow(row);
    m_carregando = false;

    atualizarTotal();
    emit dadosAlterados();
}

// ── Edição inline ─────────────────────────────────────────────────────────────

void EntradasWidget::onItemChanged(QTableWidgetItem *item)
{
    if (m_carregando || !item) return;
    const int row = item->row();
    if (isTotalRow(row)) return;

    const int id = idDaLinha(row);
    if (id <= 0) return;

    // Reformata valor ao editar
    if (item->column() == COL_VALOR) {
        m_carregando = true;
        item->setText(centavosParaTexto(textoParaCentavos(item->text())));
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_carregando = false;
    }

    // Monta Entrada e salva no banco
    Entrada e;
    e.id            = id;
    e.data          = QDate::fromString(
                          m_tabela->item(row, COL_DATA)->text(), "dd/MM/yyyy");
    e.origem        = m_tabela->item(row, COL_ORIGEM)->text();
    e.valorCentavos = textoParaCentavos(m_tabela->item(row, COL_VALOR)->text());

    DatabaseManager::instance().atualizarEntrada(e);
    atualizarTotal();
    emit dadosAlterados();
}
