# Documentação de Componentes ("Agents")

Mesmo em arquitetura monolítica, o app é organizado logicamente em componentes:

1. **PasswordDialog**
   - Diálogo de senha com "Mostrar senha".
   - Validação simples (não vazia).

2. **DataManager**
   - Singleton para persistência em JSON (orcamento.json).
   - Caminho: QStandardPaths::AppLocalDataLocation.
   - Salva em centavos (inteiro) para precisão financeira.

3. **MainWindow**
   - Janela principal com QTabWidget.
   - Aba Dashboard: saldo real e gráfico de pizza (QtCharts).
   - Aba Entradas: tabela editável com máscara de moeda (R$ xxx,xx).
   - Total único no final da tabela (calculado em tempo real).
   - Atualização sem mudar de aba.

4. **Funções auxiliares**
   - textoParaCentavos: converte entrada do usuário para centavos (suporta "1" → 1, "12" → 12, "123" → 123).
   - centavosParaTexto: formata para R$ xxx,xx.

## Fluxo principal

- main() → PasswordDialog → DataManager::abrirDados → MainWindow → configurarAbas().
- Edição na tabela → itemChanged → formata valor → salva JSON → atualiza total.

Projeto simples, eficiente e estável para uso pessoal.
