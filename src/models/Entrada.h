#pragma once

#include <QDate>
#include <QString>
#include <QtTypes>

struct Entrada {
    int     id            = 0;
    QString origem;
    qint64  valorCentavos = 0;
    QDate   data;
};
