#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <QApplication>
#include <QTimer>
#include <QDesktopWidget>
#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QRect>
#include <QTcpSocket>
#include <QMessageBox>
#include <QBuffer>
#include <QUrl>
#include <QDesktopServices>

class Screenshot : public QWidget
{
    Q_OBJECT

public:
    explicit Screenshot(QWidget *parent = 0);

private:
    QString server;
    QTcpSocket socket;

    QCursor crosshair;
    QPixmap screen;
    QPoint startPoint;
    QPoint endPoint;
    QRect selectionFrame;
    QString fileName;
    QBuffer buffer;
    bool enableSelectionFrame;

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

public slots:
    void slot_getScreenshot();
    void slot_onConnect();
    void slot_onReadyRead();
};

#endif
