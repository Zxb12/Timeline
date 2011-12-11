#include "src/client/client_c.h"

#include "src/client/opcode_c.h"
#include "src/shared/paquet.h"

#include <QDateTime>

namespace Client
{

Client::Client(QObject *parent) :
    QObject(parent), m_taillePaquet(0), m_fichier(0), m_etatTransfert(AUCUN_TRANSFERT)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(readyRead()),                                  this, SLOT(donneesRecues()));
    connect(m_socket, SIGNAL(bytesWritten(qint64)),                         this, SLOT(donneesEcrites()));
    connect(m_socket, SIGNAL(connected()),                                  this, SLOT(connecte()));
    connect(m_socket, SIGNAL(disconnected()),                               this, SLOT(deconnecte()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),          this, SLOT(erreurSocket(QAbstractSocket::SocketError)));

}

Client::~Client()
{
    if (m_socket->isOpen())
        m_socket->abort();
}

void Client::connecte(const QString &adresse, quint16 port)
{
    //Déconnecte la socket si elle est connectée
    if (m_socket->state() == QTcpSocket::ConnectedState)
        m_socket->abort();

    console("Connexion en cours...");
    m_socket->connectToHost(adresse, port);
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

    if (m_etatTransfert == SERVEUR_VERS_CLIENT)
    {
        m_fichier->remove();
        delete m_fichier;
        m_fichier = 0;
        m_etatTransfert = AUCUN_TRANSFERT;
    }
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
//        console("Paquet reçu: " + handler.nom + "("+nbr(opCode)+")");
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

void Client::nouvelleSauvegarde()
{
    Paquet out;
    out << CMSG_NEW_BACKUP;
    out >> m_socket;
}

void Client::envoie(const QFileInfo &fichier)
{
    //On ajoute le fichier à la liste et on lance le transfert si nécessaire
    m_listeTransfert.append(fichier);
    if (m_etatTransfert == AUCUN_TRANSFERT)
        transfertSuivant();
}

void Client::supprime(const QString &adresse)
{
    //On ajoute le fichier à la liste et on lance la suppression si nécessaire
    m_listeSuppressions.append(adresse);
    if (m_etatTransfert == AUCUN_TRANSFERT)
        transfertSuivant();
}

void Client::listeFichiers(quint16 noSauvegarde)
{
    console("Récupération de la liste des fichiers");
    Paquet out;
    out << CMSG_FILE_LIST << noSauvegarde;
    out >> m_socket;
}

void Client::recupereFichier(const FileHeader &header)
{
    m_listeRecuperations.append(header);

    if (m_etatTransfert == AUCUN_TRANSFERT)
        recupereSuivant();
}

void Client::donneesEcrites()
{
    //Si l'on est en transfert et que le paquet précédent a été complètement écrit, on écrit le suivant
    if (m_socket->bytesToWrite() == 0 && m_etatTransfert == CLIENT_VERS_SERVEUR)
        envoiePaquet();
}

void Client::envoiePaquet()
{
    QByteArray data = m_fichier->read(MAX_PACKET_SIZE);

    Paquet out;
    out << CMSG_FILE_DATA << data;
    out >> m_socket;

    //On vérifie si l'on a envoyé tout le fichier
    if (m_fichier->atEnd())
    {
        out.clear();
        out << CMSG_FINISH_TRANSFER;
        out >> m_socket;
        m_etatTransfert = EN_ATTENTE; //On attend la notification de fin de transfert, il ne faut pas commencer le transfert suivant en attendant !
    }
}

void Client::transfertSuivant()
{
    //Si un transfert est déjà en cours, on quitte !
    if (m_etatTransfert != AUCUN_TRANSFERT)
        return;

    //Récupération du fichier suivant
    if (m_listeTransfert.size() == 0)
    {
        if (!m_listeSuppressions.isEmpty())
        {
            foreach(QString fichier, m_listeSuppressions)
            {
                //Supprime un fichier du serveur
                Paquet out;
                out << CMSG_DELETE_FILE << fichier;
                out >> m_socket;
            }
            m_listeSuppressions.clear();
            console("Transferts terminés");
        }
        return;
    }

    QFileInfo fichier = m_listeTransfert.first();
    m_listeTransfert.pop_front();

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

    console("Début du transfert de " + fichier.filePath());

    //Début du transfert
    Paquet out;
    out << CMSG_INITIATE_TRANSFER << fichier.absoluteFilePath() << fichier.isDir() << fichier.lastModified();
    out >> m_socket;
    m_etatTransfert = EN_ATTENTE;
}

void Client::recupereSuivant()
{
    //Vérifications
    if (m_etatTransfert != AUCUN_TRANSFERT || m_listeRecuperations.size() == 0)
        return;


    //Création du dossier si nécessaire
    FileHeader fichier = m_listeRecuperations.first();
    m_listeRecuperations.pop_front();
    QString nomFichier = fichier.nomClient;
    nomFichier.remove(":"); //Supprime le caractère ":" des lecteurs (sur Windows)
    nomFichier = m_dossierRecuperation.absolutePath() + "/" + nomFichier;
    console("Réception de: " + fichier.nomClient);

    if (fichier.estUnDossier)
    {
        m_dossierRecuperation.mkpath(nomFichier.section("/", 0, nomFichier.count("/")));
        recupereSuivant();
        return;
    }
    else
        m_dossierRecuperation.mkpath(nomFichier.section("/", 0, nomFichier.count("/") - 1));

    //Ouverture du fichier local
    m_fichier = new QFile(nomFichier, this);
    if (!m_fichier->open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        console("Impossible de créer le fichier local " + m_fichier->fileName());
        delete m_fichier;
        m_fichier = 0;
        return;
    }

    //Fichier ouvert, on peut commencer le transfert
    Paquet out;
    out << CMSG_RECOVER_FILE << fichier.nomClient << fichier.noSauvegarde;
    out >> m_socket;
    m_etatTransfert = SERVEUR_VERS_CLIENT;
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
    case SERR_COULDNT_OPEN_FILE:
    {
        console("Le serveur n'a pas pu ouvrir le fichier sauvegardé");
        break;
    }
    case SERR_TRANSFER_IN_PROGESS:
    {
        console("Il y a déjà un transfert en cours !");
        if (m_fichier && m_etatTransfert == SERVEUR_VERS_CLIENT) //On désalloue le fichier si aucun transfert n'a effectivement commencé
            delete m_fichier;
        break;
    }
    case SERR_FILE_DOESNT_EXIST:
    {
        console("Le fichier n'existe pas sur le serveur.");
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
    //Début effectif du transfert
    m_etatTransfert = CLIENT_VERS_SERVEUR;
    envoiePaquet();
}

void Client::handleTransferComplete(Paquet *)
{
    if (m_fichier)
    {
        m_fichier->close();
        delete m_fichier;
        m_fichier = 0;
    }
    m_etatTransfert = AUCUN_TRANSFERT;
    transfertSuivant();
}

void Client::handleFileDeleted(Paquet *)
{
    transfertSuivant();
}

void Client::handleFileList(Paquet *in)
{
    quint32 nbFichiers;
    *in >> nbFichiers;
    QList<FileHeader> liste;
    liste.reserve(nbFichiers);

    for (quint32 i = 0; i < nbFichiers; i++)
    {
        //Extraction des informations
        FileHeader header;
        *in >> header.nomClient >> header.noSauvegarde >> header.noVersion >> header.derniereModif >> header.estUnDossier;
        liste.append(header);
    }

    //Envoi des données
    emit listeRecue(liste);
}

void Client::handleFinishTransfer(Paquet *)
{
    m_fichier->close();
    delete m_fichier;
    m_fichier = 0;
    m_etatTransfert = AUCUN_TRANSFERT;
    recupereSuivant();
}

void Client::handleCancelTransfer(Paquet *)
{
    m_fichier->remove();
    delete m_fichier;
    m_fichier = 0;
    m_etatTransfert = AUCUN_TRANSFERT;
    recupereSuivant();
}

void Client::handleFileData(Paquet *in)
{
    QByteArray data;
    *in >> data;
    m_fichier->write(data);
}

}
