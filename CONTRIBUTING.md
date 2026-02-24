# Contribuindo

Projeto pessoal — contribuições externas não são esperadas, mas são bem-vindas.

## Requisitos do ambiente

```bash
sudo dnf install \
  qt6-qtbase-devel qt6-qtcharts-devel qt6-qtbase-mysql \
  mysql-devel openssl-devel gcc-c++ cmake make -y
```

## Build de desenvolvimento

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

## Convenções

- **Idioma dos commits:** português
- **Valores monetários:** sempre `qint64` em centavos, nunca `float`/`double`
- **Datas:** `QDate` nos modelos; `dd/MM/yyyy` na UI
- **Queries SQL:** sempre com `bindValue` (sem concatenação de strings)
- **Novos widgets:** emitir `dadosAlterados()` após qualquer escrita no banco
- **Não criar código C++ sem aprovação do mantenedor**

## Estrutura de branches

- `main` — código estável
- features em branches separadas, PR para `main`
