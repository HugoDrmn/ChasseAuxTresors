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
    QPoint nouvoPoint = DonnerPositionUnique();
    listePositions.append(nouvoPoint);
    double distance=CalculerDistance(nouvoPoint);
    //envoie de la trame
    quint16  taille = 0;
    QBuffer tampon;
    tampon.open(QIODevice::WriteOnly);
    QDataStream out(&tampon);
    out<<taille<<nouvoPoint<<"start"<<distance;
    taille=tampon.size()-sizeof (taille);
    tampon.seek(0);
    out<<taille;
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
        in>>taille;
        if(client->bytesAvailable()>=(quint64)taille)
        {
            in>>caractere;
            qDebug()<<"message de "<<client->peerAddress().toString()<<":"<<caractere;
            EnvoyerDonnees(client,caractere,index);
        }
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
    case 'D':
        colonne++;
        qDebug()<<"va vers le haut";
        break;

    case 'U':
        colonne--;
        qDebug()<<"va vers le bas";
        break;

    case 'R':
        ligne++;
        qDebug()<<"va vers la gauche";
        break;

    case 'L':
        ligne--;
        qDebug()<<"va vers la droite";
        break;
    }
    nouvoPoint=QPoint(ligne,colonne);
    double distance=CalculerDistance(nouvoPoint);
    qDebug()<<"ligne:"<<ligne<<"colonne"<<colonne;
    if(nouvoPoint.y()==tresor.y()&&nouvoPoint.x()==tresor.x())
    {
        nouvoPoint=QPoint(-1,-1);
        reponse="Victoire de"+client->peerAddress().toString();
        distance=0;
            quint16  taille = 0;
            QBuffer tampon;
            tampon.open(QIODevice::WriteOnly);
            QDataStream out(&tampon);
            out<<taille<<nouvoPoint<<reponse<<distance;
            taille=tampon.size()-sizeof (taille);
            tampon.seek(0);
            out<<taille;
            for (int i=0;i<=20;i++) {
                listeSocketClient.at(i)->write(tampon.buffer());
            }
    }else {


    listePositions.replace(index,nouvoPoint);
    QPoint collision1 = listePositions.at(0);
    QPoint collision2 = listePositions.at(1);
    if(collision1.x() == collision2.x() && collision1.y() == collision2.y())
    {
        reponse="collision";
    }

    quint16  taille = 0;
    QBuffer tampon;
    tampon.open(QIODevice::WriteOnly);
    QDataStream out(&tampon);
    out<<taille<<nouvoPoint<<reponse<<distance;
    taille=tampon.size()-sizeof (taille);
    tampon.seek(0);
    out<<taille;
    client->write(tampon.buffer());
    if(collision1.x() == collision2.x() && collision1.y() == collision2.y())
    {
        qtPause(1000);
    }
    }


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
