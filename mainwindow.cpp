#include "mainwindow.h"
#include "ui_mainwindow.h"

constexpr qreal diameter = 150;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new QGraphicsScene())
    , y_axis(new QGraphicsLineItem(0,0,0,500))
    , z_axis(new QGraphicsLineItem(0,250,800,250))
    , focon_up(new QGraphicsLineItem())
    , focon_down(new QGraphicsLineItem())
    , circle(new QGraphicsEllipseItem())
    , circle_out(new QGraphicsEllipseItem())

{
    ui->setupUi(this);

    ui->view->setScene(scene);
    scene->setSceneRect(0, 0, 800, 600);

    init();

//    //result = new QGraphicsTextItem();

    scene->addItem(y_axis);
    scene->addItem(z_axis);
    scene->addItem(focon_up);
    scene->addItem(focon_down);
    scene->addItem(circle);
    scene->addItem(circle_out);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init() {
    // rescaling
    scale = 800 / ui->length->value();
    scale_xoy = diameter / qMax(ui->d_in->value(), ui->d_out->value());

    // updating spinboxes
    ui->height->setMaximum(ui->d_in->value()/2);
    ui->height->setMinimum(-ui->d_in->value()/2);
    ui->offset->setMaximum(ui->d_in->value()/2);
    ui->offset->setMinimum(-ui->d_in->value()/2);

    // updating cone's xoy projection
    qreal diameter_outer = qMin(ui->d_out->value(),ui->d_in->value()) * scale_xoy;
    qreal circle_out_x = scene->width() - diameter/2 - diameter_outer/2;
    qreal circle_out_y = diameter/2 - diameter_outer/2;
    circle_out->setRect(circle_out_x,circle_out_y, diameter_outer, diameter_outer);
    circle->setRect(scene->width()-diameter, 0, diameter, diameter);

    // updating geometry
    start = Point(-ui->offset->value(), -ui->height->value(), 0);
    if (cone != nullptr) delete cone;
    cone = (ui->d_in->value() != ui->d_out->value()
                ? new Cone(ui->d_in->value(), ui->d_out->value(), ui->length->value())
                : new Tube(ui->d_in->value(), ui->length->value()));
    beam = Beam(start, ui->angle->value());
    focon_up->setLine(0, 250 - cone->r1() * scale, 800, 250 - cone->r2() * scale);
    focon_down->setLine(0, 250 + cone->r1() * scale, 800, 250 + cone->r2() * scale);
}

void MainWindow::clear() {
    for (int i = 0; i < beams.size(); ++i) {
        delete beams[i];
    }
    beams.clear();

    for (int i = 0; i < beams_xoy.size(); ++i) {
        delete beams_xoy[i];
    }
    beams_xoy.clear();
}

void MainWindow::draw(int rotation_angle) {
    for (int i = 0; i < points.size()-1; ++i) {
        qreal theta = qDegreesToRadians(static_cast<qreal>(rotation_angle));
        QLineF line = QLineF(points[i].z() * scale, -(-points[i].y()*qCos(theta) + points[i].x()*qSin(theta)) * scale+250,
                             points[i+1].z()* scale, -(-points[i+1].y()*qCos(theta) + points[i+1].x()*qSin(theta))* scale+250);
        beams.push_back(new QGraphicsLineItem(line));
        scene->addItem(beams.back());


        QLineF line_xoy = QLineF(-(points[i].x()*qCos(theta) + points[i].y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                 -(points[i].x()*qSin(theta) - points[i].y()*qCos(theta))*scale_xoy + diameter/2,
                                 -(points[i+1].x()*qCos(theta) + points[i+1].y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                 -(points[i+1].x()*qSin(theta) - points[i+1].y()*qCos(theta))*scale_xoy + diameter/2);
        beams_xoy.push_back(new QGraphicsLineItem(line_xoy));
        scene->addItem(beams_xoy.back());

        if (points.back().z() > cone->length()) {
            beams[i]->setPen(QPen(Qt::green));
            beams_xoy[i]->setPen(QPen(Qt::green));
        }
    }
}

void MainWindow::rotate(int rotation_angle) {
    clear();
    draw(rotation_angle);
}

void MainWindow::build() {
    clear();
    points.clear();

    init();

    points.push_back(start);

    do {
        Point i_point = cone->intersection(beam);
        if (i_point.z() != -1) {
//            qDebug() << "(пересечение)" << i_point.x() << ' ' << i_point.y() << ' ' << i_point.z();
            points.push_back(i_point);

            QLineF line = {0, 0, i_point.x(), i_point.y()};
//            qDebug() << line.angle();
            qreal ksi = qDegreesToRadians(-90 + line.angle());
            qreal phi = cone->phi();
//            qDebug() << "углы " << qRadiansToDegrees(ksi) << ' ' << qRadiansToDegrees(phi);
            Matrix m = Matrix(ksi, phi);

            Beam transformed_beam = m*beam.unit(i_point);
            transformed_beam.reflect();
            beam = m.transponed()*transformed_beam;
        } else break;
    } while (points.back().z() > 0 && points.back().z() < cone->length());

    draw(ui->rotation->value());

}
