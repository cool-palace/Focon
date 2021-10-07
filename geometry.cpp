#include "geometry.h"
#include <QDebug>

Cone::Cone(qreal D1, qreal D2, qreal l) :
    l(l), tan_phi((D1-D2)/(2*l)), z_vertex(D1*l/(D1-D2)) {}

void Cone::setCone(qreal D1, qreal D2, qreal l_) {
    l = l_;
    tan_phi = (D1-D2)/(2*l);
    z_vertex = D1*l/(D1-D2);
}

Beam::Beam(Point p, qreal gamma) :
    p(p), cos_alpha(0), cos_beta(qSin(qDegreesToRadians(gamma))), cos_gamma(qCos(qDegreesToRadians(gamma))) {}

Beam::Beam(Point p, qreal alpha, qreal beta, qreal gamma) :
    p(p), cos_alpha(alpha), cos_beta(beta), cos_gamma(gamma) {}

Beam::Beam(QLineF beam_yoz) :
    p({0, beam_yoz.x1(), beam_yoz.y1()}), cos_alpha(0), cos_beta(beam_yoz.dy()), cos_gamma(beam_yoz.dx()) {}

void Beam::setBeam(Point p_, qreal gamma) {
    p = p_;
    cos_alpha = 0;
    cos_beta = qSin(qDegreesToRadians(gamma));
    cos_gamma = qCos(qDegreesToRadians(gamma));
}

Point Beam::intersection(Cone * cone) {
    Point p1, p2;
    qreal t1, t2;
    qreal a = pow(cos_a(), 2) + pow(cos_b(), 2) - pow(cos_g()*cone->tan_f(), 2);
    qreal b = 2 * (x() * cos_a() + y() * cos_b() - (z() - cone->z_k()) * cos_g() * pow(cone->tan_f(), 2));
    qreal c = pow(x(), 2) + pow(y(), 2) + (z() - cone->z_k()) * pow(cone->tan_f(), 2);
    qreal d = pow(b, 2) - 4*a*c;
    if (d >= 0) {
        t1 = (-b + qSqrt(d)) / (2*a);
        t2 = (-b - qSqrt(d)) / (2*a);
        qDebug() << x() + t1 * cos_a();
        p1 = {x() + t1 * cos_a(), y() + t1 * cos_b(), z() + t1 * cos_g()};
        p2 = {x() + t2 * cos_a(), y() + t2 * cos_b(), z() + t2 * cos_g()};
        if (p1.z > z() && p1.z <= cone->z_k()) {
            qDebug() << p1.x << p1.y << p1.z;
            return p1;
        } else if (p2.z > z() && p2.z <= cone->z_k()) {
            qDebug() << p2.x << p2.y << p2.z;
            return p2;
        }
    } else qDebug() << "Дискриминант < 0";

    return {0, 0, 0};
}

QLineF xoy_projection (Beam b, Point p) {
//	QLineF rad_vector = QLineF(p.x, p.y, 0, 0);
    QLineF beam_xoy = QLineF(p.x, p.y, b.x(), b.y());
    if (b.is_clockwise()) {
        beam_xoy.setAngle(beam_xoy.angle() - 90);
    } else {
        beam_xoy.setAngle(beam_xoy.angle() + 90);
    }
    return beam_xoy.unitVector();
}

QLineF yoz_projection (Beam b, Cone c, Point p) {
    QLineF line;
    if (p.y > 0) {
        line.setLine(c.z_k(), 0, 0, c.r1());
    } else {
        line.setLine(c.z_k(), 0, 0, -c.r1());
    }


//	QLineF rad_vector = qLineF(p.x, p.y, 0, 0);
    QLineF beam_xoy = QLineF(p.x, p.y, b.x(), b.y());
    if (b.is_clockwise()) {
        beam_xoy.setAngle(beam_xoy.angle() - 90);
    } else {
        beam_xoy.setAngle(beam_xoy.angle() + 90);
    }
    return beam_xoy.unitVector();
}

//Beam reflection(Beam b, Cone c, Point p) {
//    QLineF rad_vector = QLineF(0, 0, p.x, p.y);
//    QLineF p_norm = rad_vector.normalVector().unitVector(); // .translate(p.x, p.y)?
//    if (b.is_clockwise()) {
//        p_norm.setAngle(p_norm.angle() + 180);
//    }

//    //ksi = p_norm.angle();
//}

qreal Beam::transformation_angle(Point p) {
    QLineF rad_vector = QLineF(0, 0, p.x, p.y);
    return rad_vector.angle() - 90;
}

Beam transform(Beam b, Point p) {
    qreal ksi = Beam::transformation_angle(p);
    qreal c_alpha_tr = b.cos_a() * qCos(ksi) + b.cos_b() * qSin(ksi);
    qreal c_beta_tr = -b.cos_a() * qSin(ksi) + b.cos_b() * qCos(ksi);
    qreal c_gamma_tr = b.cos_g();
    return Beam(p, c_alpha_tr, c_beta_tr, c_gamma_tr);
}


Beam Beam::tangPart() {
    return Beam(p, cos_a(), 0, 0);
}

Beam Beam::reflPart() {
    return Beam(p, 0, cos_b(), cos_g());
}

Beam Beam::toUnit() {
    qreal beam_length = sqrt(pow(cos_a(),2) + pow(cos_b(),2) + pow(cos_g(),2));
    qreal cos_alpha_ = cos_a() / beam_length;
    qreal cos_beta_ = cos_b() / beam_length;
    qreal cos_gamma_ = cos_g() / beam_length;
    return Beam(p, cos_alpha_, cos_beta_, cos_gamma_);
}

//Beam Beam::operator+(const Beam& b1, const Beam& b2) {
//    Beam b = Beam(b1.p, b1.cos_a() + b2.cos_a(), b2.cos_b() + b1.cos_b(), b2.cos_g() + b1.cos_g());
//    return b.toUnit();
//}

//Beam::Beam(Point p, qPointF tang, qPointF refl) :
//    p(p), cos_alpha(tang.x()), cos_beta(refl.x()), cos_gamma(refl.y()) {}

Beam reflection_yoz(Cone c, Beam b, Point p) {
    // На входе reflPart
    QLineF line = QLineF(p.z, p.y, c.z_k(), 0);
    QLineF normal = line.normalVector();
    QLineF beam_yoz = QLineF(p.z, p.y, p.z - b.cos_g(), p.y - b.cos_b());
    qreal phi = beam_yoz.angleTo(normal);
    beam_yoz.setAngle(beam_yoz.angle() + 2*phi);
    Beam reflected = Beam(p, 0, beam_yoz.dx(), beam_yoz.dy());
    return reflected;
}


//	Beam p1p2 = b.tangPart() + reflection_yoz(c, b.refl_part(), b.intersection(c));

//    while (b.cos_g() > 0 && b.z() < c.length()) {
//        Point p_incident = b.intersection(c);
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

