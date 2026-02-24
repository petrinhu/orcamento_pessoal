#include "ui/MainWindow.h"

#include "ui/ConfigWidget.h"
#include "ui/DashboardWidget.h"
#include "ui/EntradasWidget.h"
#include "ui/GastosFixosWidget.h"
#include "ui/GastosVariaveisWidget.h"

#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Orçamento Pessoal");
    resize(1200, 800);
    setMinimumSize(900, 600);

    m_tabs            = new QTabWidget(this);
    m_dashboard       = new DashboardWidget(this);
    m_entradas        = new EntradasWidget(this);
    m_gastosFixos     = new GastosFixosWidget(this);
    m_gastosVariaveis = new GastosVariaveisWidget(this);
    m_config          = new ConfigWidget(this);

    m_tabs->addTab(m_dashboard,       "Dashboard");
    m_tabs->addTab(m_entradas,        "Entradas");
    m_tabs->addTab(m_gastosFixos,     "Gastos Fixos");
    m_tabs->addTab(m_gastosVariaveis, "Gastos Variáveis");
    m_tabs->addTab(m_config,          "Configurações");

    setCentralWidget(m_tabs);
    statusBar()->hide();

    conectarSinais();
}

void MainWindow::conectarSinais()
{
    // Qualquer alteração de dados recalcula o dashboard
    auto refresh = [this]() { m_dashboard->atualizar(); };

    connect(m_entradas,        &EntradasWidget::dadosAlterados,        this, refresh);
    connect(m_gastosFixos,     &GastosFixosWidget::dadosAlterados,     this, refresh);
    connect(m_gastosVariaveis, &GastosVariaveisWidget::dadosAlterados, this, refresh);
    connect(m_config, &ConfigWidget::categoriasAlteradas, this, refresh);

    // Quando categorias mudam, recarrega os combos dos widgets de gastos
    connect(m_config, &ConfigWidget::categoriasAlteradas,
            m_gastosFixos,     &GastosFixosWidget::recarregarCategorias);
    connect(m_config, &ConfigWidget::categoriasAlteradas,
            m_gastosVariaveis, &GastosVariaveisWidget::recarregarCategorias);

    // Ao trocar para a aba Dashboard, sempre recalcula
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int idx) {
        if (m_tabs->widget(idx) == m_dashboard)
            m_dashboard->atualizar();
    });
}
