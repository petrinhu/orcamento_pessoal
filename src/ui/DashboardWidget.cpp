#include "ui/DashboardWidget.h"

#include "core/DatabaseManager.h"
#include "ui/Theme.h"
#include "utils/CurrencyUtils.h"

#include <QChart>
#include <QFrame>
#include <QHBoxLayout>
#include <QPieSeries>
#include <QPieSlice>
#include <QPainter>
#include <QVBoxLayout>

// ── Card de resumo ────────────────────────────────────────────────────────────

QWidget *DashboardWidget::makeCard(const QString &titulo, QLabel *&valorLabel)
{
    auto *card = new QFrame;
    card->setFrameShape(QFrame::NoFrame);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    card->setStyleSheet(
        "QFrame { background-color: palette(base);"
        " border: 1px solid palette(mid);"
        " border-radius: 10px; }"
    );

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(6);

    auto *labelTitulo = new QLabel(titulo);
    labelTitulo->setStyleSheet("font-size: 11px; font-weight: 500; color: palette(mid);");

    valorLabel = new QLabel("R$ 0,00");
    valorLabel->setStyleSheet("font-size: 22px; font-weight: 600;");

    layout->addWidget(labelTitulo);
    layout->addWidget(valorLabel);

    return card;
}

// ── Construtor ────────────────────────────────────────────────────────────────

DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(20);

    // ── Linha de cards ────────────────────────────────────────────────────────
    auto *cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(16);

    cardsRow->addWidget(makeCard("Entradas",         m_valorEntradas));
    cardsRow->addWidget(makeCard("Gastos Fixos",     m_valorFixos));
    cardsRow->addWidget(makeCard("Gastos Variáveis", m_valorVariaveis));

    // Card de saldo — ligeiramente maior visualmente
    QFrame *cardSaldo = new QFrame;
    cardSaldo->setFrameShape(QFrame::NoFrame);
    cardSaldo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    cardSaldo->setStyleSheet(
        "QFrame { background-color: palette(base);"
        " border: 1px solid palette(mid);"
        " border-radius: 10px; }"
    );
    auto *saldoLayout = new QVBoxLayout(cardSaldo);
    saldoLayout->setContentsMargins(20, 16, 20, 16);
    saldoLayout->setSpacing(6);
    auto *saldoTitulo = new QLabel("Saldo Atual");
    saldoTitulo->setStyleSheet("font-size: 11px; font-weight: 500; color: palette(mid);");
    m_valorSaldo = new QLabel("R$ 0,00");
    m_valorSaldo->setStyleSheet("font-size: 26px; font-weight: 600;");
    saldoLayout->addWidget(saldoTitulo);
    saldoLayout->addWidget(m_valorSaldo);

    cardsRow->addWidget(cardSaldo);
    root->addLayout(cardsRow);

    // ── Gráfico ───────────────────────────────────────────────────────────────
    m_chartView = new QChartView;
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chartView->setStyleSheet("border: none; background: transparent;");

    auto *chart = new QChart;
    chart->setTitle("");
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(0, 0, 0, 0));
    m_chartView->setChart(chart);

    root->addWidget(m_chartView);

    atualizar();
}

// ── Atualizar ─────────────────────────────────────────────────────────────────

void DashboardWidget::atualizar()
{
    auto &db = DatabaseManager::instance();

    const qint64 entradas  = db.totalEntradas();
    const qint64 fixos     = db.totalGastosFixos();
    const qint64 variaveis = db.totalGastosVariaveis();
    const qint64 saldo     = entradas - fixos - variaveis;

    m_valorEntradas->setText(centavosParaTexto(entradas));
    m_valorFixos->setText(centavosParaTexto(fixos));
    m_valorVariaveis->setText(centavosParaTexto(variaveis));
    m_valorSaldo->setText(centavosParaTexto(saldo));

    atualizarCores(saldo);

    // ── Gráfico de pizza ──────────────────────────────────────────────────────
    auto *series = new QPieSeries;
    series->setHoleSize(0.40); // donut moderno

    const bool dark = Theme::isDark();
    const QColor corEntradas  = dark ? QColor("#22A367") : QColor("#166F4A");
    const QColor corFixos     = dark ? QColor("#E85555") : QColor("#C94040");
    const QColor corVariaveis = dark ? QColor("#D4A017") : QColor("#B8860B");

    if (entradas > 0) {
        auto *sl = series->append("Entradas",        entradas / 100.0);
        sl->setColor(corEntradas);
        sl->setLabelVisible(true);
    }
    if (fixos > 0) {
        auto *sl = series->append("Gastos Fixos",    fixos / 100.0);
        sl->setColor(corFixos);
        sl->setLabelVisible(true);
    }
    if (variaveis > 0) {
        auto *sl = series->append("Gastos Variáveis", variaveis / 100.0);
        sl->setColor(corVariaveis);
        sl->setLabelVisible(true);
    }

    // Sem dados: placeholder
    if (series->count() == 0) {
        auto *sl = series->append("Sem dados", 1);
        sl->setColor(dark ? QColor("#32323A") : QColor("#E0DED8"));
        sl->setLabelVisible(false);
    }

    for (auto *sl : series->slices()) {
        sl->setLabelColor(dark ? QColor("#F0EFED") : QColor("#18181A"));
        sl->setBorderColor(Qt::transparent);
        sl->setLabelPosition(QPieSlice::LabelOutside);
    }

    auto *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle("Distribuição do período");
    chart->setTitleFont(QFont("Inter", 13, QFont::DemiBold));
    chart->setTitleBrush(dark ? QColor("#F0EFED") : QColor("#18181A"));
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(8, 8, 8, 8));
    chart->legend()->setFont(QFont("Inter", 11));
    chart->legend()->setLabelColor(dark ? QColor("#9A9895") : QColor("#6E6D6A"));
    chart->legend()->setAlignment(Qt::AlignBottom);

    m_chartView->setChart(chart);
}

void DashboardWidget::atualizarCores(qint64 saldo)
{
    const bool dark     = Theme::isDark();
    const QString verde = dark ? "#22A367" : "#166F4A";
    const QString verme = dark ? "#E85555" : "#C94040";
    const QString cor   = saldo >= 0 ? verde : verme;

    m_valorSaldo->setStyleSheet(
        QString("font-size: 26px; font-weight: 600; color: %1;").arg(cor)
    );
    m_valorEntradas->setStyleSheet(
        QString("font-size: 22px; font-weight: 600; color: %1;").arg(verde)
    );
    m_valorFixos->setStyleSheet(
        QString("font-size: 22px; font-weight: 600; color: %1;").arg(verme)
    );
    m_valorVariaveis->setStyleSheet(
        QString("font-size: 22px; font-weight: 600; color: %1;").arg(verme)
    );
}
