#include "qrcontroller.h"

#include <QtGui>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRController w;
    w.show();
    return a.exec();
}
