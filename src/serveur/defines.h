#ifndef DEFINES_H
#define DEFINES_H

#include <QFile>
#include <QDateTime>

namespace Serveur
{

const QString cacheFileName = "Timeline.cache";
const quint8 filesystemVersion = 6;

struct FileHeader
{
    QString nomClient;
    quint16 noSauvegarde, noVersion;
    QDateTime derniereModif;
    bool estUnDossier, supprime;

    bool operator==(const FileHeader &other) { return nomClient == other.nomClient; }
};

struct FileDescription : FileHeader
{
    QFile *fichier;

    FileDescription &operator=(const FileHeader &);
};

struct CacheEntry : FileHeader
{
    QString nomServeur;

    CacheEntry &operator=(const FileHeader &);
};

}

#endif // DEFINES_H
