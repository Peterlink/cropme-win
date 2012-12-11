#include <QApplication>

#include "screenmanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QVector <QRect> screenGeometrys;
    quint32 screensTotal = app.desktop()->screenCount();
    for(quint32 i=0; i < screensTotal; i++)
    {
        screenGeometrys.append(app.desktop()->screenGeometry(i));
    }

    ScreenManager manager(screenGeometrys);

    return app.exec();
}
