# Arquitetura — Orçamento Pessoal

## Visão geral

Arquitetura em camadas com separação clara de responsabilidades:

```
┌─────────────────────────────────────────┐
│                   UI                    │
│  MainWindow · PasswordDialog · Theme    │
│  Dashboard · Entradas · GastosFixos     │
│  GastosVariaveis · Config               │
├─────────────────────────────────────────┤
│                  Core                   │
│    DatabaseManager · CryptoHelper       │
├─────────────────────────────────────────┤
│                 Models                  │
│  Categoria · Entrada · GastoFixo        │
│  GastoVariavel                          │
├─────────────────────────────────────────┤
│                 Utils                   │
│         CurrencyUtils                   │
└─────────────────────────────────────────┘
```

## Fluxo de inicialização

```
main()
  └─ Theme::aplicar()           # fontes Inter + QSS + listener sistema
  └─ PasswordDialog (loop)
       └─ DatabaseManager::conectar(nome, senha)
            └─ CryptoHelper::decrypt()   # AES-256-CBC: .enc → .db temp
            └─ QSqlDatabase::open()      # SQLite no .db temporário
            └─ criarEsquema()            # CREATE TABLE IF NOT EXISTS
            └─ semear categorias padrão (se vazio)
  └─ MainWindow::show()
  └─ [ao fechar]
       └─ QSqlDatabase::close()
       └─ CryptoHelper::encrypt()        # AES-256-CBC: .db → .enc
       └─ escrita atômica (.new → rename → remove old)
       └─ QFile::remove(.db temporário)
```

## Camada UI

### Theme
- Detecta `Qt::ColorScheme` via `QStyleHints` (Qt6 nativo)
- Aplica QSS completo cobrindo todos os widgets
- Reconecta ao `colorSchemeChanged` para troca de tema em tempo real
- Fontes Inter embutidas via `.qrc`

### MainWindow
- `QTabWidget` com 5 abas
- Conecta `dadosAlterados()` de cada widget ao `DashboardWidget::atualizar()`
- Conecta `categoriasAlteradas()` do `ConfigWidget` ao `recarregarCategorias()` dos widgets de gastos

### PasswordDialog
- Coleta nome de usuário e senha
- Nome determina o arquivo `data/<slug>.enc` a ser carregado
- Checklist visual de requisitos de senha (✓/✗ em tempo real)
- Botão "Entrar" habilitado apenas com todos os requisitos atendidos

### Widgets de dados (Entradas, GastosFixos, GastosVariaveis)
- `m_carregando: bool` — flag que bloqueia `itemChanged` durante load/reformatação
- ID do banco armazenado em `Qt::UserRole` na primeira célula de cada linha
- Linha de TOTAL (id = -1) sempre na última posição, não editável
- Edição inline salva imediatamente no banco via `DatabaseManager`
- Emitem `dadosAlterados()` após qualquer escrita

### ConfigWidget
- Lista de categorias com `id` em `Qt::UserRole`
- Input inline (sem QInputDialog) + `returnPressed`
- Emite `categoriasAlteradas()` — MainWindow propaga para os widgets de gastos

## Camada Core

### DatabaseManager (Singleton)
- Uma instância por processo via `static` local em `instance()`
- `conectar(nome, senha)` — deriva slug, localiza `.enc`, decripta, abre `.db` temporário
- `PRAGMA journal_mode = MEMORY` — sem arquivos de journal em disco
- `PRAGMA foreign_keys = ON` — integridade referencial ativa
- `criarEsquema()` chamado automaticamente após `conectar()`
- Totais via `COALESCE(SUM(...), 0)` — O(1) no banco, sem carregar registros
- Listas via `JOIN` em uma única query — sem N+1 queries
- Todos os valores com `bindValue` — sem risco de SQL injection

### CryptoHelper (namespace)
- Funções puras sem estado: `encrypt`, `decrypt`, `derivarChaveEIV`, `gerarSalt`
- AES-256-CBC via OpenSSL EVP
- PBKDF2-SHA256 com 600.000 iterações (NIST SP 800-132, 2026)
- Salt aleatório via `RAND_bytes` — gerado a cada escrita
- Escrita atômica: arquivo `.new` → `rename` → remove antigo

## Camada Models

Structs POD com valores padrão — sem métodos, sem herança:

| Struct | Campos |
|---|---|
| `Categoria` | id, nome |
| `Entrada` | id, origem, valorCentavos, data |
| `GastoFixo` | id, historico, valorCentavos, data, categoriaId, categoriaNome |
| `GastoVariavel` | id, historico, valorCentavos, data, categoriaId, categoriaNome |

`categoriaNome` é desnormalizado via JOIN no load — evita queries adicionais na UI.

## Camada Utils

### CurrencyUtils
- `textoParaCentavos(QString) → qint64` — parse de entrada do usuário
- `centavosParaTexto(qint64) → QString` — formatação "R$ 0,00"
- Valores sempre em `qint64` (centavos) — sem ponto flutuante em operações financeiras

## Convenções

- **Valores monetários:** sempre `qint64` em centavos
- **Datas:** `QDate` nos modelos; exibidas como `dd/MM/yyyy` na UI
- **IDs:** `int`, 0 = não persistido, -1 = linha especial (TOTAL)
- **Sinais:** `dadosAlterados()` e `categoriasAlteradas()` — direção única (widgets → MainWindow)
