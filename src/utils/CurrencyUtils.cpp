#include "utils/CurrencyUtils.h"

qint64 textoParaCentavos(const QString &texto)
{
    QString limpo = texto;
    limpo.remove('.').remove(',').remove('R').remove('$').remove(' ');
    if (limpo.isEmpty()) return 0;
    QString str = limpo.rightJustified(3, '0');
    QString cents = str.right(2);
    QString reais = str.left(str.length() - 2);
    if (reais.isEmpty()) reais = "0";
    return reais.toLongLong() * 100 + cents.toLongLong();
}

QString centavosParaTexto(qint64 centavos)
{
    if (centavos == 0) return "R$ 0,00";
    QString str = QString::number(qAbs(centavos));
    while (str.length() < 3) str.prepend('0');
    QString reais = str.left(str.length() - 2);
    QString cents = str.right(2);
    if (reais.isEmpty()) reais = "0";
    return (centavos < 0 ? "-R$ " : "R$ ") + reais + "," + cents;
}
