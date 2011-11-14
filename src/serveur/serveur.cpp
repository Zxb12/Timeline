#include "src/serveur/serveur.h"
#include "src/serveur/client_s.h"
#include "src/shared/shared.h"
#include "src/shared/paquet.h"

#include <QDirIterator>

namespace Serveur
{

Serveur::Serveur(QObject *parent) :
    QObject(parent), m_stockage(), m_transfertEnCours(false), m_cache(this)
{
    m_serveur = new QTcpServer(this);

    connect(m_serveur, SIGNAL(newConnection()), this, SLOT(clientConnecte()));
    connect(&m_cache, SIGNAL(consoleOut(QString)), this, SLOT(console(QString)));
}

Serveur::~Serveur()
{
    m_cache.sauveCache();
}

void Serveur::console(const QString &msg)
{
    emit consoleOut(msg);
}

quint16 Serveur::start(QDir stockage, quint16 port = 0)
{
    m_stockage = stockage;

    if (!m_serveur->listen(QHostAddress::Any, port))
    {
        console("Le serveur n'a pas pu démarrer (" + m_serveur->errorString() + ")");
        return 0;
    }

    //Création du dossier de stockage si besoin
    if (!m_stockage.exists())
        m_stockage.mkdir(m_stockage.absolutePath());

    //Chargement du cache
    m_cache.changeDossier(m_stockage);
    m_cache.chargeCache();

    console("Serveur démarré (hôte: " + m_serveur->serverAddress().toString() + ", port:" + nbr(m_serveur->serverPort()) + ")");

    return m_serveur->serverPort();
}

void Serveur::debuteTransfert(FileHeader &header, Client *client)
{
    //Génération des versions et ouverture du fichier
    CacheEntry entry = m_cache.nouveauFichier(header.nomReel);
    header.noSauvegarde = entry.noSauvegarde;
    header.noVersion = entry.noVersion;
    m_fichierEnTransfert.fichier = new QFile(entry.nomFichier, this);

    if (!m_fichierEnTransfert.fichier->open(QIODevice::WriteOnly))
    {
        console("Impossible d'ouvrir/créer le fichier " + entry.nomFichier);
        delete m_fichierEnTransfert.fichier;

        //Notification du client
        Paquet out;
        out << SMSG_ERROR << SERR_COULDNT_CREATE_FILE;
        out >> client;
        return;
    }

    //Ecriture de l'en-tête.
    QDataStream stream(m_fichierEnTransfert.fichier);
    quint8 octetEnTete = (filesystemVersion << 2) | (header.estUnDossier << 1) | (header.supprime);
    stream << octetEnTete << header.nomReel << header.noSauvegarde << header.noVersion << header.derniereModif;

    m_fichierEnTransfert = header;
    m_transfertEnCours = true;
}

void Serveur::termineTransfert()
{
    //Ajoute l'entrée dans le cache
    CacheEntry entry;
    entry = m_fichierEnTransfert;
    entry.nomFichier = m_fichierEnTransfert.fichier->fileName();
    m_cache.ajoute(entry);

    //Fermeture du fichier et fin du transfert
    m_fichierEnTransfert.fichier->close();
    delete m_fichierEnTransfert.fichier;
    m_fichierEnTransfert.fichier = 0;
    m_transfertEnCours = false;
}

void Serveur::annuleTransfert()
{
    if (m_transfertEnCours)
    {
        //Supprime le fichier
        m_fichierEnTransfert.fichier->remove();
        delete m_fichierEnTransfert.fichier;
        m_fichierEnTransfert.fichier = 0;
        m_transfertEnCours = false;
    }
}

void Serveur::clientConnecte()
{
    Client *client = new Client(m_serveur->nextPendingConnection(), this);
    m_clients << client;

    connect(client, SIGNAL(paquetRecu(Paquet*)), this, SLOT(paquetRecu(Paquet*)));
    connect(client, SIGNAL(deconnecte()), this, SLOT(clientDeconnecte()));

    console("Client connecté: " + client->socket()->peerAddress().toString() + ":" + nbr(client->socket()->peerPort()));
}

void Serveur::clientDeconnecte()
{
    //Récupération du client en question
    Client *client = static_cast<Client *>(sender());

    console("Client déconnecté: " + client->socket()->peerAddress().toString() + ":" + nbr(client->socket()->peerPort()));

    //Libération de la mémoire
    m_clients.removeOne(client);
    delete client;
}

void Serveur::paquetRecu(Paquet *in)
{
    //Récupération du client en question
    Client *client = static_cast<Client *>(sender());
    //Traitement du paquet
    quint16 opCode;
    *in >> opCode;

    //Vérification de l'OpCode
    if (opCode < NB_OPCODES)
    {
        OpCodeHandler handler = OpCodeTable[opCode];
        console("Paquet reçu: " + handler.nom + "("+nbr(opCode)+")");

        //Vérification des droits
        if (handler.state & client->sessionState())
            (this->*handler.f)(in, client);
        else
        {
            console("SessionState invalide !");
            kick(client);
        }
    }
    else
    {
        console("Paquet invalide reçu: " + nbr(opCode));
    }

    delete in;
}

void Serveur::kick(Client *client)
{
    console("Client kické: " + client->socket()->peerAddress().toString());
    //Envoie la notification de kick
    Paquet out;
    out << SMSG_KICK;
    out >> client;

    //Déconnexion du client
    client->socket()->disconnectFromHost();
}

void Serveur::handleServerSide(Paquet *, Client *)
{
    console("Reçu un paquet de serveur");
}

void Serveur::handleHello(Paquet *in, Client *client)
{
    QByteArray version;
    *in >> version;

    //Vérification de la version du logiciel client
    if (version != VERSION)
    {
        console("Version du client incorrecte. Serveur: " VERSION ", client: " + version);
        Paquet out;
        out << SMSG_ERROR << SERR_INVALID_VERSION >> client;
        kick(client);
        return;
    }

    //Mise à jour de l'état de la connexion du client
    client->setSessionState(AUTHED);    //TODO Changer ça lors de l'implentation d'un serveur avec gestion de compte

    //Renvoi d'un paquet Hello
    Paquet out;
    out << SMSG_HELLO >> client;
}

void Serveur::handleNewBackup(Paquet *, Client *)
{
    console("Nouvel indice de sauvegarde");

    //Incrémente le no de sauvegarde pour créer la nouvelle sauvegarde
    m_cache.nouvelleSauvegarde();
}

void Serveur::handleInitiateTransfer(Paquet *in, Client *client)
{
    //On vérifie si l'on n'a pas déjà un transfert en cours
    if (m_transfertEnCours)
    {
        Paquet out;
        out << SMSG_ERROR << SERR_TRANSFER_IN_PROGESS;
        out >> client;
        return;
    }
    //Extraction du header
    FileHeader header;
    *in >> header.nomReel;
    *in >> header.estUnDossier;
    *in >> header.derniereModif;

    //Complétion du header avec les données de sauvegarde
    header.supprime = false;

    //Débute le transfert
    debuteTransfert(header, client);

    //Met à jour le statut de session
    client->setSessionState(TRANSFER);

    //Termine le transfert s'il envoie un dossier
    if (header.estUnDossier)
    {
        termineTransfert();
    }
    //Sinon informe le client qu'on est prêts à recevoir les données
    else
    {
        Paquet out;
        out << SMSG_WAITING_FOR_DATA;
        out >> client;
    }
}

void Serveur::handleFinishTransfer(Paquet *, Client *client)
{
    //Achève le transfert
    termineTransfert();

    //Met à jour le statut de session
    client->setSessionState(AUTHED);
}

void Serveur::handleCancelTransfer(Paquet *, Client *client)
{
    //Annule le transfert
    annuleTransfert();

    //Met à jour le statut de session
    client->setSessionState(AUTHED);
}

void Serveur::handleFileData(Paquet *in, Client *)
{
    //Extraction des données
    QByteArray data;
    *in >> data;

    //Ecriture dans le fichier
    m_fichierEnTransfert.fichier->write(data);
}

void Serveur::handleDeleteFile(Paquet *in, Client *client)
{
    //Extraction du fichier à supprimer
    FileHeader header;
    *in >> header.nomReel;
    *in >> header.estUnDossier;

    //Rempmlissage du header
    header.derniereModif = QDateTime();
    header.supprime = true;

    //Suppression du fichier
    debuteTransfert(header, client);
    termineTransfert();

    //Notification de suppression
    Paquet out;
    out << SMSG_FILE_DELETED;
    out >> client;
}

void Serveur::handleFileList(Paquet *in, Client *client)
{
    //Récupération du No de sauvegarde pour lequel renvoyer la liste
    quint16 noSauv;
    *in >> noSauv;

    QList<CacheEntry> liste;
    liste = m_cache.listeFichiers(noSauv);

    //Envoi de la liste
    Paquet out;
    out << SMSG_FILE_LIST << (quint32) liste.size();
    foreach(CacheEntry entry, liste)
    {
        out << entry.nomReel << entry.noSauvegarde << entry.noVersion << entry.derniereModif << entry.estUnDossier;
    }
    out >> client;
}

}
