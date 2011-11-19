#ifndef DEFINES_H
#define DEFINES_H

#include "src/shared/shared.h"

#include <QFile>
#include <QDateTime>

namespace Serveur
{

const QString cacheFileName = "Timeline.cache";
const quint8 filesystemVersion = 6;

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
