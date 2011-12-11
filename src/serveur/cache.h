#ifndef CACHE_H
#define CACHE_H

#include "src/serveur/defines.h"

#include <QObject>
#include <QDir>
#include <QDateTime>
#include <QMap>
#include <QList>
#include <QVector>

namespace Serveur
{

class Cache : public QObject
{
    Q_OBJECT

public:
    struct CacheEntry : FileInfo
    {
        QString nomServeur;
        quint32 idNomClient;

        CacheEntry() : FileInfo(), nomServeur(), idNomClient(-1) {}
        bool operator==(const CacheEntry &other) { return idNomClient == other.idNomClient; }
    };

    Cache(QObject *parent = 0);

    void changeDossier(const QDir&);

    void chargeCache();
    void sauveCache();
    void reconstruireCache();

    FileDescription nouveauFichier(const QString &) const;
    void ajoute(const FileDescription &);
    bool existe(const QString &, quint16 = -1) const;
    QString nomFichierReel(const QString &, quint16 = -1) const;
    QList<FileDescription> listeFichiers(quint16 = -1) const;

    void nouvelleSauvegarde();

private:
    void console(const QString &);
    inline void ajouteFichier(const CacheEntry &);
    QList<CacheEntry> listeCacheEntry(quint16 = -1) const;
    FileDescription convertit(const CacheEntry &) const;
    CacheEntry convertit(const FileDescription &) const;

signals:
    void consoleOut(QString);

private:
    QDir m_dossier; //Dossier dans lequel se trouve le fichier du cache

    QMap<quint16, QList<CacheEntry> > m_historique;
    QList<CacheEntry> m_fichiersSupprimes;
    QVector<QString> m_nomsFichiers;

    quint64 m_idFichier;
    quint16 m_noSauvegarde;
};

QDataStream &operator>>(QDataStream &, Cache::CacheEntry &);
QDataStream &operator<<(QDataStream &, const Cache::CacheEntry &);

}

#endif // CACHE_H
