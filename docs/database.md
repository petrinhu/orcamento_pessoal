# Banco de Dados — Orçamento Pessoal

## Visão geral

O banco de dados é um arquivo SQLite criptografado com AES-256-CBC. Nenhum servidor de banco de dados é necessário.

```
data/
  <usuario>.enc      # banco SQLite criptografado (AES-256-CBC)
  .<usuario>.db      # arquivo temporário em uso — deletado ao fechar
```

O app cria as tabelas automaticamente na primeira execução (`CREATE TABLE IF NOT EXISTS`).

## Fluxo de criptografia

**Login:**
1. Nome → slug → `data/<slug>.enc`
2. PBKDF2-SHA256 (600k iter.) deriva chave + IV da senha e do salt embarcado no arquivo
3. AES-256-CBC decripta `.enc` → `.db` temporário
4. SQLite abre o `.db`

**Encerramento:**
1. SQLite fecha o `.db`
2. AES-256-CBC encripta `.db` → `.enc` (escrita atômica: `.new` → rename → remove old)
3. `.db` temporário é deletado

## Configuração

Nenhuma. O arquivo `.enc` é criado automaticamente na primeira execução.

## Esquema

### categorias

```sql
CREATE TABLE categorias (
  id   INTEGER PRIMARY KEY AUTOINCREMENT,
  nome TEXT NOT NULL UNIQUE
);
```

Categorias padrão semeadas na primeira execução:
`Aluguel/Moradia`, `Internet`, `Luz/Água/Gás`, `Transporte`, `Alimentação`,
`Educação`, `Saúde`, `Streaming/TV/Telefone`, `Academia`, `Outros`, `Crédito`

### entradas

```sql
CREATE TABLE entradas (
  id             INTEGER PRIMARY KEY AUTOINCREMENT,
  origem         TEXT    NOT NULL DEFAULT '',
  valor_centavos INTEGER NOT NULL DEFAULT 0,
  data           TEXT    NOT NULL
);
```

### gastos_fixos

```sql
CREATE TABLE gastos_fixos (
  id             INTEGER PRIMARY KEY AUTOINCREMENT,
  historico      TEXT    NOT NULL DEFAULT '',
  valor_centavos INTEGER NOT NULL DEFAULT 0,
  data           TEXT    NOT NULL,
  categoria_id   INTEGER NOT NULL,
  FOREIGN KEY (categoria_id) REFERENCES categorias(id) ON DELETE CASCADE
);
```

### gastos_variaveis

```sql
CREATE TABLE gastos_variaveis (
  id             INTEGER PRIMARY KEY AUTOINCREMENT,
  historico      TEXT    NOT NULL DEFAULT '',
  valor_centavos INTEGER NOT NULL DEFAULT 0,
  data           TEXT    NOT NULL,
  categoria_id   INTEGER NOT NULL,
  FOREIGN KEY (categoria_id) REFERENCES categorias(id) ON DELETE CASCADE
);
```

## Diagrama ER

```
categorias
  id (PK)
  nome

entradas                gastos_fixos              gastos_variaveis
  id (PK)                 id (PK)                   id (PK)
  origem                  historico                 historico
  valor_centavos          valor_centavos            valor_centavos
  data                    data                      data
                          categoria_id (FK) ──┐     categoria_id (FK) ──┘
                                              └─── categorias.id
```

## Decisões de design

| Decisão | Motivo |
|---|---|
| SQLite + AES-256-CBC | App standalone; sem servidor; portátil entre máquinas |
| `INTEGER` para valores | SQLite usa `INTEGER` — mapeado para `qint64` no Qt |
| `TEXT` para datas | SQLite não tem tipo DATE nativo; formato `yyyy-MM-dd` |
| `FOREIGN KEY ... ON DELETE CASCADE` | Remoção de categoria cascateia para gastos vinculados |
| `UNIQUE` em `categorias.nome` | Evita duplicatas no nível do banco |
| `PRAGMA journal_mode = MEMORY` | Sem arquivos de journal em disco |
| `PRAGMA foreign_keys = ON` | Integridade referencial ativa (desligada por padrão no SQLite) |

## Queries principais

**Totais (O(1) no banco):**
```sql
SELECT COALESCE(SUM(valor_centavos), 0) FROM entradas;
SELECT COALESCE(SUM(valor_centavos), 0) FROM gastos_fixos;
SELECT COALESCE(SUM(valor_centavos), 0) FROM gastos_variaveis;
```

**Lista com categoria (JOIN único):**
```sql
SELECT gf.id, gf.historico, gf.valor_centavos, gf.data,
       gf.categoria_id, c.nome
FROM gastos_fixos gf
JOIN categorias c ON gf.categoria_id = c.id
ORDER BY gf.data DESC;
```
