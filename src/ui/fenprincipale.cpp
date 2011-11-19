#include "fenprincipale.h"
#include "ui_fenprincipale.h"

#include <QTime>
#include <QDir>
#include <QFileDialog>
#include <QDirIterator>

FenPrincipale::FenPrincipale(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::FenPrincipale), m_dossiersDeTransfert("E:/Timeline"), m_enAttenteDeTransfert(false)
{
    ui->setupUi(this);

    //Création des objets serveur et client
    //TODO: les allouer seulement quand c'est nécessaire (au lancement de la sauvegarde)
    m_client = new Client::Client(this);
    m_serveur = new Serveur::Serveur(this);

    connect(m_client, SIGNAL(consoleOut(QString)), this, SLOT(consoleClient(QString)));
    connect(m_client, SIGNAL(listeRecue(QList<FileHeader>)), this, SLOT(listeRecue(QList<FileHeader>)));
    connect(m_serveur, SIGNAL(consoleOut(QString)), this, SLOT(consoleServeur(QString)));

    m_serveur->start(QDir("Z:/Timeline"), TIMELINE_PORT);
    m_client->connecte("localhost", TIMELINE_PORT);
}

FenPrincipale::~FenPrincipale()
{
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

    m_client->nouvelleSauvegarde();
    qDebug() << "Début: " << liste.size();

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
                m_client->envoie(fileInfo);
            }
            else
            {
                //On vérifie la date du fichier et on met à jour si nécessaire
                if (fileInfo.lastModified() > liste.at(index).derniereModif)
                {
                    m_client->envoie(fileInfo);
                }

                //On supprime le fichier de la liste
                liste.removeAt(index);
            }
        }
    }

    qDebug() << "A supprimer: " << liste.size();

    //Suppression des fichiers restants
    foreach(FileHeader header, liste)
    {
        m_client->supprime(header.nomClient);
    }

    m_enAttenteDeTransfert = false;
}

void FenPrincipale::on_btn1_clicked()
{
    m_enAttenteDeTransfert = true;
    m_client->listeFichiers();
}

void FenPrincipale::on_btn2_clicked()
{
//    QString fichierLocal = QFileDialog::getSaveFileName(this);
//    if (fichierLocal.isEmpty())
//        return;
//    m_client->recupereFichier("E:/Musique.mp3", fichierLocal);
}

void FenPrincipale::on_btn3_clicked()
{
    m_client->listeFichiers();
}
