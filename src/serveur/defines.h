#ifndef DEFINES_H
#define DEFINES_H

#include "src/shared/shared.h"

#include <QFile>
#include <QDateTime>

namespace Serveur
{

const QString cacheFileName = "Timeline.cache";
const quint8 filesVersion = 7;
const quint8 cacheVersion = 7;

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
QDataStream &operator>>(QDataStream &, CacheEntry &);
QDataStream &operator<<(QDataStream &, const CacheEntry &);

}

#endif // DEFINES_H
