#include "screenshot.h"
#include <QPainter>

Screenshot::Screenshot(QWidget *parent) : QWidget(parent)
{
    server = QString("cropme.ru");
    crosshair = QCursor(Qt::CrossCursor);
    setCursor(crosshair);
    setAutoFillBackground(false);
    setWindowState(Qt::WindowActive | Qt::WindowFullScreen);
    setWindowOpacity(0.1);
    enableSelectionFrame = false;

    connect(&socket, SIGNAL(connected()), this, SLOT(slot_onConnect()));
    connect(&socket, SIGNAL(readyRead()), this, SLOT(slot_onReadyRead()));
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

    int compression = 50;

    buffer.open(QIODevice::WriteOnly);
    if(!screen.save(&buffer, "PNG", compression))
    {
        QMessageBox::warning(this, tr("Error"), tr("unknown problem with buffer"));
    }
    buffer.close();

    socket.connectToHost(server, 80);
}

void Screenshot::slot_onConnect()
{
    postImage();
    qWarning("posted %d", buffer.size());
}

void Screenshot::slot_onReadyRead()
{
    checkReply();
    qWarning("checked");
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

    QByteArray headers;
    headers.append("POST /upload HTTP/1.1\r\n");
    headers.append("Content-Type: application/x-www-form-urlencoded\r\n");
    headers.append("Content-Length: ");
    headers.append(QString::number(imageData.size()).toAscii());
    headers.append("\r\n");
    headers.append("Connection: Keep-Alive\r\n");
    headers.append("Accept-Encoding: gzip\r\n");
    headers.append("Host: cropme.ru\r\n\r\n");

    if(socket.isWritable())
    {
        socket.write(headers);
        socket.write(imageData);
        socket.flush();
    }
    else
    {
        QMessageBox::warning(this, tr("Error"), tr("unwritable socket"));
    }
}

void Screenshot::checkReply()
{
    qWarning("waiting for data");
    socket.waitForReadyRead();
    QByteArray answer = socket.readLine();
    qWarning("!%s", QString(answer).toAscii().data());
    if(answer.contains("200 OK"))
    {
        while(socket.bytesAvailable())
        {
            QByteArray possibleLink = socket.readLine();
            if(possibleLink.contains(server.toAscii()))
            {
                QDesktopServices::openUrl(QString(possibleLink));
            }
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Error"), tr(answer));
    }
    socket.close();
    close();
}
