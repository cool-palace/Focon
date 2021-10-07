#include "layouts.h"
#include <QDebug>

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
    d_out->setValue(0.5);

    QLabel * label_length = new QLabel("Длина L", this);
    length = new QDoubleSpinBox(this);
    length->setMaximum(500);
    length->setValue(50);

    QLabel * label_angle = new QLabel("Угол вх", this);
    angle = new QDoubleSpinBox(this);
    angle->setMaximum(90);
    angle->setMinimum(-90);
    angle->setValue(5);
    angle->setSingleStep(0.5);

    QLabel * label_height = new QLabel("Высота луча", this);
    height = new QDoubleSpinBox(this);
    height->setMaximum(d_in->value()/2);
    height->setMinimum(-d_in->value()/2);
    height->setSingleStep(0.5);

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
    vbox->addWidget(calc);
    vbox->addStretch(1);

    scene = new QGraphicsScene();
    view = new QGraphicsView(scene);
    scene->setSceneRect(0, 0, 800, 500);
    view->setScene(scene); // That connects the view with the scene

    y_axis = new QGraphicsLineItem(0,0,0,500);
    z_axis = new QGraphicsLineItem(0,250,800,250);

    connect(calc, SIGNAL(clicked()), this, SLOT(draw()));

    scale = 800 / length->value();

    start = {0, height->value(), 0};
    beam = new Beam(start, angle->value());
    cone = new Cone(d_in->value(), d_out->value(), length->value());

    focon_up = new QGraphicsLineItem(0, 250 - cone->r1() * scale, 800, 250 - cone->r2() * scale);
    focon_down = new QGraphicsLineItem(0, 250 + cone->r1() * scale, 800, 250 + cone->r2() * scale);

    result = new QGraphicsTextItem();

    scene->addItem(y_axis);
    scene->addItem(z_axis);
    scene->addItem(focon_up);
    scene->addItem(focon_down);

    hbox->addLayout(vbox);
    hbox->addSpacing(15);
    hbox->addWidget(view);

    setLayout(hbox);
}



void Layouts::draw() {
    // Deleting existing beams
    for (int i = 0; i < beams.size(); ++i) {
        scene->removeItem(beams[i]);
        delete beams[i];
    }
    beams.clear();

    // rescaling
    scale = 800 / length->value();

    // updating spinboxes
    height->setMaximum(d_in->value()/2);
    height->setMinimum(-d_in->value()/2);

    // updating geometry
    start = {0, height->value(), 0};
    cone->setCone(d_in->value(), d_out->value(), length->value());
    beam->setBeam(start, angle->value());
    focon_up->setLine(0, 250 - cone->r1() * scale, 800, 250 - cone->r2() * scale);
    focon_down->setLine(0, 250 + cone->r1() * scale, 800, 250 + cone->r2() * scale);

    // calculating beam path
    QPointF start_yoz, delta_yoz, incident;
    QLineF beam_yoz;

    do {
        if (beams.empty()) {
            // constructing first beam from input data
            start_yoz = {0, 250 - height->value() * scale};
            delta_yoz = {qCos(qDegreesToRadians(-angle->value())), qSin(qDegreesToRadians(-angle->value()))};
            beam_yoz = {start_yoz, start_yoz + delta_yoz};
        } else {
            // calculating reflection
            beam_yoz = {beams.back()->line().p2(), beams.back()->line().p1()};
            QLineF normal = (beams.back()->line().angle() <= 90 && beams.back()->line().angle() < 270 ? focon_up : focon_down)->line().normalVector();
            qreal phi = beam_yoz.angleTo(normal);
            beam_yoz.setAngle(beam_yoz.angle() + 2*phi);
        }

        // setting a
        beam_yoz.intersects((beam_yoz.angle() <= 180 ? focon_up : focon_down)->line(), &incident);
        beam_yoz.setP2(incident);
        beams.push_back(new QGraphicsLineItem(beam_yoz));
        scene->addItem(beams.back());

        if (incident.x()/scale >= cone->length()) {
            for (int i = 0; i < beams.size(); ++i) {
                beams[i]->setPen(QPen(Qt::green));
            }
            QString res;
            qreal exit_angle = beams.back()->line().angle() < 180 ? beams.back()->line().angle() : 360 - beams.back()->line().angle();
            res.setNum(exit_angle);
            result->setPlainText(res);
            scene->addItem(result);
        }

    } while (incident.x()/scale < cone->length() && incident.x()/scale >=0
             && (beam_yoz.angle() < 90 || beam_yoz.angle() > 270));

//    Point p_incident = beam->intersection(cone);
//    QLineF line;
//    line.setP1({beam->z(), beam->y()});
//    line.setP2({p_incident.z, p_incident.y + 250});
//    auto beam1 = new QGraphicsLineItem(line);
//    scene->addItem(beam1);

//    while (beam->cos_g() > 0 && beam->z() < cone->length()) {
//        Point p_incident = beam->intersection(c);
//        qLineF line;

//        // Направления проверить потом
//        if (b.cos_b() >= 0) {
//            line.setP1(p_incident);
//            line.setP2({c_zk(), 0});
//        } else {
//            line.setP1({c_zk(), 0});
//            line.setP2(p_incident);
//        }

//        qLineF beam_yoz = qLineF(p_incident.z, p_incident.y, p_incident.z - b.cos_g(), p_incident.y - b.cos_b());
//        qreal phi = beam_yoz.angleTo(normal);
//        beam_yoz.setAngle(beam.angle() + 2*phi);
//        b = Beam(beam_yoz);
//    }

}
