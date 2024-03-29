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
    ~Client();

    //Handlers
    void handleClientSide(Paquet*);
    void handleError(Paquet*);
    void handleKick(Paquet*);
    void handleHello(Paquet*);
    void handleWaitingForData(Paquet*);
    void handleTransferComplete(Paquet*);
    void handleFileDeleted(Paquet*);
    void handleFileList(Paquet*);
    void handleFinishTransfer(Paquet*);
    void handleCancelTransfer(Paquet*);
    void handleFileData(Paquet*);

public slots:
    void connecte(const QString&, quint16);
    void setDossierRecuperation(const QDir &dossier) { m_dossierRecuperation = dossier; }

    //Envoi/réception de fichiers
    void nouvelleSauvegarde();
    void envoie(const QFileInfo &);
    void supprime(const QString &);
    void listeFichiers(quint16 = -1);
    void recupereFichier(const FileHeader &);

signals:
    void consoleOut(QString);
    void listeRecue(QList<FileHeader>);

private slots:
    void console(const QString&);

    void connecte();
    void deconnecte();
    void donneesRecues();
    void erreurSocket(QAbstractSocket::SocketError);

    void donneesEcrites();
    void envoiePaquet();

private:
    void transfertSuivant();
    void recupereSuivant();

private:
    //Socket
    QTcpSocket *m_socket;
    quint32 m_taillePaquet;

    //Transfert
    QFile *m_fichier;
    EtatTransfert m_etatTransfert;
    QList<QFileInfo> m_listeTransfert;
    QStringList m_listeSuppressions;
    QList<FileHeader> m_listeRecuperations;
    QDir m_dossierRecuperation;

};

}

#endif // CLIENT_H
