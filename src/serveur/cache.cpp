#include "cache.h"

#include "src/shared/shared.h"

#include <QDirIterator>

namespace Serveur
{

Cache::Cache(QObject *parent) :
    QObject(parent), m_dossier()
{
}

void Cache::console(const QString &msg)
{
    emit consoleOut("Cache: " + msg);
}

void Cache::changeDossier(const QDir &dossier)
{
    m_dossier = dossier;
}

void Cache::chargeCache()
{
    QFile cache(m_dossier.absolutePath() + "/" + cacheFileName);

    //On vérifie que le cache existe
    if (!cache.exists())
    {
        reconstruireCache();
        return;
    }

    if (!cache.open(QIODevice::ReadOnly))
    {
        console("Impossible d'ouvrir le fichier de cache. TODO: CODE A METTRE A JOUR");
        return;
    }

    //Extraction de la version du cache
    QDataStream stream(&cache);
    quint8 version;
    stream >> version;

    //Vérification de la version du cache
    if (version != filesystemVersion)
    {
        console("Cache obsolète...");
        cache.close();
        reconstruireCache();
        return;
    }

    //Récupération des infos du cache
    stream >> m_idFichier >> m_noSauvegarde;
    m_historique.clear();
    while (!stream.atEnd())
    {
        CacheEntry entry;
        stream >> entry.nomFichier >> entry.nomReel >> entry.noSauvegarde >> entry.noVersion >> entry.derniereModif
               >> entry.estUnDossier >> entry.supprime;
        m_historique.append(entry);
        console("Cache: " + entry.nomReel + "(" + entry.nomFichier + "), sauv: " + nbr(entry.noSauvegarde) + ", vers: " + nbr(entry.noVersion));
    }
    cache.close();
}

void Cache::sauveCache()
{
    //Ouverture du cache
    QFile cache(m_dossier.absolutePath() + "/" + cacheFileName);
    if (!cache.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        console("Impossible de sauvegarder le cache: " + cache.errorString());
        return;
    }

    //Ecriture du cache
    QDataStream stream(&cache);
    stream << filesystemVersion << m_idFichier << m_noSauvegarde;
    foreach (CacheEntry entry, m_historique)
    {
        stream << entry.nomFichier << entry.nomReel << entry.noSauvegarde << entry.noVersion << entry.derniereModif
               << entry.estUnDossier << entry.supprime;
    }
}

void Cache::reconstruireCache()
{
    console("Reconstruction du cache...");

    QString cachePath = m_dossier.absolutePath() + "/" + cacheFileName;
    QString oldCachePath = cachePath + ".old";
    QFile cache(cachePath);
    m_idFichier = 0;
    m_noSauvegarde = 0;

    //Suppression du cache si nécessaire
    if (cache.exists())
    {
        if (QFile::exists(oldCachePath))
            cache.remove(oldCachePath);
        cache.rename(oldCachePath);
        cache.setFileName(cachePath); //On remet le nom du fichier à sa valeur originale (elle a changé lors du renommage)
    }

    //Génération des entrées du cache
    QDirIterator itr(m_dossier.absolutePath(), QStringList("_*.tlf"), QDir::Files);
    while (itr.hasNext())
    {
        itr.next();
        console("Fichier trouvé: " + itr.fileName());

        //Extraction du No de fichier
        QString fileName = itr.fileName();
        fileName.remove(0, 1);
        fileName.remove(".tlf");

        m_idFichier = qMax(m_idFichier, fileName.toULongLong(0, 36));

        //Création de l'entrée de cache.
        QFile file(itr.filePath());
        if (!file.open(QIODevice::ReadOnly))
        {
            console("Impossible d'ouvrir le fichier: " + file.errorString());
            continue;
        }

        //Vérification de la version du fichier
        QDataStream fileStream(&file);
        quint8 octetEnTete, version;
        fileStream >> octetEnTete;
        version = octetEnTete >> 2; //On décale les bits à droite pour effacer les bits dossier et supprimé.
        if (version != filesystemVersion)
        {
            console("Fichier d'une ancienne version(" + nbr(version) + ")");
            continue;
        }

        //Ajout dans le cache
        CacheEntry entry;
        entry.nomFichier = itr.fileName();
        entry.estUnDossier = octetEnTete & (1 << 1);
        entry.supprime = octetEnTete & 1;
        fileStream >> entry.nomReel >> entry.noSauvegarde >> entry.noVersion >> entry.derniereModif;
        m_historique.append(entry);

        //Vérification du No de sauvegarde
        m_noSauvegarde = qMax(m_noSauvegarde, entry.noSauvegarde);
    }
    cache.close();

    //On incrémente l'ID de fichier pour qu'il corresponde au prochain fichier
    m_idFichier++;

    console("Reconstruction terminée");
    sauveCache();
}

CacheEntry Cache::nouveauFichier(const QString &nomFichier)
{
    //Génération du fichier
    CacheEntry entry;
    QString nbr = QString::number(m_idFichier, 36);
    entry.nomFichier = m_dossier.absolutePath() + "/_" + QString("0").repeated(13 - nbr.size()) + nbr + ".tlf";
    entry.nomReel = nomFichier;
    entry.noSauvegarde = m_noSauvegarde;

    //Recheche du No de version
    QList<CacheEntry> liste = listeFichiers();
    int index = liste.indexOf(entry);
    if (index == -1)
        entry.noVersion = 0;
    else
        entry.noVersion = liste.at(index).noVersion + 1;

    return entry;
}

void Cache::ajoute(const CacheEntry &entry)
{
    m_historique.append(entry);
    m_idFichier++;
}

bool Cache::existe(const QString &nomFichier, quint16 noSauvegarde)
{
    QList<CacheEntry> liste = listeFichiers(noSauvegarde);

    foreach(CacheEntry entry, liste)
    {
        if (entry.nomFichier == nomFichier)
            return true;
    }
    return false;
}

QString Cache::nomFichierReel(const QString &nomFichier, quint16 noSauvegarde)
{
    QList<CacheEntry> liste = listeFichiers(noSauvegarde);

    foreach(CacheEntry entry, liste)
    {
        if (entry.nomFichier == nomFichier)
            return entry.nomReel;
    }

    return QString();
}

QList<CacheEntry> Cache::listeFichiers(quint16 noSauvegarde)
{
    QList<CacheEntry> liste;
    foreach(CacheEntry entry, m_historique)
    {
        if (entry.noSauvegarde > noSauvegarde)
            break;

        //On recherche si l'entrée est déjà dans la liste
        int index = liste.indexOf(entry);

        if (index == -1)
        {
            //Le fichier n'est pas dans la liste: on l'y ajoute
            liste.append(entry);
        }
        else
        {
            //Le fichier est trouvé, on va devoir modifier la liste
            if (entry.supprime == false)
            {
                //Nouvelle version du fichier
                liste.replace(index, entry);
            }
            else
            {
                //Fichier supprimé
                liste.removeAt(index);
            }
        }
    }

    return liste;
}

}
