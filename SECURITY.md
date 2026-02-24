# Segurança

## Medidas implementadas

### Banco de dados (SQLite + AES-256-CBC)
- O banco SQLite é armazenado criptografado em disco (`data/<usuario>.enc`)
- Sem servidor de banco de dados — superfície de ataque reduzida
- Todas as queries usam `QSqlQuery::bindValue` — sem SQL injection possível
- `PRAGMA foreign_keys = ON` — integridade referencial ativa

### Criptografia de arquivo (CryptoHelper)
- AES-256-CBC via OpenSSL EVP
- Derivação de chave: PBKDF2-SHA256 com **600.000 iterações** (alinhado com NIST SP 800-132, 2026)
- Salt aleatório via `RAND_bytes` a cada operação de escrita
- Escrita atômica: `.new` → `rename` → remove old — sem corrupção em caso de falha

### Arquivo temporário
- `.db` temporário fica apenas em memória durante o uso (`PRAGMA journal_mode = MEMORY`)
- Deletado imediatamente ao encerrar o app
- Nome oculto (`.` prefixo) para reduzir visibilidade acidental

### Senha do app
- Validação de força: mínimo 8 caracteres, maiúscula, minúscula, número e símbolo
- Checklist visual com ✓/✗ em tempo real
- Senha nunca armazenada em disco — apenas derivada via PBKDF2

## Reporte de vulnerabilidades

Abra uma issue privada no GitHub ou envie e-mail ao mantenedor.
Não abra issues públicas para vulnerabilidades de segurança.
