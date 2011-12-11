#include "cache.h"

#include "src/shared/shared.h"

#include <QDirIterator>

namespace Serveur
{

Cache::Cache(QObject *parent) :
    QObject(parent), m_idFichier(0), m_noSauvegarde(0)
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
    if (version != cacheVersion)
    {
        console("Cache obsolète...");
        cache.close();
        reconstruireCache();
        return;
    }

    //Récupération des infos du cache
    m_nomsFichiers.clear();
    m_fichiersSupprimes.clear();
    m_historique.clear();
    stream >> m_idFichier >> m_noSauvegarde >> m_nomsFichiers >> m_fichiersSupprimes;
    while (!stream.atEnd())
    {
        quint16 noSauvegarde;
        QList<CacheEntry> liste;
        stream >> noSauvegarde >> liste;
        m_historique.insert(noSauvegarde, liste);
        console("Sauvegarde trouvée: " + nbr(noSauvegarde) + ", nbFichiers: " + nbr(m_historique[noSauvegarde].size()));
    }
    cache.close();

    console("Cache de " + nbr(m_historique.size()) + " entrées chargé");
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
    stream << cacheVersion << m_idFichier << m_noSauvegarde << m_nomsFichiers << m_fichiersSupprimes;
    QMapIterator<quint16, QList<CacheEntry> > itr(m_historique);
    while(itr.hasNext())
    {
        itr.next();
        stream << itr.key() << itr.value();
    }

    cache.close();
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
    int nbTrouves = 0;
    quint16 noSauvegarde = 0;
    while (itr.hasNext())
    {
        itr.next();
        nbTrouves++;
        if (nbTrouves % 100 == 0)
            console("Reconstruction: Fichiers analysés: " + nbr(nbTrouves));

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
        if (version != filesVersion)
        {
            console("Fichier d'une ancienne version(" + nbr(version) + ")");
            continue;
        }

        //Ajout dans le cache
        CacheEntry entry;
        QString nomClient; //Servira à trouver l'ID du texte
        entry.nomServeur = itr.fileName();
        entry.estUnDossier = octetEnTete & (1 << 1);
        entry.supprime = octetEnTete & 1;
        fileStream >> nomClient >> entry.noSauvegarde >> entry.noVersion >> entry.derniereModif;

        //Recherche ou création de l'ID de
        if (int index = m_nomsFichiers.indexOf(nomClient) != -1)
            entry.idNomClient = index;
        else
        {
            entry.idNomClient = m_nomsFichiers.size();
            m_nomsFichiers.append(nomClient);
        }

        if (entry.noSauvegarde > noSauvegarde)
        {
            //Copie de la liste
            m_historique[entry.noSauvegarde] = m_historique.value(noSauvegarde);
            m_noSauvegarde = noSauvegarde = entry.noSauvegarde;
            console("Reconstruction: Sauvegarde no " + nbr(noSauvegarde));
        }

        ajouteFichier(entry);
    }
    cache.close();

    //On incrémente l'ID de fichier pour qu'il corresponde au prochain fichier
    m_idFichier++;

    console("Reconstruction terminée");
    sauveCache();
}

FileDescription Cache::nouveauFichier(const QString &nomClient) const
{
    //Génération du fichier
    CacheEntry entry;
    QString nbr = QString::number(m_idFichier, 36);
    entry.nomServeur = "_" + QString("0").repeated(13 - nbr.size()) + nbr + ".tlf";
    entry.idNomClient = m_nomsFichiers.indexOf(nomClient);
    entry.noSauvegarde = m_noSauvegarde;

    //Recheche du No de version
    QList<CacheEntry> liste = listeCacheEntry();
    int index = liste.indexOf(entry);
    if (index == -1)
    {
        index = m_fichiersSupprimes.lastIndexOf(entry);
        if (index == -1)
            entry.noVersion = 0;
        else
            entry.noVersion = m_fichiersSupprimes.at(index).noVersion + 1;
    }
    else
        entry.noVersion = liste.at(index).noVersion + 1;

    return convertit(entry);
}

void Cache::ajoute(const FileDescription &description)
{
    //Si nécessaire, on ajour le nom du fichier à la liste
    if (!m_nomsFichiers.contains(description.nomClient))
        m_nomsFichiers.append(description.nomClient);

    ajouteFichier(convertit(description));

    m_idFichier++;
}

bool Cache::existe(const QString &nomClient, quint16 noSauvegarde) const
{
    QList<CacheEntry> liste = listeCacheEntry(noSauvegarde);
    int index = m_nomsFichiers.indexOf(nomClient);
    if (index != -1)
    {
        foreach(CacheEntry entry, liste)
        {
            if (entry.idNomClient == (quint32) index)
                return true;
        }
    }
    return false;
}

QString Cache::nomFichierReel(const QString &nomClient, quint16 noSauvegarde) const
{
    QList<CacheEntry> liste = listeCacheEntry(noSauvegarde);
    int index = m_nomsFichiers.indexOf(nomClient);
    if (index != -1)
    {
        foreach(CacheEntry entry, liste)
        {
            if (entry.idNomClient == (quint32) index)
                return entry.nomServeur;
        }
    }

    return QString();
}

QList<FileDescription> Cache::listeFichiers(quint16 noSauvegarde) const
{
    quint16 max = -1; //Il est impossible de comparer directement à -1
    if (noSauvegarde == max)
        noSauvegarde = m_noSauvegarde;

    //Conversion de toutes les entrées
    QList<FileDescription> liste;
    foreach(CacheEntry entry, m_historique.value(noSauvegarde))
    {
        liste.append(convertit(entry));
    }

    return liste;
}

QList<Cache::CacheEntry> Cache::listeCacheEntry(quint16 noSauvegarde) const
{
    quint16 max = -1; //Il est impossible de comparer directement à -1
    if (noSauvegarde == max)
        noSauvegarde = m_noSauvegarde;

    return m_historique.value(noSauvegarde);
}

void Cache::nouvelleSauvegarde()
{
    m_noSauvegarde++;
    if (m_noSauvegarde > 1)
        m_historique[m_noSauvegarde] = m_historique[m_noSauvegarde - 1];
    console("No nouvelle sauvegarde: " + nbr(m_noSauvegarde));
}

void Cache::ajouteFichier(const CacheEntry &entry)
{
    //On recherche si l'entrée est déjà dans la liste
    int index = m_historique[entry.noSauvegarde].indexOf(entry);

    if (index == -1)
    {
        //Le fichier n'est pas dans la liste: on l'y ajoute s'il n'est pas supprimé
        if (!entry.supprime)
            m_historique[entry.noSauvegarde].append(entry);
    }
    else
    {
        if (entry.noSauvegarde > m_historique[entry.noSauvegarde][index].noSauvegarde)
        {
            //Le fichier est trouvé, on va devoir modifier la liste
            if (!entry.supprime)
            {
                //Nouvelle version du fichier
                m_historique[entry.noSauvegarde].replace(index, entry);
            }
            else
            {
                //Fichier supprimé
                m_historique[entry.noSauvegarde].removeAt(index);
            }
        }
    }

    //On ajoute le fichier à la liste des fichiers supprimés si nécessaire
    if (entry.supprime)
        m_fichiersSupprimes.append(entry);
}

FileDescription Cache::convertit(const CacheEntry &entry) const
{
    FileDescription description;
    description.nomClient = entry.idNomClient < (quint32) m_nomsFichiers.size() ? m_nomsFichiers.at(entry.idNomClient) : QString();
    description.nomServeur = entry.nomServeur;
    description.noSauvegarde = entry.noSauvegarde;
    description.noVersion = entry.noVersion;
    description.derniereModif = entry.derniereModif;
    description.estUnDossier = entry.estUnDossier;
    description.supprime = entry.supprime;

    return description;
}

Cache::CacheEntry Cache::convertit(const FileDescription &description) const
{
    CacheEntry entry;
    entry.idNomClient = m_nomsFichiers.indexOf(description.nomClient);
    entry.nomServeur = description.nomServeur;
    entry.noSauvegarde = description.noSauvegarde;
    entry.noVersion = description.noVersion;
    entry.derniereModif = description.derniereModif;
    entry.estUnDossier = description.estUnDossier;
    entry.supprime = description.supprime;

    return entry;
}


QDataStream &operator>>(QDataStream &stream, Cache::CacheEntry &entry)
{
    stream >> entry.nomServeur >> entry.idNomClient >> entry.noSauvegarde >> entry.noVersion
           >> entry.derniereModif >> entry.estUnDossier >> entry.supprime;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const Cache::CacheEntry &entry)
{
    stream << entry.nomServeur << entry.idNomClient << entry.noSauvegarde << entry.noVersion
           << entry.derniereModif << entry.estUnDossier << entry.supprime;
    return stream;
}

}
