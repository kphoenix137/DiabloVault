#include "MainWindow.h"
#include <QApplication>
#include <QCoreApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("DiabloVault");
    QCoreApplication::setApplicationName("DiabloVault");

    app.setStyle("Fusion");

    MainWindow w;
    w.show();
    return app.exec();
}
