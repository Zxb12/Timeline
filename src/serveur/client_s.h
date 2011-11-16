#ifndef CLIENT_H
#define CLIENT_H

#include <QTcpSocket>

#include "src/serveur/opcode_s.h"

class Paquet;

namespace Serveur
{

class Client : public QObject
{
    Q_OBJECT
public:
    Client(QTcpSocket *socket, QObject *parent = 0);
    ~Client();

    QTcpSocket* socket() { return m_socket; }

    SessionState sessionState() { return m_sessionState; }
    void setSessionState(SessionState state) { m_sessionState = state; }

signals:
    void paquetRecu(Paquet*);
    void paquetEcrit();
    void deconnecte();

public slots:
    void donneesRecues();
    void donneesEcrites();
    void deconnexion();

private:
    QTcpSocket *m_socket;
    quint32 m_taillePaquet;

    SessionState m_sessionState;
};

Paquet& operator >>(Paquet &, Client *);
}

#endif // CLIENT_H
