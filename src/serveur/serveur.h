#ifndef SERVEUR_H
#define SERVEUR_H

#include <QObject>
#include <QTcpServer>
#include <QDir>
#include <QDateTime>

#define CACHE "Timeline.cache"

class Paquet;

namespace Serveur
{

class Client;
const quint8 filesystemVersion = 3;

struct CacheEntry
{
    QString nomFichier, nomReel;
    quint16 noSauvegarde, noVersion;
    QDateTime derniereModif;

    bool operator==(const CacheEntry &other)
    { return nomReel == other.nomReel; }
};

class Serveur : public QObject
{
    Q_OBJECT
public:
    Serveur(QObject *parent = 0);


    quint16 start(QDir, quint16);

    //Handlers
    void handleHello(Paquet *, Client *);
    void handleServerSide(Paquet *, Client *);

public slots:
    void creerFichierTest();

private:
    //Helpers
    void chargeCache();
    void sauveCache();
    void reconstruireCache();
    void ajouteAuCache(const CacheEntry &);
    void kick(Client *);

signals:
    void consoleOut(QString);

private slots:
    void console(const QString&);
    void clientConnecte();
    void clientDeconnecte();
    void paquetRecu(Paquet *);

private:
    //Serveur TCP-IP
    QTcpServer *m_serveur;
    QList<Client *> m_clients;

    //Dossier de stockage
    QDir m_stockage;
    quint64 m_idFichier;

    //Cache de fichiers
    QList<CacheEntry> m_cache;
};

}

#endif // SERVEUR_H
