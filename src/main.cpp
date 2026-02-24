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
            dlg.host(),
            dlg.porta(),
            dlg.banco(),
            dlg.usuario(),
            dlg.senha()
        );

        if (ok) break;

        QMessageBox::critical(
            nullptr,
            "Falha na conexão",
            "Não foi possível conectar ao banco de dados.\n\n"
            "Verifique host, porta, banco, usuário e senha e tente novamente."
        );
    }

    // ── Janela principal ──────────────────────────────────────────────────────
    MainWindow window;
    window.show();

    return app.exec();
}
