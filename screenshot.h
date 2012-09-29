#ifndef SCREENSHOT_H
#define SCREENSHOT_H

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

#include "logwriter.h"

class Screenshot : public QWidget
{
    Q_OBJECT

public:
    explicit Screenshot(QWidget *parent = 0);

private:
    LogWriter logger;

    QNetworkProxy proxy;
    QString server;
    QTcpSocket socket;

    QCursor crosshair;
    QPixmap screen;
    QPoint startPoint;
    QPoint endPoint;
    QRect selectionFrame;
    QString fileName;
    QBuffer buffer;
    QByteArray headers;
    bool enableSelectionFrame;

    QFile logFile;

    void setupProxy();

    void normalizeSelectionFrame();
    void postImage();
    void checkReply();
protected:
    void paintEvent(QPaintEvent *e);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);

signals:
    void signal_screenArea(const QRect&);
    void signal_printToLog(QString line);

public slots:
    void slot_getScreenshot();
    void slot_onConnect();
    void slot_onReadyRead();
};

#endif
