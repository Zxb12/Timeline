#include "src/client/client_c.h"

#include "src/client/opcode_c.h"
#include "src/shared/paquet.h"

#include <QDateTime>

namespace Client
{

Client::Client(QObject *parent) :
    QObject(parent), m_taillePaquet(0), m_fichier(0), m_transfertEnCours(false)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(readyRead()),                                  this, SLOT(donneesRecues()));
    connect(m_socket, SIGNAL(bytesWritten(qint64)),                         this, SLOT(donneesEcrites()));
    connect(m_socket, SIGNAL(connected()),                                  this, SLOT(connecte()));
    connect(m_socket, SIGNAL(disconnected()),                               this, SLOT(deconnecte()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),          this, SLOT(erreurSocket(QAbstractSocket::SocketError)));
}

void Client::console(const QString &msg)
{
    emit consoleOut(msg);
}


void Client::connecte()
{
    console("Connexion réussie !");

    //On envoie le Hello
    Paquet out;
    out << CMSG_HELLO;
    out << QByteArray(VERSION);
    out >> m_socket;
}

void Client::deconnecte()
{
    console("Déconnecté du serveur.");
}

void Client::donneesRecues()
{
    QDataStream stream(m_socket);

    //Récupération de la taille du paquet
    if (m_taillePaquet == 0)
    {
        if (m_socket->bytesAvailable() < sizeof m_taillePaquet)
            return;

        stream >> m_taillePaquet;
    }

    //Récupération du reste du paquet
    if (m_socket->bytesAvailable() < m_taillePaquet)
        return;

    //On lit la socket pour la taille d'un paquet et on stocke.
    Paquet *in = new Paquet(m_socket->read(m_taillePaquet));

    //Remise à zéro de la taille du paquet
    m_taillePaquet = 0;

    //Traitement du paquet
    quint16 opCode;
    *in >> opCode;

    //Vérification de l'OpCode
    if (opCode < NB_OPCODES)
    {
        OpCodeHandler handler = OpCodeTable[opCode];
        console("Paquet reçu: " + handler.nom + "("+nbr(opCode)+")");
        (this->*handler.f)(in);
    }
    else
    {
        console("Paquet invalide reçu: " + nbr(opCode));
    }

    //Libération du paquet
    delete in;

    //S'il nous reste quelque chose dans la socket, on relance la fonction.
    if (m_socket->bytesAvailable())
        donneesRecues();
}

void Client::erreurSocket(QAbstractSocket::SocketError erreur)
{
    switch(erreur) // On affiche un message différent selon l'erreur qu'on nous indique
    {
    //TODO: afficher un message plus explicite
    default:
        console(m_socket->errorString());
        break;
    }
}

void Client::connecte(const QString &adresse, quint16 port)
{
    //Déconnecte la socket si elle est connectée
    if (m_socket->state() == QTcpSocket::ConnectedState)
        m_socket->abort();

    console("Connexion en cours...");
    m_socket->connectToHost(adresse, port);
}

void Client::nouvelleSauvegarde()
{
    Paquet out;
    out << CMSG_NEW_BACKUP;
    out >> m_socket;
}

void Client::envoie(QFileInfo &fichier)
{
    //Si un transfert est déjà en cours, on quitte !
    if (m_transfertEnCours)
        return;

    //Si le fichier est un dossier, m_fichier = 0
    if (fichier.isFile())
    {
        //Ouverture du fichier
        m_fichier = new QFile(fichier.absoluteFilePath(), this);
        if (!m_fichier->open(QIODevice::ReadOnly))
        {
            console("Impossible d'ouvrir le fichier " + fichier.filePath());
            return;
        }
    }

    //Début du transfert
    Paquet out;
    out << CMSG_INITIATE_TRANSFER << fichier.absoluteFilePath() << fichier.isDir() << fichier.lastModified();
    out >> m_socket;
}

void Client::supprime(QString adresse)
{
    //Supprime un fichier du serveur
    Paquet out;
    out << CMSG_DELETE_FILE << adresse;
    out >> m_socket;
}

void Client::donneesEcrites()
{
    console("Données écrites, restant en buffer: " + nbr(m_socket->bytesToWrite()));

    //Si l'on est en transfert et que le paquet précédent a été complètement écrit, on écrit le suivant
    if (m_socket->bytesToWrite() == 0 && m_transfertEnCours)
        envoiePaquet();
}

void Client::envoiePaquet()
{

    //On envoie un fichier
    QByteArray data = m_fichier->read(60000);

    Paquet out;
    out << CMSG_FILE_DATA << data;
    out >> m_socket;

    //On vérifie si l'on a envoyé tout le fichier
    if (m_fichier->atEnd())
    {
        out.clear();
        out << CMSG_FINISH_TRANSFER;
        out >> m_socket;
        m_transfertEnCours = false;
    }
}

void Client::handleClientSide(Paquet *)
{
    console("Reçu un paquet de client");
}

void Client::handleError(Paquet *in)
{
    Error err;
    *in >> err;

    switch (err)
    {
    case SERR_INVALID_VERSION:
    {
        QByteArray versionServeur;
        *in >> versionServeur;
        console("Version invalide. Client: " VERSION ", serveur: " + versionServeur);

        //Impossible de continuer, on se déconnecte
        m_socket->abort();
        break;
    }
    case SERR_COULDNT_CREATE_FILE:
    {
        console("Le serveur n'a pas pu créer le fichier pour le transfert");
        break;
    }
    case SERR_TRANSFER_IN_PROGESS:
    {
        console("Il y a déjà un transfert en cours !");
        if (m_fichier && !m_transfertEnCours) //On désalloue le fichier si aucun transfert n'a effectivement commencé
            delete m_fichier;
        break;
    }
    default:
        console("Erreur renvoyée par le serveur: " + nbr(err));
        break;
    }
}

void Client::handleKick(Paquet *)
{
    console("Le serveur nous a kickés");
}

void Client::handleHello(Paquet *)
{
    console("Reçu Hello du serveur");
}

void Client::handleWaitingForData(Paquet *)
{
    console("Serveur prêt à recevoir les données");
    m_transfertEnCours = true;
    envoiePaquet();
}

void Client::handleTransferComplete(Paquet *)
{
    delete m_fichier;
    m_fichier = 0;
}

void Client::handleFileDeleted(Paquet*)
{
    console("Fichier supprimé du serveur.");
}


}
