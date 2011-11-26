#include "src/serveur/defines.h"

#include <QDataStream>

namespace Serveur
{

FileDescription &FileDescription::operator=(const FileHeader &other)
{
    nomClient = other.nomClient;
    noSauvegarde = other.noSauvegarde;
    noVersion = other.noVersion;
    estUnDossier = other.estUnDossier;
    supprime = other.supprime;
    derniereModif = other.derniereModif;

    return *this;
}

CacheEntry &CacheEntry::operator=(const FileHeader &other)
{
    nomClient = other.nomClient;
    noSauvegarde = other.noSauvegarde;
    noVersion = other.noVersion;
    estUnDossier = other.estUnDossier;
    supprime = other.supprime;
    derniereModif = other.derniereModif;

    return *this;
}

QDataStream &operator>>(QDataStream &stream, CacheEntry &entry)
{
    stream >> entry.nomServeur >> entry.nomClient >> entry.noSauvegarde >> entry.noVersion >> entry.derniereModif
           >> entry.estUnDossier >> entry.supprime;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const CacheEntry &entry)
{
    stream << entry.nomServeur << entry.nomClient << entry.noSauvegarde << entry.noVersion << entry.derniereModif
           << entry.estUnDossier << entry.supprime;
    return stream;
}

}
