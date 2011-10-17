#include "src/serveur/serveur.h"
#include "src/serveur/client_s.h"
#include "src/shared/shared.h"
#include "src/shared/paquet.h"

namespace Serveur
{

Serveur::Serveur(QObject *parent) :
    QObject(parent)
{
    m_serveur = new QTcpServer(this);

    connect(m_serveur, SIGNAL(newConnection()), this, SLOT(clientConnecte()));
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

    chargeCache();

    //Création du dossier de stockage si besoin
    if (!m_stockage.exists())
        m_stockage.mkdir(m_stockage.absolutePath());

    console("Serveur démarré (hôte: " + m_serveur->serverAddress().toString() + ", port:" + nbr(m_serveur->serverPort()) + ")");

    return m_serveur->serverPort();
}

void Serveur::chargeCache()
{
    QFile cache(m_stockage.absolutePath() + "/" CACHE);
    m_cache.clear();

    //On vérifie que le cache existe
    if (!cache.exists())
    {
        reconstruireCache();
        return;
    }

    if (!cache.open(QIODevice::ReadOnly))
    {
        console("Impossible d'ouvrir le fichier de cache. CODE A METTRE A JOUR");
        return;
    }

    //Extraction de la version du cache
    QDataStream stream(&cache);
    quint8 version;
    stream >> version;

    //Vérification de la version du cache
    if (version != CACHE_VERSION)
    {
        cache.close();
        reconstruireCache();
        return;
    }

    //Récupération des infos du cache
    while (!stream.atEnd())
    {
        QString nomFichier;
        quint16 noSauvegarde, noVersion;
        QDateTime derniereModif;
        stream >> nomFichier >> noSauvegarde >> noVersion >> derniereModif;
        CacheEntry entry = {nomFichier, noSauvegarde, noVersion, derniereModif};
        m_cache << entry;
    }

}

void Serveur::reconstruireCache()
{
    console("Reconstruction du cache...");

    QString cachePath = m_stockage.absolutePath() + "/" CACHE;
    QString oldCachePath = cachePath + ".old";
    QFile cache(cachePath);

    //Suppression du cache si nécessaire
    if (cache.exists())
    {
        if (QFile::exists(oldCachePath))
            cache.remove(oldCachePath);
        cache.rename(oldCachePath);
        cache.setFileName(cachePath); //On remet le nom du fichier à sa valeur originale (elle a changé lors du renommage)
    }

    if (!cache.open(QIODevice::WriteOnly))
        return;

    QDataStream stream(&cache);
    stream << quint8(CACHE_VERSION);

    //TODO ajouter au cache les fichiers présents

    cache.close();
    console("Reconstruction terminée");

    chargeCache();
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
    client->setSessionState(CHECKED);

    //Renvoi d'un paquet Hello
    Paquet out;
    out << SMSG_HELLO >> client;
}


}
