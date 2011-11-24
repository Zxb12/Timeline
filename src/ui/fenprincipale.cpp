#include "fenprincipale.h"
#include "ui_fenprincipale.h"

#include "src/client/client_c.h"
#include "src/serveur/serveur.h"

#include <QTime>
#include <QDir>
#include <QFileDialog>
#include <QDirIterator>
#include <QThread>

FenPrincipale::FenPrincipale(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::FenPrincipale), m_dossiersDeTransfert(), m_enAttenteDeTransfert(false)
{
    ui->setupUi(this);
    m_dossiersDeTransfert << "E:/dev/PDCurses-3.4";

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
    qRegisterMetaType<QList<FileHeader> >("QList<FileHeader>");
    qRegisterMetaType<QFileInfo>("QFileInfo");
    QMetaObject::invokeMethod(m_serveur, "start", Qt::QueuedConnection, Q_ARG(QDir,QDir("Z:/Timeline")), Q_ARG(quint16,TIMELINE_PORT));
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

void FenPrincipale::listeRecue(QList<FileHeader> liste)
{
    if (!m_enAttenteDeTransfert)
        return;

    QMetaObject::invokeMethod(m_client, "nouvelleSauvegarde", Qt::QueuedConnection);

    //Création de la liste de fichiers
    foreach(QString dossier, m_dossiersDeTransfert)
    {
        QDirIterator itr(dossier, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while(itr.hasNext())
        {
            int index = liste.indexOf(itr.next());
            QFileInfo fileInfo = itr.fileInfo();
            if (index == -1)
            {
                //On ajoute le fichier
                QMetaObject::invokeMethod(m_client, "envoie", Qt::QueuedConnection, Q_ARG(QFileInfo,fileInfo));
            }
            else
            {
                //On vérifie la date du fichier et on met à jour si nécessaire
                if (fileInfo.lastModified() > liste.at(index).derniereModif)
                {
                    QMetaObject::invokeMethod(m_client, "envoie", Qt::QueuedConnection, Q_ARG(QFileInfo,fileInfo));
                }

                //On supprime le fichier de la liste
                liste.removeAt(index);
            }
        }
    }

    //Suppression des fichiers restants
    foreach(FileHeader header, liste)
    {
        QMetaObject::invokeMethod(m_client, "supprime", Qt::QueuedConnection, Q_ARG(QString,header.nomClient));
    }

    m_enAttenteDeTransfert = false;
}

void FenPrincipale::on_btn1_clicked()
{
    m_enAttenteDeTransfert = true;
    QMetaObject::invokeMethod(m_client, "listeFichiers", Qt::QueuedConnection);
}

void FenPrincipale::on_btn2_clicked()
{
}

void FenPrincipale::on_btn3_clicked()
{
}
