#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("BlockByte");
    QCoreApplication::setApplicationName("BlockByteLauncher");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
