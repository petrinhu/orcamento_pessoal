#pragma once

#include "models/Categoria.h"
#include "models/GastoVariavel.h"

#include <QComboBox>
#include <QList>
#include <QTableWidget>
#include <QWidget>

class GastosVariaveisWidget : public QWidget
{
    Q_OBJECT

signals:
    void dadosAlterados();

public:
    explicit GastosVariaveisWidget(QWidget *parent = nullptr);

    void recarregarCategorias();

private slots:
    void adicionarGasto();
    void removerGasto();
    void onItemChanged(QTableWidgetItem *item);

private:
    void carregar();
    void adicionarLinha(const GastoVariavel &g);
    void inserirLinhaTotalVazia();
    void atualizarTotal();
    void salvarLinha(int row);
    void conectarCombo(QComboBox *combo, int row);

    int       idDaLinha(int row) const;
    bool      isTotalRow(int row) const;
    QComboBox *comboDaLinha(int row) const;

    QList<Categoria> m_categorias;
    QTableWidget    *m_tabela;
    bool             m_carregando = false;
};
