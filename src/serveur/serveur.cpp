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

quint16 Serveur::start(quint16 port = 0)
{
    if (!m_serveur->listen(QHostAddress::Any, port))
    {
        console("Le serveur n'a pas pu d�marrer (" + m_serveur->errorString() + ")");
        return 0;
    }

    console("Serveur d�marr� (h�te: " + m_serveur->serverAddress().toString() + ", port:" + nbr(m_serveur->serverPort()) + ")");
    return m_serveur->serverPort();
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

    OpCodeHandler handler = OpCodeTable[opCode];
    console("Paquet re�u: " + handler.nom + "("+nbr(opCode)+")");
    (this->*handler.f)(in, client);

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

void Serveur::handleServerSide(Paquet *in, Client *client)
{

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

    //Renvoi d'un paquet Hello
    Paquet out;
    out << SMSG_HELLO >> client;
}


}
