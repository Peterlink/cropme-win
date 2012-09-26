#include "logwriter.h"

LogWriter::LogWriter(QObject *parent) : QObject(parent)
{
    logFile.setFileName("cropme_log.txt");
    logFile.open(QIODevice::WriteOnly);
}

void LogWriter::slot_writeLine(QString line)
{
    if(logFile.isOpen())
    {
        logFile.write(QDateTime::currentDateTime().toString("dd.MM.yy hh.mm.ss.zzz").toAscii() + ": " + line.toAscii() + "\r\n");
        logFile.flush();
    }
}

void LogWriter::slot_closeLogFile()
{
    if(logFile.isOpen())
    {
        logFile.close();
    }
}
