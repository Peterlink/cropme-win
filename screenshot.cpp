#include "screenshot.h"
#include <QPainter>

Screenshot::Screenshot(QWidget *parent) : QWidget(parent)
{
    emit signal_printToLog("Cropme started");

    setupProxy();

    headers.reserve(4*0x1000);
    server = QString("cropme.ru");
    crosshair = QCursor(Qt::CrossCursor);
    setCursor(crosshair);
    setAutoFillBackground(false);
    setWindowState(Qt::WindowActive | Qt::WindowFullScreen);
    setWindowOpacity(0.1);
    enableSelectionFrame = false;

    connect(&socket, SIGNAL(connected()), this, SLOT(slot_onConnect()));
    connect(&socket, SIGNAL(readyRead()), this, SLOT(slot_onReadyRead()));

    connect(this, SIGNAL(signal_printToLog(QString)), &logger, SLOT(slot_writeLine(QString)));
}

void Screenshot::setupProxy()
{
    QFile proxyConfig("proxy.ini");
    if(proxyConfig.exists())
    {
        if(proxyConfig.open(QIODevice::ReadOnly))
        {
            QString firstLine = proxyConfig.readLine();
            if(firstLine.contains("proxy_enabled"))
            {
                if(firstLine.contains("true") && !firstLine.contains("false"))
                {
                    QString secondLine = proxyConfig.readLine();

                    QRegExp ipv4RegExp("[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}");
                    QRegExp URLRegExp("(https?:\/\/)?(([0-9a-z_!~*'().&=+$%-]+:)?[0-9a-z_!~*'().&=+$%-]+@)?(([0-9]{1,3}\.){3}[0-9]{1,3}|([0-9a-z_!~*'()-]+\.)*([0-9a-z][0-9a-z-]{0,61})+[0-9a-z]\.[a-z]{2,6})(:[0-9]{1,4})?((\/?)|(\/[0-9a-z_!~*'().;?:@&=+$,%#-]+)+\/?)");
                    QRegExp portNumRegExp("[1-9]{1}[0-9]{0,4}");

                    if(secondLine.contains(ipv4RegExp))
                    {
                        secondLine = ipv4RegExp.cap(0);
                        proxy.setHostName(secondLine);
                    }
                    else if(secondLine.contains(URLRegExp))
                    {
                        secondLine = URLRegExp.cap(0);
                        proxy.setHostName(secondLine);
                    }
                    else
                    {
                        QMessageBox::warning(this, tr("Error"), tr("bad IP or URL."));
                        proxyConfig.close();
                        return;
                    }

                    QString thirdLine = proxyConfig.readLine();
                    if(thirdLine.contains(portNumRegExp))
                    {
                        thirdLine = portNumRegExp.cap(0);
                        proxy.setPort(thirdLine.toUShort());
                    }
                    else
                    {
                        proxy.setPort(80);
                    }

                    socket.setProxy(proxy);
                }
                else
                {
                    qWarning("proxy disabled");
                    proxyConfig.close();
                    return;
                }
            }
            else
            {
                QMessageBox::warning(this, tr("Error"), tr("Wrong file format."));
                proxyConfig.close();
            }
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Failed to open proxy config file. Check permissions."));
        }
    }
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

    emit signal_printToLog("Mouse pressed");
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

    emit signal_printToLog("Mouse released");

    QTimer::singleShot(100, this, SLOT(slot_getScreenshot()));
}

void Screenshot::slot_getScreenshot()
{
    emit signal_printToLog("Screen grabbing started");

    screen =  QPixmap::grabWindow(QApplication::desktop()->winId());

    emit signal_printToLog("Screen grabbing ended");

    normalizeSelectionFrame();

    emit signal_printToLog("Selection normalized");

    screen = screen.copy(selectionFrame);

    emit signal_printToLog(QString("Selection made %1 %2").arg(screen.width()).arg(screen.height()).toAscii());

    int compression = 50;

    emit signal_printToLog("Bufferization started");

    buffer.open(QIODevice::WriteOnly);
    if(!screen.save(&buffer, "PNG", compression))
    {
        QMessageBox::warning(this, tr("Error"), tr("unknown problem with buffer"));
    }
    buffer.close();

    emit signal_printToLog("Bufferization ended, connection started");

    socket.connectToHost(server, 80);
}

void Screenshot::slot_onConnect()
{
    postImage();
}

void Screenshot::slot_onReadyRead()
{
    checkReply();
}

void Screenshot::postImage()
{
    emit signal_printToLog("Buffer reading");

    buffer.open(QIODevice::ReadOnly);
    QByteArray base64image(buffer.readAll().toBase64());
    buffer.close();

    emit signal_printToLog(QString("Buffer read: %1").arg(base64image.size()));

    emit signal_printToLog("Forming request");

    QByteArray imageData("image=");
    imageData.append(base64image);
    imageData.replace("/", "%2F");
    imageData.replace("+", "%2B");

    headers.append("POST /upload HTTP/1.1\r\n");
    headers.append("Content-Type: application/x-www-form-urlencoded\r\n");
    headers.append("Content-Length: ");
    headers.append(QString::number(imageData.size()).toAscii());
    headers.append("\r\n");
    headers.append("Connection: Keep-Alive\r\n");
    headers.append("Accept-Encoding: gzip\r\n");
    headers.append("Host: cropme.ru\r\n\r\n");

    emit signal_printToLog("Request formed, sending");

    if(socket.isWritable())
    {
        socket.write(headers);
        socket.write(imageData);
        socket.flush();

        emit signal_printToLog("All sent");
    }
    else
    {
        QMessageBox::warning(this, tr("Error"), tr("unwritable socket"));
    }
}

void Screenshot::checkReply()
{
    emit signal_printToLog("Header received");

    QByteArray answer = socket.readLine();
    if(answer.contains("200 OK"))
    {
        while(socket.bytesAvailable())
        {
            QByteArray possibleLink = socket.readLine();
            if(possibleLink.contains(server.toAscii()))
            {
                emit signal_printToLog("Browser 1 opening");

                QDesktopServices::openUrl(QString(possibleLink));
            }
        }
        socket.waitForReadyRead(1000);
        while(socket.bytesAvailable())
        {
            QByteArray possibleLink = socket.readLine();
            if(possibleLink.contains(server.toAscii()))
            {
                emit signal_printToLog("Browser 2 opening");

                QDesktopServices::openUrl(QString(possibleLink));
            }
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Error"), tr(answer));
    }
    socket.close();

    emit signal_printToLog("The end");

    logger.slot_closeLogFile();

    close();
}
