#include "screenshot.h"
#include <QPainter>

Screenshot::Screenshot(QWidget *parent) : QWidget(parent)
{
    server = QUrl(QString("http://cropme.ru/upload"));
    crosshair = QCursor(Qt::CrossCursor);
    setCursor(crosshair);
    setAutoFillBackground(false);
    setWindowState(Qt::WindowActive | Qt::WindowFullScreen);
    setWindowOpacity(0.01);
    enableSelectionFrame = false;
}

void Screenshot::normalizeSelectionFrame()
{
//  2 release
//   \
//    \
//     1 click

    if((startPoint.x() > endPoint.x()) && (startPoint.y() > endPoint.y()))
    {
        selectionFrame = QRect(endPoint, startPoint);
    }

//        1 click
//       /
//      /
//     /
//    /
//   /
//  2 release

    if((startPoint.x() > endPoint.x()) && (startPoint.y() < endPoint.y()))
    {
        selectionFrame = QRect(endPoint.x(),
                               startPoint.y(),
                               startPoint.x() - endPoint.x(),
                               endPoint.y() - startPoint.y());
    }
//        2 release
//       /
//      /
//     /
//    /
//   /
//  1 click

    if((startPoint.x() < endPoint.x()) && (startPoint.y() > endPoint.y()))
    {
        selectionFrame = QRect(startPoint.x(),
                               endPoint.y(),
                               endPoint.x() - startPoint.x(),
                               startPoint.y() - endPoint.y());
    }
//  1 click
//   \
//    \
//     2 release

    if((startPoint.x() < endPoint.x()) && (startPoint.y() < endPoint.y()))
    {
        selectionFrame = QRect(startPoint, endPoint);
    }
}

void Screenshot::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    QBrush brush(Qt::SolidPattern);

    if(enableSelectionFrame)
    {
        painter.drawRect(QRect(startPoint, endPoint));
        painter.fillRect(QRect(startPoint, endPoint), brush);
    }
}

void Screenshot::mousePressEvent(QMouseEvent *e)
{
    startPoint = e->pos();
    endPoint = e->pos();
    enableSelectionFrame = true;
}

void Screenshot::mouseMoveEvent(QMouseEvent *e)
{
    endPoint = e->pos();
    update();
}

void Screenshot::mouseReleaseEvent(QMouseEvent *e)
{
    enableSelectionFrame = false;
    setWindowOpacity(0.0);
    QTimer::singleShot(100, this, SLOT(slot_getScreenshot()));
}

void Screenshot::slot_getScreenshot()
{
    screen =  QPixmap::grabWindow(QApplication::desktop()->winId());
    normalizeSelectionFrame();
    screen = screen.copy(selectionFrame);

    buffer.open(QIODevice::WriteOnly);
    screen.save(&buffer, "PNG", 100);
    buffer.close();

    postImage();
}

void Screenshot::postImage()
{
    buffer.open(QIODevice::ReadOnly);
    QByteArray base64image(buffer.readAll().toBase64());
    buffer.close();

    QByteArray imageData("image=");
    imageData.append(base64image);
    imageData.replace("/", "%2F");
    imageData.replace("+", "%2B");

    request.setUrl(server);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    reply = manager.post(request, imageData.data());
    connect(reply, SIGNAL(finished()), this, SLOT(slot_replyFinished()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(slot_checkReply()));
}



void Screenshot::slot_replyFinished()
{
//    reply->deleteLater();
}

void Screenshot::slot_checkReply()
{
    QByteArray answer = reply->readAll();
    if(!reply->error())
    {
        QDesktopServices::openUrl(QString("http://cropme.ru/%1.png").arg(QString(answer)));
    }
    reply->deleteLater();
    close();
}
