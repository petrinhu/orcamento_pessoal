#pragma once

#include <QDate>
#include <QString>
#include <QtGlobal>

struct GastoFixo {
    int     id            = 0;
    QString historico;
    qint64  valorCentavos = 0;
    QDate   data;
    int     categoriaId   = 0;
    QString categoriaNome;  // preenchido via JOIN ao carregar do banco
};
