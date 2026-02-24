#pragma once

#include <QMainWindow>
#include <QTabWidget>

class DashboardWidget;
class EntradasWidget;
class GastosFixosWidget;
class GastosVariaveisWidget;
class ConfigWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void conectarSinais();

    QTabWidget            *m_tabs;
    DashboardWidget       *m_dashboard;
    EntradasWidget        *m_entradas;
    GastosFixosWidget     *m_gastosFixos;
    GastosVariaveisWidget *m_gastosVariaveis;
    ConfigWidget          *m_config;
};
