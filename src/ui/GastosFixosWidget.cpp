#include "ui/GastosFixosWidget.h"

#include "core/DatabaseManager.h"
#include "utils/CurrencyUtils.h"

#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QVBoxLayout>

static constexpr int COL_DATA  = 0;
static constexpr int COL_HIST  = 1;
static constexpr int COL_VALOR = 2;
static constexpr int COL_CAT   = 3;
static constexpr int ID_ROLE   = Qt::UserRole;
static constexpr int CATID_ROLE = Qt::UserRole + 1; // id da categoria no combo

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

GastosFixosWidget::GastosFixosWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 16);
    root->setSpacing(12);

    m_tabela = new QTableWidget(0, 4, this);
    m_tabela->setHorizontalHeaderLabels({"Data", "Histórico", "Valor", "Categoria"});
    m_tabela->horizontalHeader()->setSectionResizeMode(COL_DATA,  QHeaderView::ResizeToContents);
    m_tabela->horizontalHeader()->setSectionResizeMode(COL_HIST,  QHeaderView::Stretch);
    m_tabela->horizontalHeader()->setSectionResizeMode(COL_VALOR, QHeaderView::ResizeToContents);
    m_tabela->horizontalHeader()->setSectionResizeMode(COL_CAT,   QHeaderView::ResizeToContents);
    m_tabela->verticalHeader()->setVisible(false);
    m_tabela->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabela->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tabela->setAlternatingRowColors(true);
    m_tabela->setShowGrid(false);
    root->addWidget(m_tabela);

    auto *btnAdicionar = new QPushButton("+ Adicionar gasto fixo");
    auto *btnRemover   = new QPushButton("Remover selecionado");
    auto *btnRepetir   = new QPushButton("Repetir mês anterior");
    btnRemover->setProperty("secondary", true);
    btnRepetir->setProperty("secondary", true);

    auto *botoesRow = new QHBoxLayout;
    botoesRow->setSpacing(8);
    botoesRow->addWidget(btnAdicionar);
    botoesRow->addWidget(btnRemover);
    botoesRow->addWidget(btnRepetir);
    botoesRow->addStretch();
    root->addLayout(botoesRow);

    connect(btnAdicionar, &QPushButton::clicked, this, &GastosFixosWidget::adicionarGasto);
    connect(btnRemover,   &QPushButton::clicked, this, &GastosFixosWidget::removerGasto);
    connect(btnRepetir,   &QPushButton::clicked, this, &GastosFixosWidget::repetirMesAnterior);
    connect(m_tabela, &QTableWidget::itemChanged, this, &GastosFixosWidget::onItemChanged);

    carregar();
}

// ── Carga ─────────────────────────────────────────────────────────────────────

void GastosFixosWidget::carregar()
{
    m_carregando = true;
    m_tabela->setRowCount(0);
    m_categorias = DatabaseManager::instance().listarCategorias();

    for (const GastoFixo &g : DatabaseManager::instance().listarGastosFixos())
        adicionarLinha(g);

    inserirLinhaTotalVazia();
    atualizarTotal();
    m_carregando = false;
}

void GastosFixosWidget::recarregarCategorias()
{
    carregar();
}

void GastosFixosWidget::adicionarLinha(const GastoFixo &g)
{
    const int row = m_tabela->rowCount();
    m_tabela->insertRow(row);
    m_tabela->setRowHeight(row, 36);

    auto *itemData = makeItem(g.data.toString("dd/MM/yyyy"));
    itemData->setData(ID_ROLE, g.id);
    m_tabela->setItem(row, COL_DATA,  itemData);
    m_tabela->setItem(row, COL_HIST,  makeItem(g.historico));

    auto *itemValor = makeItem(centavosParaTexto(g.valorCentavos));
    itemValor->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tabela->setItem(row, COL_VALOR, itemValor);

    // Combo de categorias
    auto *combo = new QComboBox;
    for (const Categoria &cat : m_categorias)
        combo->addItem(cat.nome, cat.id);  // id no UserData

    const int idx = combo->findData(g.categoriaId);
    combo->setCurrentIndex(idx >= 0 ? idx : 0);

    m_tabela->setCellWidget(row, COL_CAT, combo);
    conectarCombo(combo, row);
}

void GastosFixosWidget::inserirLinhaTotalVazia()
{
    const int row = m_tabela->rowCount();
    m_tabela->insertRow(row);
    m_tabela->setRowHeight(row, 36);

    auto *labelTotal = makeTotalItem("TOTAL");
    labelTotal->setData(ID_ROLE, -1);
    m_tabela->setItem(row, COL_DATA,  labelTotal);
    m_tabela->setItem(row, COL_HIST,  makeItem("", false));
    m_tabela->setItem(row, COL_VALOR, makeTotalItem("R$ 0,00"));
    m_tabela->setItem(row, COL_CAT,   makeItem("", false));
}

// ── Helpers ───────────────────────────────────────────────────────────────────

int GastosFixosWidget::idDaLinha(int row) const
{
    auto *item = m_tabela->item(row, COL_DATA);
    return item ? item->data(ID_ROLE).toInt() : -1;
}

bool GastosFixosWidget::isTotalRow(int row) const
{
    return idDaLinha(row) == -1;
}

QComboBox *GastosFixosWidget::comboDaLinha(int row) const
{
    return qobject_cast<QComboBox*>(m_tabela->cellWidget(row, COL_CAT));
}

void GastosFixosWidget::atualizarTotal()
{
    const int totalRow = m_tabela->rowCount() - 1;
    qint64 total = 0;
    for (int r = 0; r < totalRow; ++r)
        total += textoParaCentavos(m_tabela->item(r, COL_VALOR)->text());

    m_carregando = true;
    m_tabela->item(totalRow, COL_VALOR)->setText(centavosParaTexto(total));
    m_carregando = false;
}

void GastosFixosWidget::salvarLinha(int row)
{
    if (isTotalRow(row)) return;
    const int id = idDaLinha(row);
    if (id <= 0) return;

    auto *combo = comboDaLinha(row);

    GastoFixo g;
    g.id            = id;
    g.data          = QDate::fromString(m_tabela->item(row, COL_DATA)->text(), "dd/MM/yyyy");
    g.historico     = m_tabela->item(row, COL_HIST)->text();
    g.valorCentavos = textoParaCentavos(m_tabela->item(row, COL_VALOR)->text());
    g.categoriaId   = combo ? combo->currentData().toInt() : 0;

    DatabaseManager::instance().atualizarGastoFixo(g);
}

void GastosFixosWidget::conectarCombo(QComboBox *combo, int row)
{
    connect(combo, &QComboBox::currentIndexChanged, this, [this, row]() {
        if (m_carregando) return;
        salvarLinha(row);
        emit dadosAlterados();
    });
}

// ── Adicionar ─────────────────────────────────────────────────────────────────

void GastosFixosWidget::adicionarGasto()
{
    if (m_categorias.isEmpty()) {
        QMessageBox::warning(this, "Sem categorias",
            "Cadastre ao menos uma categoria em Configurações.");
        return;
    }

    GastoFixo g;
    g.data          = QDate::currentDate();
    g.historico     = "";
    g.valorCentavos = 0;
    g.categoriaId   = m_categorias.first().id;

    if (!DatabaseManager::instance().inserirGastoFixo(g)) return;

    m_carregando = true;
    const int totalRow = m_tabela->rowCount() - 1;
    m_tabela->insertRow(totalRow);
    m_tabela->setRowHeight(totalRow, 36);

    auto *itemData = makeItem(g.data.toString("dd/MM/yyyy"));
    itemData->setData(ID_ROLE, g.id);
    m_tabela->setItem(totalRow, COL_DATA,  itemData);
    m_tabela->setItem(totalRow, COL_HIST,  makeItem(g.historico));

    auto *itemValor = makeItem(centavosParaTexto(g.valorCentavos));
    itemValor->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tabela->setItem(totalRow, COL_VALOR, itemValor);

    auto *combo = new QComboBox;
    for (const Categoria &cat : m_categorias)
        combo->addItem(cat.nome, cat.id);
    m_tabela->setCellWidget(totalRow, COL_CAT, combo);
    m_carregando = false;

    conectarCombo(combo, totalRow);
    atualizarTotal();
    m_tabela->selectRow(totalRow);
    m_tabela->scrollToItem(m_tabela->item(totalRow, COL_HIST));
    emit dadosAlterados();
}

// ── Remover ───────────────────────────────────────────────────────────────────

void GastosFixosWidget::removerGasto()
{
    const int row = m_tabela->currentRow();
    if (row < 0 || isTotalRow(row)) {
        QMessageBox::information(this, "Remover", "Selecione um gasto para remover.");
        return;
    }

    const auto resp = QMessageBox::question(
        this, "Confirmar remoção", "Remover o gasto fixo selecionado?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (resp != QMessageBox::Yes) return;

    if (!DatabaseManager::instance().removerGastoFixo(idDaLinha(row))) return;

    m_carregando = true;
    m_tabela->removeRow(row);
    m_carregando = false;

    atualizarTotal();
    emit dadosAlterados();
}

// ── Repetir mês anterior ──────────────────────────────────────────────────────

void GastosFixosWidget::repetirMesAnterior()
{
    const QDate hoje        = QDate::currentDate();
    const QDate mesAnterior = hoje.addMonths(-1);

    QList<GastoFixo> paraRepetir;
    const int totalRow = m_tabela->rowCount() - 1;

    for (int r = 0; r < totalRow; ++r) {
        const QDate data = QDate::fromString(
            m_tabela->item(r, COL_DATA)->text(), "dd/MM/yyyy");
        if (data.month() == mesAnterior.month() && data.year() == mesAnterior.year()) {
            GastoFixo g;
            g.data          = hoje;
            g.historico     = m_tabela->item(r, COL_HIST)->text();
            g.valorCentavos = 0;  // valor em aberto, usuário preenche
            auto *combo     = comboDaLinha(r);
            g.categoriaId   = combo ? combo->currentData().toInt()
                                    : m_categorias.first().id;
            paraRepetir.append(g);
        }
    }

    if (paraRepetir.isEmpty()) {
        QMessageBox::information(this, "Repetir mês anterior",
            "Nenhum gasto fixo encontrado no mês anterior.");
        return;
    }

    m_carregando = true;
    for (GastoFixo &g : paraRepetir) {
        if (!DatabaseManager::instance().inserirGastoFixo(g)) continue;

        const int row = m_tabela->rowCount() - 1; // antes da linha total
        m_tabela->insertRow(row);
        m_tabela->setRowHeight(row, 36);

        auto *itemData = makeItem(g.data.toString("dd/MM/yyyy"));
        itemData->setData(ID_ROLE, g.id);
        m_tabela->setItem(row, COL_DATA,  itemData);
        m_tabela->setItem(row, COL_HIST,  makeItem(g.historico));

        auto *itemValor = makeItem(centavosParaTexto(g.valorCentavos));
        itemValor->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tabela->setItem(row, COL_VALOR, itemValor);

        auto *combo = new QComboBox;
        for (const Categoria &cat : m_categorias)
            combo->addItem(cat.nome, cat.id);
        combo->setCurrentIndex(combo->findData(g.categoriaId));
        m_tabela->setCellWidget(row, COL_CAT, combo);
        conectarCombo(combo, row);
    }
    m_carregando = false;

    atualizarTotal();
    emit dadosAlterados();

    QMessageBox::information(this, "Repetir mês anterior",
        QString("%1 gasto(s) repetido(s) com valor em aberto.")
            .arg(paraRepetir.size()));
}

// ── Edição inline ─────────────────────────────────────────────────────────────

void GastosFixosWidget::onItemChanged(QTableWidgetItem *item)
{
    if (m_carregando || !item) return;
    const int row = item->row();
    if (isTotalRow(row)) return;

    if (item->column() == COL_VALOR) {
        m_carregando = true;
        item->setText(centavosParaTexto(textoParaCentavos(item->text())));
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_carregando = false;
    }

    salvarLinha(row);
    atualizarTotal();
    emit dadosAlterados();
}
