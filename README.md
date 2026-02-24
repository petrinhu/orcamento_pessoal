# Orçamento Pessoal

App de controle de orçamento pessoal em C++20/Qt6/MySQL para Linux (Fedora/KDE Plasma).

## Funcionalidades

- **Dashboard** — cards de resumo (entradas, gastos fixos, gastos variáveis, saldo) e gráfico donut interativo
- **Entradas** — tabela editável com persistência automática no MySQL
- **Gastos Fixos** — tabela com categorias e botão "Repetir mês anterior"
- **Gastos Variáveis** — tabela com categorias
- **Configurações** — gerenciamento de categorias compartilhadas
- **Tema automático** — light/dark seguindo o sistema (KDE Plasma)
- **Fonte Inter** — embutida no binário

## Stack

| Componente | Tecnologia |
|---|---|
| Linguagem | C++20 |
| UI | Qt6 Widgets + Charts |
| Banco de dados | MySQL via Qt6::Sql |
| Criptografia | OpenSSL — AES-256-CBC, PBKDF2-SHA256 (600k iter.) |
| Build | CMake 3.16+ |
| Plataforma | Fedora 43 / KDE Plasma |

## Dependências

```bash
sudo dnf install \
  qt6-qtbase-devel \
  qt6-qtcharts-devel \
  qt6-qtbase-mysql \
  mysql-devel \
  openssl-devel \
  gcc-c++ cmake make -y
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/orcamento-pessoal
```

## Banco de dados

Crie o banco antes de executar:

```sql
CREATE DATABASE orcamento CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'orcamento'@'localhost' IDENTIFIED BY 'sua_senha';
GRANT ALL PRIVILEGES ON orcamento.* TO 'orcamento'@'localhost';
FLUSH PRIVILEGES;
```

O esquema (tabelas) é criado automaticamente pelo app na primeira execução.

## Estrutura

```
src/
  main.cpp
  core/
    CryptoHelper.h/cpp       # AES-256-CBC, PBKDF2-SHA256
    DatabaseManager.h/cpp    # Conexão MySQL + CRUD
  models/
    Categoria.h/cpp
    Entrada.h/cpp
    GastoFixo.h/cpp
    GastoVariavel.h/cpp
  ui/
    Theme.h/cpp              # Paleta, QSS, Inter, light/dark
    MainWindow.h/cpp
    PasswordDialog.h/cpp
    DashboardWidget.h/cpp
    EntradasWidget.h/cpp
    GastosFixosWidget.h/cpp
    GastosVariaveisWidget.h/cpp
    ConfigWidget.h/cpp
  utils/
    CurrencyUtils.h/cpp      # textoParaCentavos / centavosParaTexto
resources/
  fonts/                     # Inter Regular, Medium, SemiBold (embutidas)
  resources.qrc
docs/
  architecture.md
  database.md
```

## Licença

MIT © 2026 Petrus Silva Costa
