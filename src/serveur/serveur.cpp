#include "src/serveur/serveur.h"
#include "src/serveur/client_s.h"
#include "src/shared/shared.h"
#include "src/shared/paquet.h"

#include <QDirIterator>

namespace Serveur
{

Serveur::Serveur(QObject *parent) :
    QObject(parent), m_stockage(), m_idFichier(0), m_cache()
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
        console("Le serveur n'a pas pu d�marrer (" + m_serveur->errorString() + ")");
        return 0;
    }

    //Chargement du cache
    chargeCache();

    //Cr�ation du dossier de stockage si besoin
    if (!m_stockage.exists())
        m_stockage.mkdir(m_stockage.absolutePath());

    console("Serveur d�marr� (h�te: " + m_serveur->serverAddress().toString() + ", port:" + nbr(m_serveur->serverPort()) + ")");

    return m_serveur->serverPort();
}

void Serveur::creerFichierTest()
{
    QString fileName = "_0000000000000.tlf";
    QString nbr = QString::number(m_idFichier, 36);
    fileName.replace(13 + 1 - nbr.size(), nbr.size(), nbr);
    QFile file(m_stockage.absolutePath() + "/" + fileName);

    file.open(QIODevice::WriteOnly);
    QDataStream stream(&file);
    stream << quint8(filesystemVersion << 2) << QString("Nom du fichier.bak") << (quint16) 2 << (quint16) 1 << QDateTime::currentDateTime();
    file.close();

    m_idFichier++;
}

void Serveur::chargeCache()
{
    QFile cache(m_stockage.absolutePath() + "/" CACHE);
    m_cache.clear();

    //On v�rifie que le cache existe
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

    //V�rification de la version du cache
    if (version != filesystemVersion)
    {
        cache.close();
        reconstruireCache();
        return;
    }

    //R�cup�ration des infos du cache
    stream >> m_idFichier;
    while (!stream.atEnd())
    {
        CacheEntry entry;
        stream >> entry.nomFichier >> entry.nomReel >> entry.noSauvegarde >> entry.noVersion >> entry.derniereModif;
        ajouteAuCache(entry);
    }
    cache.close();
}

void Serveur::sauveCache()
{
    //Ouverture du cache
    QFile cache(m_stockage.absolutePath() + "/" CACHE);
    if (!cache.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        console("Impossible de sauvegarder le cache: " + cache.errorString());
        return;
    }

    //Ecriture du cache
    QDataStream stream(&cache);
    stream << filesystemVersion << m_idFichier;
    foreach (CacheEntry entry, m_cache)
    {
        stream << entry.nomFichier << entry.nomReel << entry.noSauvegarde << entry.noVersion << entry.derniereModif;
    }
}

void Serveur::reconstruireCache()
{
    console("Reconstruction du cache...");

    QString cachePath = m_stockage.absolutePath() + "/" CACHE;
    QString oldCachePath = cachePath + ".old";
    QFile cache(cachePath);
    m_idFichier = 0;

    //Suppression du cache si n�cessaire
    if (cache.exists())
    {
        if (QFile::exists(oldCachePath))
            cache.remove(oldCachePath);
        cache.rename(oldCachePath);
        cache.setFileName(cachePath); //On remet le nom du fichier � sa valeur originale (elle a chang� lors du renommage)
    }

    //G�n�ration des entr�es du cache
    QDirIterator itr(m_stockage.absolutePath(), QStringList("_*.tlf"), QDir::Files);
    while (itr.hasNext())
    {
        itr.next();
        console("Fichier trouv�: " + itr.fileName());

        //Extraction du No de fichier
        QString fileName = itr.fileName();
        fileName.remove(0, 1);
        fileName.remove(".tlf");

        m_idFichier = qMax(m_idFichier, fileName.toULongLong(0, 36));

        //Cr�ation de l'entr�e de cache.
        QFile file(itr.filePath());
        if (!file.open(QIODevice::ReadOnly))
        {
            console("Impossible d'ouvrir le fichier: " + file.errorString());
            continue;
        }

        //V�rification de la version du fichier
        QDataStream fileStream(&file);
        quint8 octetEnTete, version;
        fileStream >> octetEnTete;
        version = octetEnTete >> 2; //On d�cale les bits � droite pour effacer les bits dossier et supprim�.
        if (version != filesystemVersion)
        {
            console("Fichier d'une ancienne version(" + nbr(version) + ")");
            continue;
        }

        CacheEntry entry;
        entry.nomFichier = itr.fileName();
        fileStream >> entry.nomReel >> entry.noSauvegarde >> entry.noVersion >> entry.derniereModif;
        ajouteAuCache(entry);

    }
    cache.close();

    //On incr�mente l'ID de fichier pour qu'il corresponde au prochain fichier
    m_idFichier++;

    console("Reconstruction termin�e");
    sauveCache();
}

void Serveur::ajouteAuCache(const CacheEntry &entry)
{
    //Recherche du fichier dans le cache
    int index = m_cache.indexOf(entry);

    //Si le fichier n'est pas dans le cache, on l'y ajoute
    if (index == -1)
    {
        m_cache << entry;
    }
    //Sinon, on met � jour le fichier dans le cache
    else
    {
        if (entry.noVersion > m_cache[index].noVersion)
        {
            Q_ASSERT(entry.noSauvegarde > m_cache[index].noSauvegarde);
            m_cache[index] = entry;
        }
    }

}

void Serveur::clientConnecte()
{
    Client *client = new Client(m_serveur->nextPendingConnection(), this);
    m_clients << client;

    connect(client, SIGNAL(paquetRecu(Paquet*)), this, SLOT(paquetRecu(Paquet*)));
    connect(client, SIGNAL(deconnecte()), this, SLOT(clientDeconnecte()));

    console("Client connect�: " + client->socket()->peerAddress().toString() + ":" + nbr(client->socket()->peerPort()));
}

void Serveur::clientDeconnecte()
{
    //R�cup�ration du client en question
    Client *client = static_cast<Client *>(sender());

    console("Client d�connect�: " + client->socket()->peerAddress().toString() + ":" + nbr(client->socket()->peerPort()));

    //Lib�ration de la m�moire
    m_clients.removeOne(client);
    delete client;
}

void Serveur::paquetRecu(Paquet *in)
{
    //R�cup�ration du client en question
    Client *client = static_cast<Client *>(sender());
    //Traitement du paquet
    quint16 opCode;
    *in >> opCode;

    //V�rification de l'OpCode
    if (opCode < NB_OPCODES)
    {
        OpCodeHandler handler = OpCodeTable[opCode];
        console("Paquet re�u: " + handler.nom + "("+nbr(opCode)+")");

        //V�rification des droits
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
        console("Paquet invalide re�u: " + nbr(opCode));
    }

    delete in;
}

void Serveur::kick(Client *client)
{
    console("Client kick�: " + client->socket()->peerAddress().toString());
    //Envoie la notification de kick
    Paquet out;
    out << SMSG_KICK;
    out >> client;

    //D�connexion du client
    client->socket()->disconnectFromHost();
}

void Serveur::handleServerSide(Paquet *, Client *)
{
    console("Re�u un paquet de serveur");
}

void Serveur::handleHello(Paquet *in, Client *client)
{
    QByteArray version;
    *in >> version;

    //V�rification de la version du logiciel client
    if (version != VERSION)
    {
        console("Version du client incorrecte. Serveur: " VERSION ", client: " + version);
        Paquet out;
        out << SMSG_ERROR << SERR_INVALID_VERSION >> client;
        kick(client);
        return;
    }

    //Mise � jour de l'�tat de la connexion du client
    client->setSessionState(CHECKED);

    //Renvoi d'un paquet Hello
    Paquet out;
    out << SMSG_HELLO >> client;
}


}
