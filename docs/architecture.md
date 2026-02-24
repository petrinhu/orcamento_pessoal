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
       └─ DatabaseManager::conectar()
            └─ criarEsquema()   # CREATE TABLE IF NOT EXISTS
            └─ semear categorias padrão (se vazio)
  └─ MainWindow::show()
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
- Coleta credenciais MySQL (host, porta, banco, usuário, senha)
- Valida força da senha em tempo real (barra progressiva 4 níveis)
- Botão "Conectar" habilitado apenas com todos os requisitos atendidos

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
- Uma instância por processo via `static` local
- Conexão: `QSqlDatabase::addDatabase("QMYSQL")`
- `criarEsquema()` chamado automaticamente após `conectar()`
- Totais via `COALESCE(SUM(...), 0)` — O(1) no banco, sem carregar registros
- Listas via `JOIN` em uma única query — sem N+1 queries
- Todos os valores com `bindValue` — sem risco de SQL injection

### CryptoHelper (namespace)
- Funções puras sem estado: `encrypt`, `decrypt`, `derivarChaveEIV`
- AES-256-CBC via OpenSSL EVP
- PBKDF2-SHA256 com 600.000 iterações (NIST recomenda ≥ 600k para 2026)
- Disponível para uso futuro (ex: exportação criptografada)

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
