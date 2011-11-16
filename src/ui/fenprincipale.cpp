#include "fenprincipale.h"
#include "ui_fenprincipale.h"

#include <QTime>
#include <QDir>
#include <QFileDialog>

FenPrincipale::FenPrincipale(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::FenPrincipale)
{
    ui->setupUi(this);

    //Création des objets serveur et client
    //TODO: les allouer seulement quand c'est nécessaire (au lancement de la sauvegarde)
    m_client = new Client::Client(this);
    m_serveur = new Serveur::Serveur(this);

    connect(m_client, SIGNAL(consoleOut(QString)), this, SLOT(consoleClient(QString)));
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

void FenPrincipale::on_btn1_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (fileName.isEmpty())
        return;
    QFileInfo file(fileName);
    m_client->nouvelleSauvegarde();
    m_client->envoie(file);
}

void FenPrincipale::on_btn2_clicked()
{
    QString fichierLocal = QFileDialog::getSaveFileName(this);
    if (fichierLocal.isEmpty())
        return;
    m_client->recupereFichier("E:/Musique.mp3", fichierLocal);
}

void FenPrincipale::on_btn3_clicked()
{
    m_client->listeFichiers();
}
