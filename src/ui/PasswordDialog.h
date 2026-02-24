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

    QString nome()  const;
    QString senha() const;

private:
    struct Requisito {
        QLabel *label;
        bool    ok = false;
    };

    void onSenhaChanged(const QString &senha);
    void atualizarRequisito(Requisito &req, bool ok, bool ativo);
    void atualizarBotao();
    int  calcularForca(const QString &senha) const; // 0–5

    QLineEdit    *m_nome;
    QLineEdit    *m_senha;
    QPushButton  *m_btnMostrar;
    QProgressBar *m_barraForca;
    QLabel       *m_labelForca;

    Requisito m_reqTamanho;
    Requisito m_reqMaiuscula;
    Requisito m_reqMinuscula;
    Requisito m_reqNumero;
    Requisito m_reqEspecial;

    QPushButton *m_btnOk;
};
