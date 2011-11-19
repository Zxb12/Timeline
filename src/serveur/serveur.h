#ifndef SERVEUR_H
#define SERVEUR_H

#include "src/shared/shared.h"
#include "src/serveur/defines.h"
#include "src/serveur/cache.h"

#include <QObject>
#include <QTcpServer>
#include <QDir>
#include <QDateTime>

class Paquet;

namespace Serveur
{

class Client;

class Serveur : public QObject
{
    Q_OBJECT
public:
    Serveur(QObject *parent = 0);
    ~Serveur();

    quint16 start(QDir, quint16);

    //Handlers
    void handleHello(Paquet*, Client*);
    void handleServerSide(Paquet*, Client*);
    void handleNewBackup(Paquet*, Client*);
    void handleInitiateTransfer(Paquet*, Client*);
    void handleFinishTransfer(Paquet*, Client*);
    void handleCancelTransfer(Paquet*, Client*);
    void handleFileData(Paquet*, Client*);
    void handleDeleteFile(Paquet*, Client*);
    void handleFileList(Paquet*, Client*);
    void handleRecoverFile(Paquet*, Client*);

private:
    //Helpers transfert
    void debuteTransfert(FileHeader &, Client *);
    void termineTransfert(Client *);
    void annuleTransfert(Client *);
    void envoiePaquet(Client *);

    //Helpers réseau
    void kick(Client *);

signals:
    void consoleOut(QString);

private slots:
    void console(const QString&);
    void clientConnecte();
    void clientDeconnecte();
    void paquetRecu(Paquet *);
    void paquetEcrit();

private:
    //Serveur TCP-IP
    QTcpServer *m_serveur;
    QList<Client *> m_clients;

    //Dossier de stockage
    QDir m_stockage;
    FileDescription m_fichierEnTransfert;
    EtatTransfert m_etatTransfert;

    //Cache de fichiers
    Cache m_cache;
};

}

#endif // SERVEUR_H
