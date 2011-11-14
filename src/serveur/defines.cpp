#include "src/serveur/defines.h"

namespace Serveur
{

FileDescription &FileDescription::operator=(const FileHeader &other)
{
    nomReel = other.nomReel;
    noSauvegarde = other.noSauvegarde;
    noVersion = other.noVersion;
    estUnDossier = other.estUnDossier;
    supprime = other.supprime;
    derniereModif = other.derniereModif;

    return *this;
}

CacheEntry &CacheEntry::operator=(const FileHeader &other)
{
    nomReel = other.nomReel;
    noSauvegarde = other.noSauvegarde;
    noVersion = other.noVersion;
    estUnDossier = other.estUnDossier;
    supprime = other.supprime;
    derniereModif = other.derniereModif;

    return *this;
}

}
