#include <QApplication>
#include <QMenuBar>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QGraphicsRectItem>
#include <QMainWindow>
#include "layouts.h"
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    Layouts window;
    window.setFixedSize(1000,600);

    window.setWindowTitle("Модель фокона, версия 0.1");
    window.show();


//    auto mainWindow = new QMainWindow;
//    auto menuBar = new QMenuBar;
//    auto menu = new QMenu("Menu");
//    auto action = new QAction("Action");
//    menu->addAction(action);
//    menuBar->addMenu(menu);
//    mainWindow->setMenuBar(menuBar);

//    auto frame = new QFrame;
//    frame->setLayout(new QHBoxLayout);
//    mainWindow->setCentralWidget(frame);

//    auto scene = new QGraphicsScene();
//    auto view=new QGraphicsView(scene);
//    view->setScene(scene); // That connects the view with the scene
//    frame->layout()->addWidget(view);

//    QObject::connect(action, &QAction::triggered, [&]() {
//        scene->addItem(new QGraphicsRectItem(10, 10, 20, 20));
//    });
//    mainWindow->show();

    return app.exec();
}
