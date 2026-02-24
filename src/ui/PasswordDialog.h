#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(QWidget *parent = nullptr);

    QString host()     const;
    int     porta()    const;
    QString banco()    const;
    QString usuario()  const;
    QString senha()    const;

private slots:
    void onSenhaChanged(const QString &senha);

private:
    struct Requisito {
        QLabel *label;
        bool    ok = false;
    };

    void atualizarRequisito(Requisito &req, bool ok);
    int  calcularForca(const QString &senha) const; // 0–4

    // Conexão MySQL
    QLineEdit *m_host;
    QLineEdit *m_porta;
    QLineEdit *m_banco;
    QLineEdit *m_usuario;

    // Senha
    QLineEdit   *m_senha;
    QPushButton *m_btnMostrar;
    QProgressBar *m_barraForca;
    QLabel       *m_labelForca;

    // Requisitos
    Requisito m_reqTamanho;
    Requisito m_reqMaiuscula;
    Requisito m_reqMinuscula;
    Requisito m_reqNumero;
    Requisito m_reqEspecial;

    QPushButton *m_btnOk;
};
