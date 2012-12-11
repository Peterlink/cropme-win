#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <QApplication>
#include <QTimer>
#include <QDesktopWidget>
#include <QWidget>
#include <QMouseEvent>
#include <QRect>
#include <QMessageBox>
#include <QBuffer>

class Screenshot : public QWidget
{
    Q_OBJECT

public:
    QBuffer *buffer;
    explicit Screenshot(QWidget *parent = 0, quint32 screenNumber = 0);

private:

    QCursor crosshair;
    QPixmap screen;
    QPoint startPoint;
    QPoint endPoint;
    QRect selectionFrame;
    QString fileName;

    QByteArray headers;
    bool enableSelectionFrame;
    quint32 screenNumber;

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
    void signal_postToServer();

public slots:
    void slot_getScreenshot();
};

#endif
