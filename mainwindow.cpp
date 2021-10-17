#include "mainwindow.h"
#include <QDebug>



MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox = new QHBoxLayout(this);

    QLabel * label_d_in = new QLabel("D вх", this);
    d_in = new QDoubleSpinBox(this);
    d_in->setSingleStep(0.5);
    d_in->setValue(25);

    QLabel * label_d_out = new QLabel("D вых", this);
    d_out = new QDoubleSpinBox(this);
    d_out->setSingleStep(0.5);
    d_out->setValue(0.5);

    QLabel * label_length = new QLabel("Длина L", this);
    length = new QDoubleSpinBox(this);
    length->setMaximum(500);
    length->setValue(100);

    QLabel * label_angle = new QLabel("Угол вх", this);
    angle = new QDoubleSpinBox(this);
    angle->setSingleStep(0.5);



    calc = new QPushButton("Рассчитать", this);

    vbox->setSpacing(3);
    vbox->addStretch(1);
    vbox->addWidget(label_d_in);
    vbox->addWidget(d_in);
    vbox->addWidget(label_d_out);
    vbox->addWidget(d_out);
    vbox->addWidget(label_length);
    vbox->addWidget(length);
    vbox->addWidget(label_angle);
    vbox->addWidget(angle);

    vbox->addWidget(calc);
    vbox->addStretch(1);

    scene = new QGraphicsScene();
    view = new QGraphicsView(scene);
    scene->setSceneRect(0, 0, 800, 500);
    view->setScene(scene); // That connects the view with the scene

    y_axis = new QGraphicsLineItem(0,0,0,500);
    z_axis = new QGraphicsLineItem(0,250,800,250);

    connect(calc, SIGNAL(clicked()), this, SLOT(move()));

    qreal scale = 800 / length->value();
    QGraphicsLineItem * focon_up = new QGraphicsLineItem(0, 250 - d_in->value() * scale, 800, 250 - d_out->value() * scale);

    QGraphicsLineItem * focon_down = new QGraphicsLineItem(0, 250 + d_in->value() * scale, 800, 250 + d_out->value() * scale);

    scene->addItem(y_axis);
    scene->addItem(z_axis);
    scene->addItem(focon_up);
    scene->addItem(focon_down);

    hbox->addLayout(vbox);
    hbox->addSpacing(15);
    hbox->addWidget(view);



    setLayout(hbox);
}

MainWindow::~MainWindow()
{
}


void MainWindow::move() {
    qDebug() << '1';
//    y_axis->setPos(y_axis->x()+10, y_axis->y());
//    scene->addItem(y_axis);
}




