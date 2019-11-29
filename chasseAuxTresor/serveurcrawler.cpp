#include "serveurcrawler.h"
#include "ui_serveurcrawler.h"
#include "QDebug"
#include "QToolButton"


ServeurCrawler::ServeurCrawler(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServeurCrawler)
{
    ui->setupUi(this);
    this->setLayout(&grille);
    grille.setMargin(100);

    for (int i=1;i<=20;i++) {
        for (int j=1;j<=20;j++) {
            QToolButton *b=new QToolButton();
            grille.addWidget(b,i,j,1,1);
        }
    }

    socketEcouteServeur = new QTcpServer(this);
    socketEcouteServeur->setMaxPendingConnections(2);
    tresor=DonnerPositionUnique();
}



void ServeurCrawler::onQTcpServer_newConnection()
{
    QTcpSocket *client;
    client = socketEcouteServeur->nextPendingConnection();
    connect(client,&QTcpSocket::readyRead,this,&ServeurCrawler::onQTcpSocket_readyRead);
    QHostAddress addresseClient = client->peerAddress();
    qDebug()<<addresseClient.toString();
    listeSocketClient.append(client);
    QProcess *p = new QProcess(this);
    connect(p,&QProcess::readyReadStandardOutput,this,&ServeurCrawler::onQProcess_readyReadStandardOutput);
    listeProcess.append(p);
    //définition de la pos aléatoire et de sa distance
    QPoint e = DonnerPositionUnique();
    listePositions.append(e);
    quint16  taille = 0;
    QBuffer tampon;
    double distance=CalculerDistance(e);
    //envoie de la trame
    tampon.open(QIODevice::WriteOnly);
    QDataStream out(&tampon);
    taille=tampon.size()-sizeof (taille);
    out<<taille<<e<<"start"<<distance;
    tampon.seek(0);
    client->write(tampon.buffer());
}

void ServeurCrawler::onQTcpSocket_readyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    int index=listeSocketClient.indexOf(client);
    quint16 taille = 0;
    QChar caractere;
    if(client->bytesAvailable() >= (quint64)sizeof (taille))
    {
        QDataStream in(client);

        in>>taille>>caractere;
        qDebug()<<"message de "<<client->peerAddress().toString()<<":"<<caractere;
        EnvoyerDonnees(client,caractere,index);
    }
}
void ServeurCrawler::EnvoyerDonnees(QTcpSocket *client,QChar caractere,int index)
{
    QString reponse="vide";
    QPoint nouvoPoint=listePositions.at(index);
    int colonne=nouvoPoint.y();
    int ligne=nouvoPoint.x();
    switch(caractere.toLatin1())
    {
    case 'U':
        colonne++;
        break;

    case 'D':
        colonne--;
        break;

    case 'L':
        ligne++;
        break;

    case 'R':
        ligne--;
    }
    nouvoPoint=QPoint(colonne,ligne);
    if(nouvoPoint.y()==tresor.y()&&nouvoPoint.x()==tresor.x())
        nouvoPoint=QPoint(-1,-1);
    double distance=CalculerDistance(nouvoPoint);
    quint16  taille = 0;
    QBuffer tampon;
    tampon.open(QIODevice::WriteOnly);
    QDataStream out(&tampon);
    taille=tampon.size()-sizeof (taille);
    out<<taille<<nouvoPoint<<reponse<<distance;
    tampon.seek(0);
    client->write(tampon.buffer());
}





























void ServeurCrawler::onQTcpSocket_disconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    qDebug()<<"Deconnexion de "+client->peerAddress().toString();
    int index=listeSocketClient.indexOf(client);
    listeSocketClient.removeAt(index);
    listeProcess.removeAt(index);
}

void ServeurCrawler::onQProcess_readyReadStandardOutput()
{
    QProcess *p= qobject_cast<QProcess*>(sender());
    int indexClient=listeProcess.indexOf(p);
    QString reponse = p->readAllStandardOutput();
    if(!reponse.isEmpty())
    {
        QString message = "Réponse envoyée  à "+listeSocketClient.at(indexClient)->peerAddress().toString()+" : " + reponse;
        qDebug()<<message;
        listeSocketClient.at(indexClient)->write(reponse.toUtf8());
    }
}
ServeurCrawler::~ServeurCrawler()
{
    delete ui;
    delete socketEcouteServeur;
}

void ServeurCrawler::qtPause(int millisecondes)
{
    QTimer timer;
    timer.start(millisecondes);
    QEventLoop loop;
    QObject::connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);
    loop.exec();
}

QPoint ServeurCrawler::DonnerPositionUnique()
{
    QRandomGenerator gen;
    QPoint pt;
    gen.seed(QDateTime::currentMSecsSinceEpoch());
    int ligne;
    int colonne;
    do
    {
        ligne=gen.bounded(TAILLE);
        qtPause(20);
        colonne=gen.bounded(TAILLE);
        qtPause(20);
        pt=QPoint(colonne,ligne);
    }while (listePositions.contains(pt));
    return pt;
}

double ServeurCrawler::CalculerDistance(QPoint pos)
{
    double distance;
    int xVecteur=tresor.x()-pos.x();
    int yVecteur=tresor.y()-pos.y();
    distance=sqrt((xVecteur*xVecteur+yVecteur*yVecteur));
    return distance;
}


void ServeurCrawler::on_pushButton_lancement_clicked()
{
    quint16 port = static_cast<quint16>(ui->spinBox_port->value());
    if(!socketEcouteServeur->listen(QHostAddress::Any,port))
    {
        QString message = "Impossible de démarrer le serveur " + socketEcouteServeur->errorString();
        qDebug()<<message;;
        close();
    }
    else
    {
        QList<QHostAddress> listeAdresse = QNetworkInterface::allAddresses();

        for (int i = 0; i < listeAdresse.size();i++)
        {
            if(listeAdresse.at(i).toIPv4Address())
                qDebug()<<listeAdresse.at(i).toString();
        }

        qDebug()<<"N° du port : " + QString::number(socketEcouteServeur->serverPort());
        connect(socketEcouteServeur,&QTcpServer::newConnection,this,&ServeurCrawler::onQTcpServer_newConnection);

        ui->pushButton_lancement->setEnabled(false);
        ui->spinBox_port->setEnabled(false);
    }
}
