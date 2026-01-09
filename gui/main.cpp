#include "MainWindow.h"
#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    // Optional: keep this if you want a consistent style
    app.setStyle("Fusion");

    MainWindow w;
    w.show();
    return app.exec();
}
