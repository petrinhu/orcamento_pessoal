#pragma once

#include <QTableWidget>
#include <QWidget>

class EntradasWidget : public QWidget
{
    Q_OBJECT

signals:
    void dadosAlterados();

public:
    explicit EntradasWidget(QWidget *parent = nullptr);

private slots:
    void adicionarEntrada();
    void removerEntrada();
    void onItemChanged(QTableWidgetItem *item);

private:
    void carregar();
    void adicionarLinha(int id, const QDate &data,
                        const QString &origem, qint64 valorCentavos);
    void inserirLinhaTotalVazia();
    void atualizarTotal();
    int  idDaLinha(int row) const;
    bool isTotalRow(int row) const;

    QTableWidget *m_tabela;
    bool          m_carregando = false; // bloqueia itemChanged durante load
};
