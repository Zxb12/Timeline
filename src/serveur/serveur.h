#ifndef SERVEUR_H
#define SERVEUR_H

#include <QObject>
#include <QTcpServer>
#include <QDir>
#include <QDateTime>

#define CACHE "Timeline.cache"
#define CACHE_VERSION 1

class Paquet;

namespace Serveur
{

class Client;

struct CacheEntry
{
    QString nomFichier;
    quint16 noSauvegarde, noVersion;
    QDateTime derniereModif;
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

private:
    //Helpers
    void chargeCache();
    void reconstruireCache();
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

    //Cache de fichiers
    QList<CacheEntry> m_cache;
};

}

#endif // SERVEUR_H
