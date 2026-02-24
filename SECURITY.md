# Segurança

## Medidas implementadas

### Banco de dados
- Todas as queries usam `QSqlQuery::bindValue` — sem SQL injection possível
- Credenciais nunca armazenadas em disco pelo app (usuário digita a cada execução)

### Criptografia (CryptoHelper)
- AES-256-CBC via OpenSSL EVP
- Derivação de chave: PBKDF2-SHA256 com **600.000 iterações** (alinhado com NIST SP 800-132, 2026)
- Salt aleatório via `RAND_bytes` a cada operação de escrita
- Disponível para exportação criptografada de dados futura

### Senha do app
- Validação de força: mínimo 8 caracteres, maiúscula, minúscula, número e símbolo
- Barra de força visual com 4 níveis

## Reporte de vulnerabilidades

Abra uma issue privada no GitHub ou envie e-mail ao mantenedor.
Não abra issues públicas para vulnerabilidades de segurança.
