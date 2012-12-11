#include "screenshot.h"
#include <QPainter>

Screenshot::Screenshot(QWidget *parent, quint32 screenNumber) : QWidget(parent)
{
    emit signal_printToLog("Cropme started");

    this->screenNumber = screenNumber;
    headers.reserve(4*0x1000);

    crosshair = QCursor(Qt::CrossCursor);
    setCursor(crosshair);
    setAutoFillBackground(false);
    setWindowState(Qt::WindowActive | Qt::WindowFullScreen);
    setWindowOpacity(0.1);
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
    emit signal_printToLog(QString("Screen %1 grabbing started").arg(screenNumber));

    QDesktopWidget *desktop = QApplication::desktop();
    QRect geometry = desktop->screenGeometry(screenNumber);

    screen =  QPixmap::grabWindow(desktop->winId(),
                                  geometry.left(),
                                  geometry.top(),
                                  geometry.width(),
                                  geometry.height());

    emit signal_printToLog("Screen grabbing ended");

    normalizeSelectionFrame();

    emit signal_printToLog("Selection normalized");

    screen = screen.copy(selectionFrame);

    emit signal_printToLog(QString("Selection made %1 %2").arg(screen.width()).arg(screen.height()).toAscii());

    int compression = 50;

    emit signal_printToLog("Bufferization started");

    buffer->open(QIODevice::WriteOnly);
    if(!screen.save(buffer, "PNG", compression))
    {
        QMessageBox::warning(this, tr("Error"), tr("unknown problem with buffer"));
    }
    buffer->close();

    emit signal_printToLog("Bufferization ended, connection started");

    emit signal_postToServer();
}


