#pragma once

#include <QLabel>
#include <QWidget>
#include <QtCharts/QChartView>
#include <QtTypes>

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget *parent = nullptr);

public slots:
    void atualizar();

private:
    QWidget    *makeCard(const QString &titulo, QLabel *&valorLabel);
    void        atualizarCores(qint64 saldo);

    // Cards de resumo
    QLabel *m_valorEntradas;
    QLabel *m_valorFixos;
    QLabel *m_valorVariaveis;
    QLabel *m_valorSaldo;

    QChartView *m_chartView;
};
