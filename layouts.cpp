#include "layouts.h"
#include <QDebug>

constexpr qreal diameter = 150;

Layouts::Layouts(QWidget *parent) : QWidget(parent)
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
    d_out->setValue(5);

    QLabel * label_length = new QLabel("Длина L", this);
    length = new QDoubleSpinBox(this);
    length->setMaximum(500);
    length->setValue(50);

    QLabel * label_angle = new QLabel("Угол вх", this);
    angle = new QDoubleSpinBox(this);
    angle->setMaximum(90);
    angle->setMinimum(-90);
    angle->setValue(10);
    angle->setSingleStep(0.5);

    QLabel * label_height = new QLabel("Верт. смещение луча", this);
    height = new QDoubleSpinBox(this);
    height->setMaximum(d_in->value()/2);
    height->setMinimum(-d_in->value()/2);
    height->setSingleStep(0.5);

    QLabel * label_offset = new QLabel("Гор. смещение луча", this);
    offset = new QDoubleSpinBox(this);
    offset->setMaximum(d_in->value()/2);
    offset->setMinimum(-d_in->value()/2);
    offset->setSingleStep(0.5);

    QLabel * label_rotation = new QLabel("Поворот", this);
    rotation = new QDial(this);
    rotation->setSingleStep(1);
    rotation->setMaximum(360);
    rotation->setMinimum(0);
    rotation->setValue(0);

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
    vbox->addWidget(label_height);
    vbox->addWidget(height);
    vbox->addWidget(label_offset);
    vbox->addWidget(offset);
    vbox->addWidget(label_rotation);
    vbox->addWidget(rotation);
    vbox->addWidget(calc);
    vbox->addStretch(1);

    scene = new QGraphicsScene();
    view = new QGraphicsView(scene);
    scene->setSceneRect(0, 0, 800, 600);
    view->setScene(scene);

    y_axis = new QGraphicsLineItem(0,0,0,500);
    z_axis = new QGraphicsLineItem(0,250,800,250);

    connect(calc, SIGNAL(clicked()), this, SLOT(build()));
    connect(rotation, SIGNAL(valueChanged(int)), this, SLOT(rotate(int)));

    scale = 800 / length->value();
    scale_xoy = diameter / d_in->value();

    start = {0, height->value(), 0};
    beam = Beam(start, 0, qSin(angle->value()), qCos(angle->value()));
    cone = Cone(d_in->value(), d_out->value(), length->value());

    focon_up = new QGraphicsLineItem(0, 250 - cone.r1() * scale, 800, 250 - cone.r2() * scale);
    focon_down = new QGraphicsLineItem(0, 250 + cone.r1() * scale, 800, 250 + cone.r2() * scale);

    circle = new QGraphicsEllipseItem(scene->width()-diameter, 0, diameter, diameter);
    qreal diameter_outer = diameter*d_out->value()/d_in->value();
    qreal circle_out_x = scene->width() - diameter/2 - diameter_outer/2;
    qreal circle_out_y = diameter/2 - diameter_outer/2;
    circle_out = new QGraphicsEllipseItem(circle_out_x,circle_out_y, diameter_outer, diameter_outer);

    result = new QGraphicsTextItem();

    scene->addItem(y_axis);
    scene->addItem(z_axis);
    scene->addItem(focon_up);
    scene->addItem(focon_down);
    scene->addItem(circle);
    scene->addItem(circle_out);

    hbox->addLayout(vbox);
    hbox->addSpacing(15);
    hbox->addWidget(view);

    setLayout(hbox);
}

void Layouts::clear() {
    for (int i = 0; i < beams.size(); ++i) {
        delete beams[i];
    }
    beams.clear();

    for (int i = 0; i < beams_xoy.size(); ++i) {
        delete beams_xoy[i];
    }
    beams_xoy.clear();
}

void Layouts::draw(int rotation_angle) {
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

        if (points.back().z() > cone.length()) {
            beams[i]->setPen(QPen(Qt::green));
            beams_xoy[i]->setPen(QPen(Qt::green));
        }
    }
}

void Layouts::rotate(int rotation_angle) {
    clear();
    draw(rotation_angle);
}

void Layouts::build() {
    clear();
    points.clear();

    // rescaling
    scale = 800 / length->value();
    scale_xoy = diameter / d_in->value();

    // updating spinboxes
    height->setMaximum(d_in->value()/2);
    height->setMinimum(-d_in->value()/2);
    d_out->setMaximum(d_in->value()-0.5);

    // updating cone's xoy projection
    qreal diameter_outer = diameter*d_out->value()/d_in->value();
    qreal circle_out_x = scene->width() - diameter/2 - diameter_outer/2;
    qreal circle_out_y = diameter/2 - diameter_outer/2;
    circle_out->setRect(circle_out_x, circle_out_y,diameter_outer,diameter_outer);

    // updating geometry
    start = {-offset->value(), -height->value(), 0};
    cone = Cone(d_in->value(), d_out->value(), length->value());
    beam = Beam(start, angle->value());
    focon_up->setLine(0, 250 - cone.r1() * scale, 800, 250 - cone.r2() * scale);
    focon_down->setLine(0, 250 + cone.r1() * scale, 800, 250 + cone.r2() * scale);

    points.push_back(start);

    do {
        Point i_point = beam.intersection(cone);
        if (i_point.z() != -1) {
//            qDebug() << "(пересечение)" << i_point.x() << ' ' << i_point.y() << ' ' << i_point.z();
            points.push_back(i_point);

            QLineF line = {0, 0, i_point.x(), i_point.y()};
//            qDebug() << line.angle();
            qreal ksi = qDegreesToRadians(-90 + line.angle());
            qreal phi = cone.phi();
//            qDebug() << "углы " << qRadiansToDegrees(ksi) << ' ' << qRadiansToDegrees(phi);
            Matrix m = Matrix(ksi, phi);

            Beam transformed_beam = m*beam.unit(i_point);
            transformed_beam.reflect();
            beam = m.transponed()*transformed_beam;
        } else break;
    } while (points.back().z() > 0 && points.back().z() < cone.length());

    draw(rotation->value());

}


