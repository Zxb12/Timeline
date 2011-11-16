#include "src/serveur/client_s.h"

#include "src/shared/paquet.h"

namespace Serveur
{

Client::Client(QTcpSocket *socket, QObject *parent) : QObject(parent), m_socket(socket), m_taillePaquet(0), m_sessionState(NOT_CHECKED)
{
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(donneesRecues()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(deconnexion()));
    connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(donneesEcrites()));
}

Client::~Client()
{
}

void Client::donneesRecues()
{
    QDataStream stream(m_socket);

    //R�cup�ration de la taille du paquet
    if (m_taillePaquet == 0)
    {
        if (m_socket->bytesAvailable() < sizeof m_taillePaquet)
            return;

        stream >> m_taillePaquet;
    }

    //R�cup�ration du reste du paquet
    if (m_socket->bytesAvailable() < m_taillePaquet)
        return;

    //On lit la socket pour la taille d'un paquet et on stocke.
    Paquet *in = new Paquet(m_socket->read(m_taillePaquet));

    //Remise � z�ro de la taille du paquet
    m_taillePaquet = 0;

    //On envoie le paquet re�u
    emit paquetRecu(in);

    //S'il nous reste quelque chose dans la socket, on relance la fonction.
    if (m_socket->bytesAvailable())
        donneesRecues();
}

void Client::donneesEcrites()
{
    if (m_socket->bytesToWrite() == 0)
        emit paquetEcrit();
}

void Client::deconnexion()
{
    emit deconnecte();
}

Paquet& operator >>(Paquet &paquet, Client *client)
{
    paquet >> client->socket();
    return paquet;
}

}
