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

    setupProxy();
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
    QSettings proxyConfig("proxy.ini", QSettings::IniFormat);

    emit signal_printToLog(tr("Proxy config file: %1").arg(proxyConfig.fileName()));

    if(proxyConfig.value("proxy/proxy_enabled").toString() == "true")
    {
        QUrl proxyUrl = proxyConfig.value("proxy/proxy").toUrl();
        if(proxyUrl.isValid())
        {
            emit signal_printToLog(tr("Setting proxy to: %1").arg(proxyUrl.toString()));

            proxy.setHostName(proxyUrl.toString());

            quint16 proxyPort = proxyConfig.value("proxy/port").toUInt();
            if(proxyPort)
            {
                emit signal_printToLog(tr("Setting proxy port to: %1").arg(proxyPort));

                proxy.setPort(proxyPort);
            }
            else
            {
                emit signal_printToLog(tr("Setting proxy port to default"));

                proxy.setPort(DEFAULT_PROXY_PORT);
            }

            if(proxyConfig.value("proxy/authorization").toString() == "true")
            {
                emit signal_printToLog(tr("Setting proxy 'login:pass'"));

                proxy.setUser(proxyConfig.value("proxy/login").toString());
                proxy.setPassword(proxyConfig.value("proxy/pass").toString());
            }

            proxy.setType(QNetworkProxy::HttpProxy);
            QNetworkProxy::setApplicationProxy(proxy);
        }
        else
        {
            QMessageBox::warning(0, tr("Error"), tr("Wrong proxy URL: %1").arg(proxyUrl.toString()));
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
