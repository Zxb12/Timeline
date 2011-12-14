#ifndef DEFINES_H
#define DEFINES_H

#include "src/shared/shared.h"

#include <QFile>
#include <QDateTime>

namespace Serveur
{

const QString cacheFileName = "Timeline.cache";
const quint8 filesVersion = 7;
const quint8 cacheVersion = 9;
#ifdef Q_WS_WIN
const QString gZipExeCompression = "gzip.exe -c";
const QString gZipExeDecompression = "gzip.exe -c -d";
#else
const QString gZipExeCompression = "gzip -c";
const QString gZipExeDecompression = "gzip -cd";
#endif

struct FileDescription : FileHeader
{
    QString nomServeur;

    FileDescription &operator=(const FileHeader &);
};

}

#endif // DEFINES_H
