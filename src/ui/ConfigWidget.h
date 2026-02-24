#pragma once

#include <QLineEdit>
#include <QListWidget>
#include <QWidget>

class ConfigWidget : public QWidget
{
    Q_OBJECT

signals:
    void categoriasAlteradas();

public:
    explicit ConfigWidget(QWidget *parent = nullptr);

private slots:
    void adicionarCategoria();
    void removerCategoria();

private:
    void carregar();

    QListWidget *m_lista;
    QLineEdit   *m_inputNova;
};
