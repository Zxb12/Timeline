#include "src/serveur/serveur.h"
#include "src/serveur/client_s.h"
#include "src/shared/shared.h"
#include "src/shared/paquet.h"

#include <QDirIterator>

namespace Serveur
{

CacheEntry &CacheEntry::operator=(const FileHeader &other)
{
    nomReel = other.nomReel;
    noSauvegarde = other.noSauvegarde;
    noVersion = other.noVersion;
    estUnDossier = other.estUnDossier;
    supprime = other.supprime;
    derniereModif = other.derniereModif;

    return *this;
}

FileDescription &FileDescription::operator=(const FileHeader &other)
{
    nomReel = other.nomReel;
    noSauvegarde = other.noSauvegarde;
    noVersion = other.noVersion;
    estUnDossier = other.estUnDossier;
    supprime = other.supprime;
    derniereModif = other.derniereModif;

    return *this;
}

Serveur::Serveur(QObject *parent) :
    QObject(parent), m_stockage(), m_idFichier(0), m_transfertEnCours(false), m_noSauvegarde(0), m_cache()
{
    m_serveur = new QTcpServer(this);

    connect(m_serveur, SIGNAL(newConnection()), this, SLOT(clientConnecte()));
}

Serveur::~Serveur()
{
    sauveCache();
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

    //Chargement du cache
    chargeCache();

    //Création du dossier de stockage si besoin
    if (!m_stockage.exists())
        m_stockage.mkdir(m_stockage.absolutePath());

    console("Serveur démarré (hôte: " + m_serveur->serverAddress().toString() + ", port:" + nbr(m_serveur->serverPort()) + ")");

    return m_serveur->serverPort();
}

void Serveur::creerFichierTest()
{
    console("Création d'un fichier de test: " + QString::number(m_idFichier, 36));

    FileHeader header;
    header.nomReel = "C:/dossier/fichier.txt";
    header.noSauvegarde = m_idFichier;
    header.noVersion = (m_idFichier + 1) / 2;
    header.derniereModif = QDateTime::currentDateTime();
    header.estUnDossier = false;
    header.supprime = false;

    debuteTransfert(header);
    termineTransfert();
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
    if (version != filesystemVersion)
    {
        console("Cache obsolète...");
        cache.close();
        reconstruireCache();
        return;
    }

    //Récupération des infos du cache
    stream >> m_idFichier >> m_noSauvegarde;
    while (!stream.atEnd())
    {
        CacheEntry entry;
        stream >> entry.nomFichier >> entry.nomReel >> entry.noSauvegarde >> entry.noVersion >> entry.derniereModif
               >> entry.estUnDossier >> entry.supprime;
        ajouteAuCache(entry);
        console("Cache: " + entry.nomReel + "(" + entry.nomFichier + "), sauv: " + nbr(entry.noSauvegarde) + ", vers: " + nbr(entry.noVersion));
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
    stream << filesystemVersion << m_idFichier << m_noSauvegarde;
    foreach (CacheEntry entry, m_cache)
    {
        stream << entry.nomFichier << entry.nomReel << entry.noSauvegarde << entry.noVersion << entry.derniereModif
               << entry.estUnDossier << entry.supprime;
    }
}

void Serveur::reconstruireCache()
{
    console("Reconstruction du cache...");

    QString cachePath = m_stockage.absolutePath() + "/" CACHE;
    QString oldCachePath = cachePath + ".old";
    QFile cache(cachePath);
    m_idFichier = 0;
    m_noSauvegarde = 0;

    //Suppression du cache si nécessaire
    if (cache.exists())
    {
        if (QFile::exists(oldCachePath))
            cache.remove(oldCachePath);
        cache.rename(oldCachePath);
        cache.setFileName(cachePath); //On remet le nom du fichier à sa valeur originale (elle a changé lors du renommage)
    }

    //Génération des entrées du cache
    QDirIterator itr(m_stockage.absolutePath(), QStringList("_*.tlf"), QDir::Files);
    while (itr.hasNext())
    {
        itr.next();
        console("Fichier trouvé: " + itr.fileName());

        //Extraction du No de fichier
        QString fileName = itr.fileName();
        fileName.remove(0, 1);
        fileName.remove(".tlf");

        m_idFichier = qMax(m_idFichier, fileName.toULongLong(0, 36));

        //Création de l'entrée de cache.
        QFile file(itr.filePath());
        if (!file.open(QIODevice::ReadOnly))
        {
            console("Impossible d'ouvrir le fichier: " + file.errorString());
            continue;
        }

        //Vérification de la version du fichier
        QDataStream fileStream(&file);
        quint8 octetEnTete, version;
        fileStream >> octetEnTete;
        version = octetEnTete >> 2; //On décale les bits à droite pour effacer les bits dossier et supprimé.
        if (version != filesystemVersion)
        {
            console("Fichier d'une ancienne version(" + nbr(version) + ")");
            continue;
        }

        //Ajout dans le cache
        CacheEntry entry;
        entry.nomFichier = itr.fileName();
        entry.estUnDossier = octetEnTete & (1 << 1);
        entry.supprime = octetEnTete & 1;
        fileStream >> entry.nomReel >> entry.noSauvegarde >> entry.noVersion >> entry.derniereModif;
        ajouteAuCache(entry);

        //Vérification du No de sauvegarde
        m_noSauvegarde = qMax(m_noSauvegarde, entry.noSauvegarde);
    }
    cache.close();

    //On incrémente l'ID de fichier pour qu'il corresponde au prochain fichier
    m_idFichier++;

    console("Reconstruction terminée");
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
    //Sinon, on met à jour le fichier dans le cache
    else
    {
        if (entry.noVersion > m_cache[index].noVersion)
        {
            Q_ASSERT(entry.noSauvegarde >= m_cache[index].noSauvegarde);
            m_cache[index] = entry;
        }
    }
}

quint16 Serveur::noNouvelleVersion(const QString &fichier)
{
    //Recherche le no de version du prochain fichier pour le transfert.
    foreach(CacheEntry entry, m_cache)
    {
        if (entry.nomReel == fichier)
            return entry.noVersion + 1;
    }
    return 0;
}

void Serveur::creeFichierDeTransfert()
{
    //Génération du fichier
    QString nbr = QString::number(m_idFichier, 36);
    QString nomFichier = "_" + QString("0").repeated(13 - nbr.size()) + nbr + ".tlf";
    m_fichierEnTransfert.fichier = new QFile(m_stockage.absolutePath() + "/" + nomFichier, this);
    if (!m_fichierEnTransfert.fichier->open(QIODevice::WriteOnly))
    {
        console("Impossible d'ouvrir/créer le fichier " + nomFichier);
        return;
    }
}

void Serveur::debuteTransfert(const FileHeader &header)
{
    creeFichierDeTransfert();

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
    ajouteAuCache(entry);

    //Fermeture du fichier et fin du transfert
    m_fichierEnTransfert.fichier->close();
    delete m_fichierEnTransfert.fichier;
    m_fichierEnTransfert.fichier = 0;
    m_transfertEnCours = false;
    m_idFichier++;
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
    m_noSauvegarde++;
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
    header.noSauvegarde = m_noSauvegarde;
    header.noVersion = noNouvelleVersion(header.nomReel);

    //Débute le transfert
    debuteTransfert(header);

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
    console(nbr(header.derniereModif < QDateTime::currentDateTime()));
    header.noSauvegarde = m_noSauvegarde;
    header.noVersion = noNouvelleVersion(header.nomReel);
    header.supprime = true;

    //Suppression du fichier
    debuteTransfert(header);
    termineTransfert();

    //Notification de suppression
    Paquet out;
    out << SMSG_FILE_DELETED;
    out >> client;
}


}
