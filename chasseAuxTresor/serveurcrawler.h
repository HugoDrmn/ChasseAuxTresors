#ifndef SERVEURCRAWLER_H
#define SERVEURCRAWLER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QProcess>
#include <QHostInfo>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QPoint>
#include "QBuffer"
#include "QTimer"
#include "QRandomGenerator"
#include "QDateTime"
#include "QGridLayout"

#define TAILLE 20
namespace Ui {
class ServeurCrawler;
}

class ServeurCrawler : public QWidget
{
    Q_OBJECT

public:
    explicit ServeurCrawler(QWidget *parent = nullptr);
    ~ServeurCrawler();
    void qtPause(int millisecondes);
    QPoint DonnerPositionUnique();
    double CalculerDistance(QPoint pos);
    void EnvoyerDonnees(QTcpSocket *client,QChar caractere,int index);
private slots:
    void on_pushButton_lancement_clicked();
    void onQTcpServer_newConnection();
    void onQTcpSocket_readyRead();
    void onQTcpSocket_disconnected();
    void onQProcess_readyReadStandardOutput();
private:
    Ui::ServeurCrawler *ui;
    QTcpServer *socketEcouteServeur;
    QList <QTcpSocket* >listeSocketClient;
    QList <QProcess* >listeProcess;
    QList<QPoint>listePositions;
    QPoint tresor;
    QGridLayout grille;

};

#endif // SERVEURCRAWLER_H
