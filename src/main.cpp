#include "ui/MainWindow.h"
#include "ui/PasswordDialog.h"
#include "ui/Theme.h"
#include "core/DatabaseManager.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Orçamento Pessoal");
    app.setOrganizationName("petrinhu");

    Theme::aplicar();

    // ── Autenticação + conexão ────────────────────────────────────────────────
    while (true) {
        PasswordDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return 0;

        const bool ok = DatabaseManager::instance().conectar(
            dlg.nome(),
            dlg.senha()
        );

        if (ok) break;

        QMessageBox::critical(
            nullptr,
            "Falha ao abrir dados",
            "Não foi possível abrir os dados do usuário \"" + dlg.nome() + "\".\n\n"
            "Senha incorreta ou arquivo corrompido."
        );
    }

    // ── Janela principal ──────────────────────────────────────────────────────
    MainWindow window;
    window.show();

    return app.exec();
}
