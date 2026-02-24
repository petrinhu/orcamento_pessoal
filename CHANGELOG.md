# Changelog

## [Não lançado] — 2026-02-24

### Adicionado
- `gerarSalt()` em `CryptoHelper` — salt aleatório via `RAND_bytes`
- Escrita atômica do banco: `.new` → rename → remove old
- `PRAGMA journal_mode = MEMORY` — sem arquivos de journal em disco
- `PRAGMA foreign_keys = ON` + `ON DELETE CASCADE` nas FKs

### Alterado
- **Migração MySQL → SQLite + AES-256-CBC:** banco agora é um arquivo local criptografado (`data/<usuario>.enc`)
- `DatabaseManager::conectar()` agora recebe `(nome, senha)` — sem host/porta/usuário de banco
- `PasswordDialog` redesenhado: splash com nome + senha + checklist de requisitos ✓/✗
- `CryptoHelper` agora central ao fluxo: decripta o banco no login e criptografa no encerramento
- Dependências: removidos `qt6-qtbase-mysql` e `mysql-devel`

### Removido
- Dependência de servidor MySQL — app agora é completamente standalone

---

## [0.2.0] — 2026-02-24

### Adicionado
- Estrutura modular completa em `src/` (Core, Models, UI, Utils)
- `DatabaseManager` — backend MySQL com conexão, esquema automático e CRUD completo
- `CryptoHelper` — namespace com AES-256-CBC e PBKDF2-SHA256 (600k iterações)
- `CurrencyUtils` — `textoParaCentavos` / `centavosParaTexto`
- Modelos: `Categoria`, `Entrada`, `GastoFixo`, `GastoVariavel`
- `Theme` — sistema de tema light/dark automático via `QStyleHints`, paleta Forest Neutral
- Fonte Inter (Regular, Medium, SemiBold) embutida via `.qrc`
- `PasswordDialog` — coleta credenciais MySQL + barra de força de senha (4 níveis)
- `DashboardWidget` — cards de resumo + gráfico donut (Qt6 Charts)
- `EntradasWidget` — tabela editável com persistência automática
- `GastosFixosWidget` — tabela com categorias + "Repetir mês anterior"
- `GastosVariaveisWidget` — tabela com categorias
- `ConfigWidget` — gerenciamento de categorias com input inline
- `MainWindow` — orquestração por sinais (`dadosAlterados`, `categoriasAlteradas`)
- `CMakeLists.txt` atualizado: Qt6::Sql, OpenSSL, `.qrc`

### Alterado
- Persistência migrada de JSON criptografado local para MySQL

### Removido
- `DataManager` (JSON legado) — descartado em favor do MySQL

---

## [0.1.0] — 2026-02-23

### Adicionado
- Versão monolítica inicial (`main.cpp`, 873 linhas)
- PasswordDialog com validação de força de senha
- DataManager — persistência JSON criptografada (AES-256-CBC + PBKDF2)
- Dashboard com saldo e gráfico de pizza
- Tabelas editáveis: Entradas, Gastos Fixos (com categorias), Gastos Variáveis
- Aba Configurações para gerenciar categorias
