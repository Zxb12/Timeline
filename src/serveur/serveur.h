#ifndef SERVEUR_H
#define SERVEUR_H

#include <QObject>
#include <QTcpServer>

class Paquet;

namespace Serveur
{

class Client;

class Serveur : public QObject
{
    Q_OBJECT
public:
    Serveur(QObject *parent = 0);

    quint16 start(quint16);

    //Handlers
    void handleHello(Paquet *, Client *);
    void handleServerSide(Paquet *, Client *);

private:
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
};

}

#endif // SERVEUR_H
