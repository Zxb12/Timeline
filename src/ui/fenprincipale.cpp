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
    connect(m_client, SIGNAL(listeRecue(QVector<FileHeader>)), this, SLOT(listeRecue(QVector<FileHeader>)));
    connect(m_client, SIGNAL(consoleOut(QString)), this, SLOT(consoleClient(QString)));
    connect(m_serveur, SIGNAL(consoleOut(QString)), this, SLOT(consoleServeur(QString)));

    //Appels des fonctions
    qRegisterMetaType<QDir>("QDir");
    qRegisterMetaType<QFileInfo>("QFileInfo");
    qRegisterMetaType<QVector<FileHeader> >("QVector<FileHeader>");
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

void FenPrincipale::listeRecue(QVector<FileHeader> liste)
{
    QVector<FileHeader> copieListe = liste;
    //Envoi des fichiers
    if (m_enAttenteDeTransfert)
    {
        bool nouvelleSauvegardeLancee = false;
        int n = 0;
        //Création de la liste de fichiers
        foreach(QString dossier, m_dossiersDeTransfert)
        {
            QDirIterator itr(dossier, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while(itr.hasNext())
            {
                n++;
                if (n % 100 == 0)
                {
                    consoleClient(nbr(n) + " fichiers analysés");
                    QApplication::processEvents();
                }
                int index = liste.indexOf(itr.next());
                QFileInfo fileInfo = itr.fileInfo();
                if (index == -1)
                {
                    //On ajoute le fichier
                    if (!nouvelleSauvegardeLancee)
                    {
                        nouvelleSauvegardeLancee = true;
                        QMetaObject::invokeMethod(m_client, "nouvelleSauvegarde", Qt::QueuedConnection);
                    }

                    QMetaObject::invokeMethod(m_client, "envoie", Qt::QueuedConnection, Q_ARG(QFileInfo,fileInfo));
                }
                else
                {
                    //On vérifie la date du fichier et on met à jour si nécessaire
                    if (fileInfo.lastModified() > liste.at(index).derniereModif)
                    {
                        if (!nouvelleSauvegardeLancee)
                        {
                            nouvelleSauvegardeLancee = true;
                            QMetaObject::invokeMethod(m_client, "nouvelleSauvegarde", Qt::QueuedConnection);
                        }

                        QMetaObject::invokeMethod(m_client, "envoie", Qt::QueuedConnection, Q_ARG(QFileInfo,fileInfo));
                    }

                    //On supprime le fichier de la liste
                    liste.remove(index);
                }
            }
        }

        //Suppression des fichiers restants
        foreach(FileHeader header, liste)
        {
            if (!nouvelleSauvegardeLancee)
            {
                nouvelleSauvegardeLancee = true;
                QMetaObject::invokeMethod(m_client, "nouvelleSauvegarde", Qt::QueuedConnection);
            }

            QMetaObject::invokeMethod(m_client, "supprime", Qt::QueuedConnection, Q_ARG(QString,header.nomClient));
        }

        if (!nouvelleSauvegardeLancee)
            consoleClient("Aucun transfert n'est nécessaire !");

        m_enAttenteDeTransfert = false;
    }

    //Récupération des fichiers
    if (m_enAttenteDeRecuperation)
    {
        liste = copieListe;
        QMetaObject::invokeMethod(m_client, "setDossierRecuperation", Qt::QueuedConnection, Q_ARG(QDir,QDir("E:/Récupération")));


        foreach(FileHeader header, liste)
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
