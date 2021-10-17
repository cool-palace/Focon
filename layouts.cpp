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
    view->setScene(scene); // That connects the view with the scene

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
    // Deleting existing beams
    for (int i = 0; i < beams.size(); ++i) {
        //scene->removeItem(beams[i]);
        delete beams[i];
    }
    beams.clear();

    //scene->removeItem(xoy_projection);
    for (int i = 0; i < beams_xoy.size(); ++i) {
        //scene->removeItem(beams_xoy[i]);
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
//    start = {0, height->value(), 0};
    cone = Cone(d_in->value(), d_out->value(), length->value());
    beam = Beam(start, angle->value());
    focon_up->setLine(0, 250 - cone.r1() * scale, 800, 250 - cone.r2() * scale);
    focon_down->setLine(0, 250 + cone.r1() * scale, 800, 250 + cone.r2() * scale);

    points.push_back(start);
//    int i = 0;
    do {
        Point i_point = beam.intersection(cone);
        if (i_point.z() != -1) {
            qDebug() << "(пересечение)" << i_point.x() << ' ' << i_point.y() << ' ' << i_point.z();
            points.push_back(i_point);

            QLineF line = {0, 0, i_point.x(), i_point.y()};
            qDebug() << line.angle();
            qreal ksi = qDegreesToRadians(-90 + line.angle());
            qreal phi = cone.phi();
            qDebug() << "углы " << qRadiansToDegrees(ksi) << ' ' << qRadiansToDegrees(phi);
            Matrix m = Matrix(ksi, phi);

            Beam transformed_beam = m*beam.unit(i_point);
            transformed_beam.reflect();
            beam = m.transponed()*transformed_beam;
        } else break;
    } while (points.back().z() > 0 && points.back().z() < cone.length());
//    } while (points.back().z() > points[points.size()-2].z() && points.back().z() < cone.length());

    draw(rotation->value());


//    // calculating beam path
//    QPointF start_yoz, delta_yoz, incident;
//    QLineF beam_yoz;

//    do {
//        if (beams.empty()) {
//            // constructing first beam from input data
//            start_yoz = {0, 250 - height->value() * scale};
//            delta_yoz = {qCos(qDegreesToRadians(-angle->value())), qSin(qDegreesToRadians(-angle->value()))};
//            beam_yoz = {start_yoz, start_yoz + delta_yoz};
//        } else {
//            // calculating reflection
//            beam_yoz = {beams.back()->line().p2(), beams.back()->line().p1()};
//            QLineF normal = (beams.back()->line().angle() <= 90 && beams.back()->line().angle() < 270 ? focon_up : focon_down)->line().normalVector();
//            qreal phi = beam_yoz.angleTo(normal);
//            beam_yoz.setAngle(beam_yoz.angle() + 2*phi);
//        }

//        // setting a
//        beam_yoz.intersects((beam_yoz.angle() <= 180 ? focon_up : focon_down)->line(), &incident);
//        beam_yoz.setP2(incident);
//        beams.push_back(new QGraphicsLineItem(beam_yoz));
//        scene->addItem(beams.back());

//        if (incident.x()/scale >= cone.length()) {
//            for (int i = 0; i < beams.size(); ++i) {
//                beams[i]->setPen(QPen(Qt::green));
//            }
//            QString res;
//            qreal exit_angle = beams.back()->line().angle() < 180 ? beams.back()->line().angle() : 360 - beams.back()->line().angle();
//            res.setNum(exit_angle);
//            result->setPlainText(res);
//            scene->addItem(result);
//        }

//    } while (incident.x()/scale < cone.length() && incident.x()/scale >=0
//             && (beam_yoz.angle() < 90 || beam_yoz.angle() > 270));


}


