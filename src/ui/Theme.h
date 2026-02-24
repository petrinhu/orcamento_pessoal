#pragma once

#include <QString>

namespace Theme {

// Detecta se o sistema está em modo escuro
bool isDark();

// Carrega as fontes Inter no app (chamar uma vez em main())
void carregarFontes();

// Retorna o stylesheet completo para o tema atual
QString stylesheet(bool dark);

// Aplica o tema ao QApplication de acordo com o sistema
void aplicar();

} // namespace Theme
