# Componentes do sistema

## Core

### CryptoHelper (`src/core/`)
- Namespace com funções puras (sem estado)
- `gerarSalt() → QByteArray` — salt aleatório via `RAND_bytes`
- `derivarChaveEIV(salt, senha, chave, iv) → bool` — PBKDF2-SHA256, 600k iterações
- `encrypt(plaintext, key, iv) → QByteArray` — AES-256-CBC
- `decrypt(ciphertext, key, iv) → QByteArray` — AES-256-CBC

### DatabaseManager (`src/core/`)
- Singleton — `DatabaseManager::instance()`
- `conectar(nome, senha) → bool` — slug → `.enc` → decripta → SQLite abre `.db` temp
- `criarEsquema() → bool` — `CREATE TABLE IF NOT EXISTS` + semeadura de categorias
- CRUD completo: Categoria, Entrada, GastoFixo, GastoVariavel
- Totais via `COALESCE(SUM(...), 0)` no banco (O(1))
- Listas via JOIN único (sem N+1)
- `PRAGMA journal_mode = MEMORY` · `PRAGMA foreign_keys = ON`

## Models (`src/models/`)

Structs POD sem métodos:

| Struct | Campos principais |
|---|---|
| `Categoria` | id, nome |
| `Entrada` | id, origem, valorCentavos, data |
| `GastoFixo` | id, historico, valorCentavos, data, categoriaId, categoriaNome |
| `GastoVariavel` | id, historico, valorCentavos, data, categoriaId, categoriaNome |

## UI (`src/ui/`)

### Theme
- `isDark() → bool` — detecta tema do sistema via Qt6 QStyleHints
- `carregarFontes()` — Inter Regular/Medium/SemiBold do `.qrc`
- `stylesheet(dark) → QString` — QSS completo (paleta Forest Neutral)
- `aplicar()` — aplica tudo + listener de mudança de tema

### PasswordDialog
- Campos: nome de usuário + senha
- Checklist visual de requisitos (✓/✗ em tempo real)
- Botão "Entrar" habilitado apenas com todos os requisitos atendidos

### MainWindow
- Orquestra 5 abas via `QTabWidget`
- Propaga `dadosAlterados()` → `DashboardWidget::atualizar()`
- Propaga `categoriasAlteradas()` → `recarregarCategorias()` dos widgets de gastos

### DashboardWidget
- 4 cards: Entradas (verde), Gastos Fixos (vermelho), Gastos Variáveis (vermelho), Saldo (dinâmico)
- Gráfico donut (QPieSeries, holeSize=0.40)
- `atualizar()` — 3 queries SUM + recria o gráfico

### EntradasWidget / GastosFixosWidget / GastosVariaveisWidget
- Edição inline com persistência imediata
- `m_carregando` — flag anti-loop em `itemChanged`
- ID do banco em `Qt::UserRole` por linha
- Linha TOTAL (id = -1) não editável

### ConfigWidget
- Lista de categorias com `QListWidget`
- Input inline + Enter para adicionar
- Emite `categoriasAlteradas()` ao modificar

## Utils (`src/utils/`)

### CurrencyUtils
- `textoParaCentavos(QString) → qint64`
- `centavosParaTexto(qint64) → QString`

## Fluxo principal

```
main()
  Theme::aplicar()
  loop:
    PasswordDialog.exec()
    DatabaseManager::conectar(nome, senha)  →  criarEsquema()
  MainWindow.show()
  app.exec()
  [encerramento: encrypt .db → .enc → delete .db]
```
