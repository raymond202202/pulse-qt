#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("pulse-qt");
    app.setApplicationVersion(APP_VERSION);
    MainWindow w;
    w.show();
    return app.exec();
}
