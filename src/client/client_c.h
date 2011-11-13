#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QDir>

#include "src/shared/shared.h"

class Paquet;

namespace Client
{

class Client : public QObject
{
    Q_OBJECT
public:
    Client(QObject *parent = 0);

    //Envoi/réception de fichiers
    void nouvelleSauvegarde();
    void envoie(QFileInfo &);
    void supprime(QString);

    //Handlers
    void handleClientSide(Paquet*);
    void handleError(Paquet*);
    void handleKick(Paquet*);
    void handleHello(Paquet*);
    void handleWaitingForData(Paquet*);
    void handleTransferComplete(Paquet*);
    void handleFileDeleted(Paquet*);

signals:
    void consoleOut(QString);

public slots:
    void connecte(const QString&, quint16);

private slots:
    void console(const QString&);

    void connecte();
    void deconnecte();
    void donneesRecues();
    void erreurSocket(QAbstractSocket::SocketError);

    void donneesEcrites();
    void envoiePaquet();

private:
    //Socket
    QTcpSocket *m_socket;
    quint32 m_taillePaquet;

    //Transfert
    QFile *m_fichier;
    bool m_transfertEnCours;
};

}

#endif // CLIENT_H
