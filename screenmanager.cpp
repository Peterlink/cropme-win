#include "screenmanager.h"

ScreenManager::ScreenManager(QVector<QRect> geometrys, QObject *parent) : QObject(parent)
{
    screensTotal = geometrys.size();
    server = QString("cropme.ru");

    connect(this, SIGNAL(signal_printToLog(QString)), &logger, SLOT(slot_writeLine(QString)));

    emit signal_printToLog(QString("Cropme started for %1 screens").arg(screensTotal));

    widgets = new Screenshot*[screensTotal];

    for(quint32 i=0; i < screensTotal; i++)
    {
        widgets[i] = new Screenshot(0, i);

        connect(widgets[i], SIGNAL(signal_printToLog(QString)), &logger, SLOT(slot_writeLine(QString)));
        connect(widgets[i], SIGNAL(signal_postToServer()), SLOT(slot_onScreenshotReady()));

        widgets[i]->buffer = &(this->buffer);
        widgets[i]->setGeometry(geometrys[i]);
        widgets[i]->show();
    }

    connect(&socket, SIGNAL(connected()), this, SLOT(slot_onConnect()));
    connect(&socket, SIGNAL(readyRead()), this, SLOT(slot_onReadyRead()));
}

void ScreenManager::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Escape)
    {
        emit signal_printToLog("Escape pressed");
        logger.slot_closeLogFile();

        for(quint32 i=0; i < screensTotal; i++)
        {
            widgets[i]->close();
            delete widgets[i];
        }
        delete []widgets;
    }
}

void ScreenManager::setupProxy()
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
                        QMessageBox::warning(0, tr("Error"), tr("bad IP or URL."));
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

                    proxy.setType(QNetworkProxy::HttpProxy);
                    QNetworkProxy::setApplicationProxy(proxy);
                }
                else
                {
                    proxyConfig.close();
                    return;
                }
            }
            else
            {
                QMessageBox::warning(0, tr("Error"), tr("Wrong file format."));
                proxyConfig.close();
            }
        }
        else
        {
            QMessageBox::warning(0, tr("Warning"), tr("Failed to open proxy config file. Check permissions."));
        }
    }
}

void ScreenManager::slot_onScreenshotReady()
{
    for(quint32 i=0; i < screensTotal; i++)
    {
        widgets[i]->hide();
    }

    socket.connectToHost(server, 80);
}

void ScreenManager::slot_onConnect()
{
    postImage();
}

void ScreenManager::slot_onReadyRead()
{
    checkReply();
}

void ScreenManager::postImage()
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
    headers.append("Connection: close\r\n");
    headers.append("Accept-Encoding: identity\r\n");
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
        QMessageBox::warning(0, tr("Error"), tr("unwritable socket"));
    }
}

void ScreenManager::checkReply()
{
    QByteArray possibleLink;
    bool linkAccepted = false;
    QClipboard *clipboard = QApplication::clipboard();
    unsigned int fragmentationCounter = 0;

    emit signal_printToLog("Header received");

    QByteArray answer = socket.readLine();

    if(answer.contains("200 OK"))
    {
        do
        {
            while(socket.bytesAvailable())
            {
                possibleLink = socket.readLine();

                if(possibleLink.contains(server.toAscii()))
                {
                    linkAccepted = true;
                    emit signal_printToLog(QString("Fragmentation %1").arg(fragmentationCounter));
                }
            }
            if(!linkAccepted)
            {
                socket.waitForReadyRead(1000);
                fragmentationCounter++;
            }
        }
        while(!linkAccepted || fragmentationCounter > 3);

        QDesktopServices::openUrl(QString(possibleLink));
        clipboard->setText(possibleLink);
    }
    else
    {
        QMessageBox::warning(0, tr("Error"), tr(answer));
    }
    socket.close();

    for(quint32 i=0; i < screensTotal; i++)
    {
        widgets[i]->close();
        delete widgets[i];
    }
    delete []widgets;

    emit signal_printToLog("The end");
    logger.slot_closeLogFile();
}
