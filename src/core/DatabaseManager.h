#pragma once

#include "models/Categoria.h"
#include "models/Entrada.h"
#include "models/GastoFixo.h"
#include "models/GastoVariavel.h"

#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QtTypes>

class DatabaseManager
{
public:
    static DatabaseManager &instance();

    bool conectar(const QString &host, int porta, const QString &banco,
                  const QString &usuario, const QString &senha);
    void desconectar();
    bool isConectado() const;

    // Esquema
    bool criarEsquema();

    // Categorias
    QList<Categoria> listarCategorias();
    bool inserirCategoria(Categoria &cat);   // preenche cat.id
    bool removerCategoria(int id);

    // Entradas
    QList<Entrada> listarEntradas();
    bool inserirEntrada(Entrada &entrada);   // preenche entrada.id
    bool atualizarEntrada(const Entrada &entrada);
    bool removerEntrada(int id);
    qint64 totalEntradas();

    // Gastos Fixos
    QList<GastoFixo> listarGastosFixos();
    bool inserirGastoFixo(GastoFixo &gasto);  // preenche gasto.id
    bool atualizarGastoFixo(const GastoFixo &gasto);
    bool removerGastoFixo(int id);
    qint64 totalGastosFixos();

    // Gastos Variáveis
    QList<GastoVariavel> listarGastosVariaveis();
    bool inserirGastoVariavel(GastoVariavel &gasto);  // preenche gasto.id
    bool atualizarGastoVariavel(const GastoVariavel &gasto);
    bool removerGastoVariavel(int id);
    qint64 totalGastosVariaveis();

private:
    DatabaseManager() = default;
    QSqlDatabase m_db;
};
