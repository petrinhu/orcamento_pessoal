# Orçamento Pessoal

App de controle de orçamento pessoal em C++20/Qt6/SQLite para Linux (Fedora/KDE Plasma).

## Funcionalidades

- **Dashboard** — cards de resumo (entradas, gastos fixos, gastos variáveis, saldo) e gráfico donut interativo
- **Entradas** — tabela editável com persistência automática
- **Gastos Fixos** — tabela com categorias e botão "Repetir mês anterior"
- **Gastos Variáveis** — tabela com categorias
- **Configurações** — gerenciamento de categorias compartilhadas
- **Banco criptografado** — SQLite + AES-256-CBC por arquivo; sem servidor externo
- **Tema automático** — light/dark seguindo o sistema (KDE Plasma)
- **Fonte Inter** — embutida no binário

## Stack

| Componente | Tecnologia |
|---|---|
| Linguagem | C++20 |
| UI | Qt6 Widgets + Charts |
| Banco de dados | SQLite via QSQLITE (built-in no Qt6) |
| Criptografia | OpenSSL — AES-256-CBC, PBKDF2-SHA256 (600k iter.) |
| Build | CMake 3.16+ |
| Plataforma | Fedora 43 / KDE Plasma |

## Dependências

```bash
sudo dnf install \
  qt6-qtbase-devel \
  qt6-qtcharts-devel \
  openssl-devel \
  gcc-c++ cmake make -y
# qt6-qtbase-mysql e mysql-devel NÃO são necessários
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/orcamento-pessoal
```

## Banco de dados

Nenhuma configuração prévia necessária. Na primeira execução o app solicita nome de usuário e senha — o banco SQLite é criado e criptografado automaticamente em `data/<usuario>.enc`.

- O arquivo `.enc` é portátil e pode ser copiado entre máquinas
- A senha é derivada via PBKDF2 (600k iterações) para cifrar o banco com AES-256-CBC
- Nenhum servidor de banco de dados é necessário

## Estrutura

```
src/
  main.cpp
  core/
    CryptoHelper.h/cpp       # AES-256-CBC, PBKDF2-SHA256, gerarSalt()
    DatabaseManager.h/cpp    # SQLite+AES — conectar(nome,senha), CRUD
  models/
    Categoria.h/cpp
    Entrada.h/cpp
    GastoFixo.h/cpp
    GastoVariavel.h/cpp
  ui/
    Theme.h/cpp              # Paleta Forest Neutral, QSS, Inter, light/dark
    MainWindow.h/cpp
    PasswordDialog.h/cpp     # Splash: nome + senha + checklist ✓/✗
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
data/                        # criado em runtime — NÃO versionar
  <usuario>.enc              # banco SQLite criptografado
docs/
  architecture.md
  database.md
```

## Licença

MIT © 2026 Petrus Silva Costa
