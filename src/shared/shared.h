#ifndef SHARED_H
#define SHARED_H

#include <QDateTime>
#include <QDebug>

#define VERSION "0.0.1a"
#define TIMELINE_PORT 50182
#define MAX_PACKET_SIZE 256 * 1024 //Taille maximum des donn�es dans FILE_DATA
#define nbr(x) QString::number(x)

struct FileInfo
{
    quint16 noSauvegarde, noVersion;
    QDateTime derniereModif;
    bool estUnDossier, supprime;

    FileInfo() : noSauvegarde(0), noVersion(0), derniereModif(), estUnDossier(false), supprime(false) {}
};

struct FileHeader : FileInfo
{
    QString nomClient;

    FileHeader() : FileInfo(), nomClient() {}
    FileHeader(const QString &nom) : FileInfo(), nomClient(nom) {}
    bool operator==(const FileHeader &other) const { return nomClient == other.nomClient; }
    bool operator<(const FileHeader &other) const { return nomClient < other.nomClient; }
};

enum Error
{
    SERR_INVALID_VERSION = 0,
    SERR_COULDNT_CREATE_FILE,
    SERR_COULDNT_OPEN_FILE,
    SERR_TRANSFER_IN_PROGESS,
    SERR_FILE_DOESNT_EXIST
};

enum EtatTransfert
{
    AUCUN_TRANSFERT = 0,
    EN_ATTENTE,
    CLIENT_VERS_SERVEUR,
    SERVEUR_VERS_CLIENT
};

enum OpCodeValues
{
    CMSG_HELLO = 0,
    SMSG_HELLO,
    SMSG_ERROR,
    SMSG_KICK,
    CMSG_NEW_BACKUP,
    CMSG_INITIATE_TRANSFER,
    CMSG_FINISH_TRANSFER,
    CMSG_CANCEL_TRANSFER,
    CMSG_FILE_DATA,
    SMSG_WAITING_FOR_DATA,
    SMSG_TRANSFER_COMPLETE,
    CMSG_DELETE_FILE,
    SMSG_FILE_DELETED,
    CMSG_FILE_LIST,
    SMSG_FILE_LIST,
    CMSG_RECOVER_FILE,
    SMSG_FINISH_TRANSFER,
    SMSG_CANCEL_TRANSFER,
    SMSG_FILE_DATA
};

#define NB_OPCODES 19

#endif // SHARED_H
