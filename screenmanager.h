#ifndef SCREENMANAGER_H
#define SCREENMANAGER_H

#include <QApplication>
#include <QDateTime>
#include <QTimer>
#include <QDesktopWidget>
#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QRect>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QMessageBox>
#include <QBuffer>
#include <QUrl>
#include <QDesktopServices>
#include <QClipboard>
#include <QObject>
#include <QList>
#include <QSettings>
#include "screenshot.h"
#include "logwriter.h"

class ScreenManager : public QObject
{
    Q_OBJECT

    const static quint16 DEFAULT_PROXY_PORT = 80;

    QString server;

    QTcpSocket socket;
    QNetworkProxy proxy;

    LogWriter logger;

    quint32 screensTotal;
    Screenshot** widgets;
    QBuffer buffer;
    QByteArray headers;

    void setupProxy();
    void postImage();
    void checkReply();

public:
    explicit ScreenManager(QVector <QRect> geometrys, QObject *parent = 0);
//    void captureAllScreens();
protected:
    void keyPressEvent(QKeyEvent *e);
signals:
    void signal_printToLog(QString line);
public slots:
    void slot_onScreenshotReady();
    void slot_onConnect();
    void slot_onReadyRead();
    void slot_onSocketError(QAbstractSocket::SocketError error);
};

#endif // SCREENMANAGER_H
