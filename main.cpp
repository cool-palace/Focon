#include <QApplication>
#include <QMenuBar>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QGraphicsRectItem>
#include <QMainWindow>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.setWindowTitle("Модель фокона, версия 1.8");
    window.show();

    return app.exec();
}
