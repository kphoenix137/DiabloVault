#include "MainWindow.h"
#include <qapplication.h>
#include <qtenvironmentvariables.h>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    app.setStyle("Fusion");  // <— add this

    MainWindow w;
    w.show();
    return app.exec();
}
