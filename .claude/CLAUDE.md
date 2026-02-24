# CLAUDE.md — Orçamento Pessoal

## Visão geral do projeto

App de controle de orçamento pessoal em C++20/Qt6/SQLite, para Linux (Fedora/KDE Plasma).

**Repositório GitHub:** petrinhu/orcamento_pessoal

## Stack

- **Linguagem:** C++20
- **UI:** Qt6 (Widgets, Charts)
- **Banco de dados:** SQLite via QSQLITE (built-in no Qt6 — sem driver externo)
- **Criptografia:** OpenSSL — AES-256-CBC, PBKDF2-SHA256 (600.000 iterações)
- **Build:** CMake 3.16+
- **Plataforma:** Fedora 43 / KDE Plasma
- **Fonte:** Inter (Regular, Medium, SemiBold) — embutida via .qrc
- **Tema:** light/dark automático via QStyleHints (Qt6)

## Dependências (dnf)

```bash
sudo dnf install \
  qt6-qtbase-devel \
  qt6-qtcharts-devel \
  openssl-devel \
  gcc-c++ cmake make -y
# qt6-qtbase-mysql e mysql-devel NÃO são necessários
```

## Estrutura modular (implementada)

```
src/
  main.cpp                       # Ponto de entrada
  core/
    CryptoHelper.h/cpp           # AES-256-CBC, PBKDF2, gerarSalt() (namespace)
    DatabaseManager.h/cpp        # SQLite+AES — conectar(nome,senha), CRUD
    DataManager.h/cpp            # DESCARTADO (JSON legado — arquivos vazios)
  models/
    Categoria.h/cpp              # struct POD
    Entrada.h/cpp                # struct POD
    GastoFixo.h/cpp              # struct POD
    GastoVariavel.h/cpp          # struct POD
  ui/
    Theme.h/cpp                  # Paleta Forest Neutral, QSS, Inter, light/dark
    MainWindow.h/cpp
    PasswordDialog.h/cpp         # Splash: nome + senha + checklist ✓/✗
    DashboardWidget.h/cpp        # Cards + gráfico donut
    EntradasWidget.h/cpp
    GastosFixosWidget.h/cpp      # + Repetir mês anterior
    GastosVariaveisWidget.h/cpp
    ConfigWidget.h/cpp
  utils/
    CurrencyUtils.h/cpp          # textoParaCentavos / centavosParaTexto
resources/
  fonts/Inter-Regular.ttf
  fonts/Inter-Medium.ttf
  fonts/Inter-SemiBold.ttf
  resources.qrc
data/                            # criado em runtime — NÃO versionar
  <nome>.enc                     # banco SQLite criptografado (AES-256-CBC)
  .<nome>.db                     # arquivo temporário (deletado ao fechar)
docs/
  architecture.md
  database.md
tests/
```

## Fluxo principal

`main()` → `Theme::aplicar()` → `PasswordDialog(nome+senha)` → `DatabaseManager::conectar(nome,senha)` → `MainWindow` → abas

## Arquitetura de dados

- Nome do usuário → slug → `data/<slug>.enc` (banco criptografado)
- Login: PBKDF2 deriva chave da senha → AES decripta → SQLite abre o `.db` temporário
- Encerramento: SQLite fecha → AES encripta → escrita atômica → `.db` deletado
- `PRAGMA journal_mode = MEMORY` — sem journal files em disco
- `PRAGMA foreign_keys = ON` + `ON DELETE CASCADE` nas FKs

## Paleta de cores (Forest Neutral)

| Token | Light | Dark |
|---|---|---|
| Background | #F5F4F0 | #18181A |
| Surface | #FFFFFF | #222225 |
| Acento | #166F4A | #22A367 |
| Positivo | #166F4A | #22A367 |
| Negativo | #C94040 | #E85555 |

## Convenções

- Valores monetários sempre em **centavos (qint64)** — nunca float/double
- Datas: `QDate` nos modelos; `dd/MM/yyyy` na UI
- Formatação: `centavosParaTexto()` → "R$ 0,00"
- IDs: 0 = não persistido, -1 = linha TOTAL nas tabelas
- **Não criar código C++ sem perguntar ao usuário antes**
- Commits sempre em português
- Sinais: `dadosAlterados()` e `categoriasAlteradas()` — direção única (widgets → MainWindow)
- Queries SQL: sempre com `bindValue` (sem concatenação)

## Estado atual

- Implementação completa e build 100% limpo
- Commit: `51d2b11` — Migração MySQL → SQLite+AES; redesign login
