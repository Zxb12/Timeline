#ifndef DEFINES_H
#define DEFINES_H

#include "src/shared/shared.h"

#include <QFile>
#include <QDateTime>

namespace Serveur
{

const QString cacheFileName = "Timeline.cache";
const quint8 filesVersion = 7;
const quint8 cacheVersion = 8;

struct FileDescription : FileHeader
{
    QString nomServeur;

    FileDescription &operator=(const FileHeader &);
};

}

#endif // DEFINES_H
