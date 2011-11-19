#ifndef FENPRINCIPALE_H
#define FENPRINCIPALE_H

#include <QMainWindow>

#include "src/client/client_c.h"
#include "src/serveur/serveur.h"

namespace Ui {
    class FenPrincipale;
}

class FenPrincipale : public QMainWindow
{
    Q_OBJECT

public:
    FenPrincipale(QWidget *parent = 0);
    ~FenPrincipale();

private slots:
    void consoleClient(QString);
    void consoleServeur(QString);
    void listeRecue(QList<FileHeader>);

    void on_btn1_clicked();
    void on_btn2_clicked();
    void on_btn3_clicked();

private:
    Ui::FenPrincipale *ui;

    Client::Client *m_client;
    Serveur::Serveur *m_serveur;

    //Transfert
    QStringList m_dossiersDeTransfert;
    bool m_enAttenteDeTransfert;
};

#endif // FENPRINCIPALE_H
