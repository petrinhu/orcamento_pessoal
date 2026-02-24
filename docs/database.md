# Banco de Dados — Orçamento Pessoal

## Configuração inicial

```sql
CREATE DATABASE orcamento
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

CREATE USER 'orcamento'@'localhost' IDENTIFIED BY 'sua_senha';
GRANT ALL PRIVILEGES ON orcamento.* TO 'orcamento'@'localhost';
FLUSH PRIVILEGES;
```

O app cria as tabelas automaticamente na primeira execução (`CREATE TABLE IF NOT EXISTS`).

## Esquema

### categorias

```sql
CREATE TABLE categorias (
  id   INT AUTO_INCREMENT PRIMARY KEY,
  nome VARCHAR(100) NOT NULL UNIQUE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

Categorias padrão semeadas na primeira execução:
`Aluguel/Moradia`, `Internet`, `Luz/Água/Gás`, `Transporte`, `Alimentação`,
`Educação`, `Saúde`, `Streaming/TV/Telefone`, `Academia`, `Outros`, `Crédito`

### entradas

```sql
CREATE TABLE entradas (
  id             INT AUTO_INCREMENT PRIMARY KEY,
  origem         VARCHAR(200) NOT NULL DEFAULT '',
  valor_centavos BIGINT       NOT NULL DEFAULT 0,
  data           DATE         NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### gastos_fixos

```sql
CREATE TABLE gastos_fixos (
  id             INT AUTO_INCREMENT PRIMARY KEY,
  historico      VARCHAR(200) NOT NULL DEFAULT '',
  valor_centavos BIGINT       NOT NULL DEFAULT 0,
  data           DATE         NOT NULL,
  categoria_id   INT          NOT NULL,
  FOREIGN KEY (categoria_id) REFERENCES categorias(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### gastos_variaveis

```sql
CREATE TABLE gastos_variaveis (
  id             INT AUTO_INCREMENT PRIMARY KEY,
  historico      VARCHAR(200) NOT NULL DEFAULT '',
  valor_centavos BIGINT       NOT NULL DEFAULT 0,
  data           DATE         NOT NULL,
  categoria_id   INT          NOT NULL,
  FOREIGN KEY (categoria_id) REFERENCES categorias(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

## Diagrama ER

```
categorias
  id (PK)
  nome

entradas                gastos_fixos            gastos_variaveis
  id (PK)                 id (PK)                 id (PK)
  origem                  historico               historico
  valor_centavos          valor_centavos          valor_centavos
  data                    data                    data
                          categoria_id (FK) ──┐   categoria_id (FK) ──┘
                                              └── categorias.id
```

## Decisões de design

| Decisão | Motivo |
|---|---|
| `BIGINT` para valores | Sem risco de overflow; centavos de transações do dia a dia cabem em `INT`, mas `BIGINT` é mais seguro |
| `ENGINE=InnoDB` | Suporte a FK com integridade referencial |
| `utf8mb4` | Suporte completo a Unicode (emojis, acentos) |
| `UNIQUE` em `categorias.nome` | Evita duplicatas no nível do banco |
| FK sem `ON DELETE CASCADE` explícito | A remoção de categoria via app avisa o usuário; o MySQL recusa a remoção se houver gastos vinculados — proteção de dados |

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
