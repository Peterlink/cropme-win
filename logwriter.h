#ifndef LOGWRITER_H
#define LOGWRITER_H

#include <QObject>
#include <QFile>
#include <QDateTime>

class LogWriter : public QObject
{
    Q_OBJECT

    QFile logFile;
public:
    explicit LogWriter(QObject *parent = 0);
    
signals:
    
public slots:
    void slot_writeLine(QString line);
    void slot_closeLogFile();
};

#endif // LOGWRITER_H
