# Memória — Orçamento Pessoal

## Projeto

App de controle de orçamento pessoal em C++20/Qt6/SQLite para Linux (Fedora/KDE Plasma).

- **Repo GitHub:** https://github.com/petrinhu/orcamento_pessoal
- **Diretório local:** /home/petrus/IDrive/Documentos/projetos_dev/orcamento-pessoal
- **Branch principal:** main

## Stack confirmada

- C++20, CMake 3.16+
- Qt6: Widgets, Charts, Sql (**QSQLITE** built-in — sem driver externo)
- **MySQL descartado** — usar SQLite com criptografia de arquivo
- OpenSSL: AES-256-CBC, PBKDF2-SHA256 (600.000 iterações)
- Fonte Inter embutida via `.qrc`
- Tema light/dark automático via `QStyleHints` (Qt6)
- Fedora 43 / KDE Plasma

## Dependências (dnf)

```bash
sudo dnf install qt6-qtbase-devel qt6-qtcharts-devel openssl-devel gcc-c++ cmake make -y
# qt6-qtbase-mysql e mysql-devel NÃO são necessários
```

## Estado atual (2026-02-24)

**Build 100% limpo.** Arquitetura SQLite+AES concluída e compilando.

Commit mais recente: `51d2b11` — Migração MySQL → SQLite + redesign login

## Arquivos implementados

| Arquivo | Status |
|---|---|
| CMakeLists.txt | ✓ Qt6::Sql, OpenSSL, .qrc, sem mysql |
| src/main.cpp | ✓ conectar(nome, senha) |
| src/core/CryptoHelper.h/cpp | ✓ namespace, funções puras + gerarSalt() |
| src/core/DatabaseManager.h/cpp | ✓ Singleton, **SQLite+AES**, CRUD completo |
| src/core/DataManager.h/cpp | Descartado (vazios) |
| src/models/Categoria.h/cpp | ✓ struct POD |
| src/models/Entrada.h/cpp | ✓ struct POD |
| src/models/GastoFixo.h/cpp | ✓ struct POD |
| src/models/GastoVariavel.h/cpp | ✓ struct POD |
| src/ui/Theme.h/cpp | ✓ Paleta Forest Neutral, QSS, Inter |
| src/ui/PasswordDialog.h/cpp | ✓ Splash: nome + senha + checklist ✓/✗ |
| src/ui/MainWindow.h/cpp | ✓ |
| src/ui/DashboardWidget.h/cpp | ✓ Cards + donut chart |
| src/ui/EntradasWidget.h/cpp | ✓ |
| src/ui/GastosFixosWidget.h/cpp | ✓ + Repetir mês anterior |
| src/ui/GastosVariaveisWidget.h/cpp | ✓ |
| src/ui/ConfigWidget.h/cpp | ✓ |
| src/utils/CurrencyUtils.h/cpp | ✓ |
| resources/resources.qrc | ✓ |
| resources/fonts/Inter-*.ttf | ✓ baixados |

## Convenções do usuário

- **Não criar código C++ sem perguntar antes**
- Valores monetários sempre em centavos (qint64)
- Commits em português
- O usuário prefere aprovar cada etapa de implementação
- Preferência por diálogo de texto (sem AskUserQuestion com opções clicáveis)
- "Faça o mais indicado para o porte do projeto e menor O(n)"

## Design

- **Paleta:** Forest Neutral — acento verde-floresta (#166F4A light / #22A367 dark)
- **Background light:** #F5F4F0 (greige quente)
- **Background dark:** #18181A (carvão quente)
- **Fonte:** Inter 13px corpo, 12px labels, 14-16px títulos
- **Tabelas:** linha 36px, sem grid, alternating rows sutil
- **Botão primário:** fundo acento + texto branco; secundário: transparente + borda

## Arquitetura chave

- `DatabaseManager` singleton — `static` local em `instance()`
- **Dados em** `data/<nome>.enc` (AES-256-CBC); temp `data/.<nome>.db` deletado no encerramento
- **Fluxo login:** nome determina arquivo → PBKDF2 deriva chave da senha → decripta
- `PRAGMA journal_mode = MEMORY` — sem arquivos de journal; `PRAGMA foreign_keys = ON`
- Escrita atômica: `.new` → rename → remove old
- `m_carregando: bool` em cada widget — bloqueia `itemChanged` durante load
- ID do banco em `Qt::UserRole` célula col 0; linha TOTAL tem id = -1
- Sinais: `dadosAlterados()` → `DashboardWidget::atualizar()` via MainWindow
- `categoriasAlteradas()` → `recarregarCategorias()` nos widgets de gastos
- Totais via `COALESCE(SUM(...), 0)` no banco — O(1)
- Listas via JOIN único — sem N+1 queries
