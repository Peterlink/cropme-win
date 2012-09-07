#include <QApplication>

#include "screenshot.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Screenshot widget;
    widget.show();

    return app.exec();
}
