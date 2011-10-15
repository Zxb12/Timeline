#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QDir>

#include "src/shared/shared.h"

class Paquet;

namespace Client
{

class Client : public QObject
{
    Q_OBJECT
public:
    Client(QObject *parent = 0);

    //Handlers
    void handleClientSide(Paquet*);
    void handleError(Paquet*);
    void handleKick(Paquet*);
    void handleHello(Paquet*);

signals:
    void consoleOut(QString);

public slots:
    void connecte(const QString&, quint16);

private slots:
    void console(const QString&);

    void connecte();
    void deconnecte();
    void donneesRecues();
    void erreurSocket(QAbstractSocket::SocketError);

private:
    //Socket
    QTcpSocket *m_socket;
    quint16 m_taillePaquet;
};

}

#endif // CLIENT_H
