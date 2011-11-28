#include "fenprincipale.h"
#include "ui_fenprincipale.h"

#include "src/client/client_c.h"
#include "src/serveur/serveur.h"

#include <QTime>
#include <QDir>
#include <QInputDialog>
#include <QDirIterator>
#include <QThread>

FenPrincipale::FenPrincipale(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::FenPrincipale), m_dossiersDeTransfert(), m_enAttenteDeTransfert(false), m_enAttenteDeRecuperation(false)
{
    ui->setupUi(this);
    m_dossiersDeTransfert << "E:/dev";

    //Création des objets serveur et client
    //TODO: les allouer seulement quand c'est nécessaire (au lancement de la sauvegarde)
    m_client = new Client::Client;
    m_serveur = new Serveur::Serveur;
    m_clientThread = new QThread;
    m_serveurThread = new QThread;

    //Lancement des threads
    m_client->moveToThread(m_clientThread);
    m_serveur->moveToThread(m_serveurThread);
    m_clientThread->start(QThread::LowPriority);
    m_serveurThread->start(QThread::LowPriority);

    //Connexions
    connect(m_client, SIGNAL(listeRecue(QList<FileHeader>)), this, SLOT(listeRecue(QList<FileHeader>)));
    connect(m_client, SIGNAL(consoleOut(QString)), this, SLOT(consoleClient(QString)));
    connect(m_serveur, SIGNAL(consoleOut(QString)), this, SLOT(consoleServeur(QString)));

    //Appels des fonctions
    qRegisterMetaType<QDir>("QDir");
    qRegisterMetaType<QFileInfo>("QFileInfo");
    qRegisterMetaType<QList<FileHeader> >("QList<FileHeader>");
    qRegisterMetaType<FileHeader>("FileHeader");
    QMetaObject::invokeMethod(m_serveur, "start", Qt::QueuedConnection, Q_ARG(QDir,QDir("E:/Timeline")), Q_ARG(quint16,TIMELINE_PORT));
    QMetaObject::invokeMethod(m_client, "connecte", Qt::QueuedConnection, Q_ARG(QString,"localhost"), Q_ARG(quint16,TIMELINE_PORT));
}

FenPrincipale::~FenPrincipale()
{
    m_clientThread->quit();
    m_clientThread->wait();
    m_serveurThread->quit();
    m_serveurThread->wait();
    delete m_clientThread;
    delete m_serveurThread;
    delete m_client;
    delete m_serveur;
    delete ui;
}

void FenPrincipale::consoleClient(QString msg)
{
    msg = QTime::currentTime().toString() + ": " + msg;
    ui->consoleClient->appendPlainText(msg);
}

void FenPrincipale::consoleServeur(QString msg)
{
    msg = QTime::currentTime().toString() + ": " + msg;
    ui->consoleServeur->appendPlainText(msg);
}

void FenPrincipale::listeRecue(QList<FileHeader> listeDistante)
{
    QList<FileHeader> copieListe = listeDistante;
    consoleClient("Liste reçue");
    qApp->processEvents();

    //Envoi des fichiers
    if (m_enAttenteDeTransfert)
    {
        //Génération de la liste locale
        QList<QString> listeLocale;
        int n = 0;
        foreach(QString dossier, m_dossiersDeTransfert)
        {
            QDirIterator itr(dossier, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while(itr.hasNext())
            {
                n++;
                if (n % 1000 == 0)
                {
                    consoleClient("Fichiers trouvés: " + nbr(n));
                    qApp->processEvents();
                }
                listeLocale.append(itr.next());
            }
        }
        qSort(listeDistante.begin(), listeDistante.end());
        qSort(listeLocale.begin(), listeLocale.end());
        consoleClient("Fin de la génération de la liste locale");
        qApp->processEvents();
        n = 0;
        bool nouvelleSauvegardeLancee = false;

        while (!listeDistante.isEmpty() && !listeLocale.isEmpty())
        {
            n++;
            if (n % 1000 == 0)
            {
                consoleClient("Fichiers analysés: " + nbr(n));
                qApp->processEvents();
            }

            const FileHeader &header = listeDistante.first();
            const QString &nomLocal = listeLocale.first();

            if (header.nomClient == nomLocal)
            {
                //Fichier existe, on vérifie la date
                QFileInfo fileInfo(nomLocal);
                if (fileInfo.lastModified() > header.derniereModif)
                {
                    //On ajoute le fichier
                    if (!nouvelleSauvegardeLancee)
                    {
                        nouvelleSauvegardeLancee = true;
                        QMetaObject::invokeMethod(m_client, "nouvelleSauvegarde", Qt::QueuedConnection);
                    }
                    QMetaObject::invokeMethod(m_client, "envoie", Qt::QueuedConnection, Q_ARG(QFileInfo,fileInfo));
                }

                listeDistante.removeFirst();
                listeLocale.removeFirst();
            }
            else if (header.nomClient < nomLocal)
            {
                //Fichier disant présent, fichier local absent
                //Il faut supprimer le fichier distant
                if (!nouvelleSauvegardeLancee)
                {
                    nouvelleSauvegardeLancee = true;
                    QMetaObject::invokeMethod(m_client, "nouvelleSauvegarde", Qt::QueuedConnection);
                }
                QMetaObject::invokeMethod(m_client, "supprime", Qt::QueuedConnection, Q_ARG(QString,header.nomClient));
                listeDistante.removeFirst();
            }
            else
            {
                //Fichier local présent, fichier distant absent
                //Il faut envoyer le fichier
                if (!nouvelleSauvegardeLancee)
                {
                    nouvelleSauvegardeLancee = true;
                    QMetaObject::invokeMethod(m_client, "nouvelleSauvegarde", Qt::QueuedConnection);
                }
                QFileInfo fileInfo(nomLocal);
                QMetaObject::invokeMethod(m_client, "envoie", Qt::QueuedConnection, Q_ARG(QFileInfo,fileInfo));
                listeLocale.removeFirst();
            }
        }

        //Envoi/suppression des fichiers restants
        foreach(FileHeader header, listeDistante)
        {
            if (!nouvelleSauvegardeLancee)
            {
                nouvelleSauvegardeLancee = true;
                QMetaObject::invokeMethod(m_client, "nouvelleSauvegarde", Qt::QueuedConnection);
            }
            QMetaObject::invokeMethod(m_client, "supprime", Qt::QueuedConnection, Q_ARG(QString,header.nomClient));
        }

        foreach(QString nomFichier, listeLocale)
        {
            QFileInfo fileInfo(nomFichier);
            if (!nouvelleSauvegardeLancee)
            {
                nouvelleSauvegardeLancee = true;
                QMetaObject::invokeMethod(m_client, "nouvelleSauvegarde", Qt::QueuedConnection);
            }
            QMetaObject::invokeMethod(m_client, "envoie", Qt::QueuedConnection, Q_ARG(QFileInfo,fileInfo));
        }
    }

    //Récupération des fichiers
    if (m_enAttenteDeRecuperation)
    {
        listeDistante = copieListe;
        QMetaObject::invokeMethod(m_client, "setDossierRecuperation", Qt::QueuedConnection, Q_ARG(QDir,QDir("E:/Récupération")));


        foreach(FileHeader header, listeDistante)
        {
            QMetaObject::invokeMethod(m_client, "recupereFichier", Qt::QueuedConnection, Q_ARG(FileHeader,header));
        }

        m_enAttenteDeRecuperation = false;
    }

}

void FenPrincipale::on_btn1_clicked()
{
    m_enAttenteDeTransfert = true;
    QMetaObject::invokeMethod(m_client, "listeFichiers", Qt::QueuedConnection);
}

void FenPrincipale::on_btn2_clicked()
{
    quint16 noSauvegarde = QInputDialog::getInt(this, "Timeline", "No sauvegarde à récupérer");
    if (noSauvegarde > 0)
    {
        m_enAttenteDeRecuperation = true;
        QMetaObject::invokeMethod(m_client, "listeFichiers", Qt::QueuedConnection, Q_ARG(quint16,noSauvegarde));
    }
}

void FenPrincipale::on_btn3_clicked()
{
}
