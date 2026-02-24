#pragma once

#include "models/Categoria.h"
#include "models/Entrada.h"
#include "models/GastoFixo.h"
#include "models/GastoVariavel.h"

#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QtGlobal>

class DatabaseManager
{
public:
    static DatabaseManager &instance();

    // nome → determina o arquivo data/<nome>.enc
    bool conectar(const QString &nome, const QString &senha);
    void desconectar();
    bool isConectado() const;

    // Categorias
    QList<Categoria> listarCategorias();
    bool inserirCategoria(Categoria &cat);
    bool removerCategoria(int id);

    // Entradas
    QList<Entrada> listarEntradas();
    bool inserirEntrada(Entrada &entrada);
    bool atualizarEntrada(const Entrada &entrada);
    bool removerEntrada(int id);
    qint64 totalEntradas();

    // Gastos Fixos
    QList<GastoFixo> listarGastosFixos();
    bool inserirGastoFixo(GastoFixo &gasto);
    bool atualizarGastoFixo(const GastoFixo &gasto);
    bool removerGastoFixo(int id);
    qint64 totalGastosFixos();

    // Gastos Variáveis
    QList<GastoVariavel> listarGastosVariaveis();
    bool inserirGastoVariavel(GastoVariavel &gasto);
    bool atualizarGastoVariavel(const GastoVariavel &gasto);
    bool removerGastoVariavel(int id);
    qint64 totalGastosVariaveis();

private:
    DatabaseManager() = default;

    bool criarEsquema();
    bool decriptarParaTemp();
    bool salvarEEncriptar();

    QSqlDatabase m_db;
    QString      m_senha;
    QString      m_arquivoEnc;  // data/<nome>.enc
    QString      m_arquivoTmp;  // data/.<nome>.db  (temp, deletado ao sair)
};
