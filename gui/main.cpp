#include "../core/core.h"
#include <QApplication>
#include <QLabel>


int main(int argc, char **argv) {
  QApplication app(argc, argv);

  QLabel label(QString("Qt Widgets OK. CoreVersion=%1").arg(CoreVersion()));
  label.resize(360, 80);
  label.show();

  return app.exec();
}
