# CLAUDE.md — Orçamento Pessoal

## Visão geral do projeto

App de controle de orçamento pessoal em C++20/Qt6, para Linux (Fedora/KDE Plasma).

**Repositório GitHub:** petrinhu/orcamento_pessoal

## Stack

- **Linguagem:** C++20
- **UI:** Qt6 (Widgets, Charts)
- **Banco de dados:** MySQL (via Qt6::Sql + driver qt6-qtbase-mysql)
- **Criptografia:** OpenSSL — AES-256-CBC, PBKDF2-SHA256 (600.000 iterações)
- **Build:** CMake 3.16+
- **Plataforma:** Fedora 43 / KDE Plasma

## Dependências (dnf)

```bash
sudo dnf install \
  qt6-qtbase-devel \
  qt6-qtcharts-devel \
  qt6-qtbase-mysql \
  mysql-devel \
  openssl-devel \
  gcc-c++ cmake make -y
```

## Estrutura modular

```
src/
  main.cpp                  # Ponto de entrada
  core/
    CryptoHelper.h/cpp      # AES-256-CBC, PBKDF2
    DataManager.h/cpp       # Persistência JSON (legado)
    DatabaseManager.h/cpp   # Backend MySQL (novo)
  models/
    Entrada.h/cpp
    GastoFixo.h/cpp
    GastoVariavel.h/cpp
    Categoria.h/cpp
  ui/
    MainWindow.h/cpp
    PasswordDialog.h/cpp
    DashboardWidget.h/cpp
    EntradasWidget.h/cpp
    GastosFixosWidget.h/cpp
    GastosVariaveisWidget.h/cpp
    ConfigWidget.h/cpp
  utils/
    CurrencyUtils.h/cpp     # textoParaCentavos / centavosParaTexto
docs/
  architecture.md
  database.md
tests/
```

## Fluxo principal

`main()` → `PasswordDialog` → `DatabaseManager::conectar()` → `MainWindow` → abas

## Convenções

- Valores monetários sempre em **centavos (qint64)**
- Formatação: `centavosParaTexto()` → "R$ 0,00"
- Não criar código C++ sem perguntar ao usuário antes
- Commits sempre em português
