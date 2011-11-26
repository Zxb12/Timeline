#ifndef CACHE_H
#define CACHE_H

#include "src/serveur/defines.h"

#include <QObject>
#include <QDir>
#include <QDateTime>
#include <QMap>
#include <QVector>

namespace Serveur
{

class Cache : public QObject
{
    Q_OBJECT
public:
    Cache(QObject *parent = 0);

    void changeDossier(const QDir&);

    void chargeCache();
    void sauveCache();
    void reconstruireCache();

    CacheEntry nouveauFichier(const QString &);
    void ajoute(const CacheEntry &);
    bool existe(const QString &, quint16 = -1);
    QString nomFichierReel(const QString &, quint16 = -1);
    QVector<CacheEntry> listeFichiers(quint16 = -1);

    //Accesseurs
    quint64 idFichier() { return m_idFichier; }
    quint16 noSauvegarde() { return m_noSauvegarde; }
    void nouvelleSauvegarde();

private:
    void console(const QString &);
    inline void ajouteFichier(const CacheEntry &);

signals:
    void consoleOut(QString);

private:
    QDir m_dossier; //Dossier dans lequel se trouve le fichier du cache

    QMap<quint16, QVector<CacheEntry> > m_historique;
    QVector<CacheEntry> m_fichiersSupprimes;

    quint64 m_idFichier;
    quint16 m_noSauvegarde;

};

}

#endif // CACHE_H
