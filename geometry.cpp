#include "geometry.h"
#include <QDebug>
#include <iomanip>

QDebug& operator<<(QDebug debug, const Point& p) {
    debug << "Point (" << QString().setNum(p.x_, 'f', 6) << ", "
                       << QString().setNum(p.y_, 'f', 6) << ", "
                       << QString().setNum(p.z_, 'f', 6) << ")\n";
    return debug.noquote();
}

Vector::Vector(qreal x, qreal y, qreal z) {
    qreal length = sqrt(x*x + y*y + z*z);
    dx = x/length;
    dy = y/length;
    dz = z/length;
}

Vector::Vector(const Point& p1, const Point& p2) {
    qreal x = p2.x() - p1.x();
    qreal y = p2.y() - p1.y();
    qreal z = p2.z() - p1.z();
    qreal length = sqrt(x*x + y*y + z*z);
    dx = x/length;
    dy = y/length;
    dz = z/length;
}

Matrix::Matrix(qreal theta) {
    // Angle theta represents rotation around the Y axis
    a[0][0] = qCos(theta);
    a[0][1] = 0;
    a[0][2] = -qSin(theta);
    a[1][0] = 0;
    a[1][1] = 1;
    a[1][2] = 0;
    a[2][0] = qSin(theta);
    a[2][1] = 0;
    a[2][2] = qCos(theta);
}

Matrix::Matrix(qreal ksi, qreal phi) {
    // Angles ksi and phi represent rotation around Z and X axes respectively
    a[0][0] = qCos(ksi);
    a[0][1] = -qSin(ksi);
    a[0][2] = 0;
    a[1][0] = qSin(ksi)*qCos(phi);
    a[1][1] = qCos(ksi)*qCos(phi);
    a[1][2] = -qSin(phi);
    a[2][0] = qSin(ksi)*qSin(phi);
    a[2][1] = qCos(ksi)*qSin(phi);
    a[2][2] = qCos(phi);
}

Matrix::Matrix(qreal ksi, qreal phi, qreal theta) {
    // Angles ksi, phi and theta represent rotation around Z, X and Y axes respectively
    a[0][0] = qCos(ksi)*qCos(theta) - qSin(ksi)*qSin(phi)*qSin(theta);
    a[0][1] = -qSin(ksi)*qCos(theta) - qCos(ksi)*qSin(phi)*qSin(theta);
    a[0][2] = -qCos(phi)*qSin(theta);
    a[1][0] = qSin(ksi)*qCos(phi);
    a[1][1] = qCos(ksi)*qCos(phi);
    a[1][2] = -qSin(phi);
    a[2][0] = qSin(ksi)*qSin(phi)*qCos(theta) + qCos(ksi)*qSin(theta);
    a[2][1] = qCos(ksi)*qSin(phi)*qCos(theta) - qSin(ksi)*qSin(theta);
    a[2][2] = qCos(phi)*qCos(theta);
}

Matrix Matrix::transponed() {
    std::swap(a[0][1], a[1][0]);
    std::swap(a[0][2], a[2][0]);
    std::swap(a[1][2], a[2][1]);
    return *this;
}

Beam Matrix::operator* (const Beam& b) {
    qreal dx = b.d_x()*a[0][0] + b.d_y()*a[0][1] + b.d_z()*a[0][2];
    qreal dy = b.d_x()*a[1][0] + b.d_y()*a[1][1] + b.d_z()*a[1][2];
    qreal dz = b.d_x()*a[2][0] + b.d_y()*a[2][1] + b.d_z()*a[2][2];
    return Beam(b.p1(), dx, dy, dz);
}

QDebug& operator<<(QDebug debug, const Matrix& m) {
    debug << "Matrix (\t" << QString().setNum(m.a[0][0], 'f', 6) << ' '
                          << QString().setNum(m.a[0][1], 'f', 6) << ' '
                          << QString().setNum(m.a[0][2], 'f', 6) << "\n\t"
                          << QString().setNum(m.a[1][0], 'f', 6) << ' '
                          << QString().setNum(m.a[1][1], 'f', 6) << ' '
                          << QString().setNum(m.a[1][2], 'f', 6) << "\n\t"
                          << QString().setNum(m.a[2][0], 'f', 6) << ' '
                          << QString().setNum(m.a[2][1], 'f', 6) << ' '
                          << QString().setNum(m.a[2][2], 'f', 6) << ")\n";
    return debug.noquote();
}

bool Tube::is_conic() { return dynamic_cast<Cone*>(this); }

QDebug& operator<<(QDebug debug, const Beam& b) {
    debug << "Beam (" << b.p << "dx = " << QString().setNum(b.d_x(), 'f', 6) << ", "
                             << "dy = " << QString().setNum(b.d_y(), 'f', 6) << ", "
                             << "dz = " << QString().setNum(b.d_z(), 'f', 6) << "\n";
    return debug.noquote();
}

Beam Tube::reflected(const Beam& beam, const Point& intersection) const {
    QLineF line = QLineF(0, 0, intersection.x(), intersection.y());
    qreal ksi = qDegreesToRadians(-90 + line.angle());
    qreal phi = this->phi();
    Matrix m = Matrix(ksi, phi);
    Beam transformed_beam = m*beam.on_point(intersection);
    transformed_beam.reflect();
    return m.transponed()*transformed_beam;
}

Beam Tube::refracted(const Beam &beam) const {
    qreal length_xz = sqrt(beam.d_x()*beam.d_x() + beam.d_z()*beam.d_z());
    qreal n_ratio = beam.d_y() < 0 ? 1.0/1.5 : 1.5;
    qreal sin_new_beta = qSin(qAcos(beam.d_y()))*n_ratio;
    if (sin_new_beta > 1 || beam.d_y() < 0) {
        return Beam(beam.p1(), Vector(beam.d_x(), -beam.d_y(), beam.d_z()));
    }
    qreal new_beta = qAsin(sin_new_beta);
    qreal dx = beam.d_x() * qSin(new_beta) / length_xz;
    qreal dz = beam.d_z() * qSin(new_beta) / length_xz;
    return Beam(beam.p1(), Vector(dx, qCos(new_beta), dz));
}

Point Tube::intersection(const Beam &beam) const {
    // The only case when the beam does not intersect the tube is when the input angle == 0
    // Then the exiting point coordinates in XOY are equal to the input coordinates
    if (qFabs(beam.d_y()) < 1e-6) return Point(beam.x(), beam.y(), 2*length());
    // Else the intersection point can be found by finding the bigger root of the quadratic equation
    qreal a = pow(beam.cos_a(), 2) + pow(beam.cos_b(), 2);
    qreal b = 2 * (beam.x() * beam.cos_a() + beam.y() * beam.cos_b());
    qreal c = pow(beam.x(), 2) + pow(beam.y(), 2) - pow(r1(), 2);
    qreal d = pow(b, 2) - 4*a*c;
    qreal t = (-b + qSqrt(d)) / (2*a);
    return Point(beam.x() + t*beam.cos_a(), beam.y() + t*beam.cos_b(), beam.z() + t*beam.cos_g());
}

Point Cone::intersection(const Beam& beam) const {
    // Axial beam intersects the cone's vertex and passes
    if (qFabs(beam.d_y()) < 1e-6 && qFabs(beam.x()) < 1e-6 && qFabs(beam.y()) < 1e-6) {
        return Point(0, 0, z_k() * (r1() > r2() ? 1 : -length()));
    }
    qreal a = pow(beam.cos_a(), 2) + pow(beam.cos_b(), 2) - pow(beam.cos_g()*tan_phi(), 2);
    qreal b = 2 * (beam.x() * beam.cos_a() + beam.y() * beam.cos_b() - (beam.z() - z_k()) * beam.cos_g() * pow(tan_phi(), 2));
    qreal c = pow(beam.x(), 2) + pow(beam.y(), 2) - pow((beam.z() - z_k()) * tan_phi(), 2);
    qreal d = pow(b, 2) - 4*a*c;
    // There might be a case when 'd' turns out to be a miniscule negative number due to some floating point ambiguities
    // Negative 'd' means that the beam does not intersect the cone at all, which is impossible
    // So this case should be fixed by setting 'd' to zero
    if (d < 0 && fabs(d) < 1e-8) d = 0;
    // If (a == 0) then the equation degenerates into linear equation
//    qDebug() << "roots" << (-b + qSqrt(d))/(2*a) << (-b - qSqrt(d))/(2*a);
    qreal t1 = (-b + qSqrt(d))/(2*a);
    qreal t2 = (-b - qSqrt(d))/(2*a);
    // Case 1: t1 > 0, t2 > 0. Beam's starting point does not belong the cone's surface.

    qreal t = qFabs(a) > 1e-9
            ? t2 > 0 && qFabs(t2) > 1e-9
              ? qFabs(t1) > 1e-9
                ? qMin(t1, t2)
                : t2
              : /*qFabs(t1) > 1e-9 ? */ t1 //: t2
            : -c/b;
//    qDebug() << t << " selected";
    Point p;
    if (qFabs(t) < 1e-8) {
        if (t2 < -1e-6) {
            t = t2;
            p = Point(beam.x() - t*beam.cos_a(), beam.y() - t*beam.cos_b(), beam.z() - t*beam.cos_g());
        } else throw beam_exception();
    } else p = Point(beam.x() + t*beam.cos_a(), beam.y() + t*beam.cos_b(), beam.z() + t*beam.cos_g());
//    qDebug() << "check" << qFabs(qSqrt(p.x()*p.x() + p.y()*p.y())) << (z_k() - p.z())*tan_phi();

    bool correct_root = qFabs(qSqrt(p.x()*p.x() + p.y()*p.y()) - (z_k() - p.z())*tan_phi()) < 1e-6;
    if (!correct_root && d1() < 1e-6 /*&& qFabs(t2) > 1e-6*/) {
        t = (t == t1) ? t2 : t1;
        p = Point(beam.x() + t*beam.cos_a(), beam.y() + t*beam.cos_b(), beam.z() + t*beam.cos_g());
//        qDebug() << "check failed " << t << " selected instead";
//        qDebug() << "check" << qFabs(qSqrt(p.x()*p.x() + p.y()*p.y())) << (z_k() - p.z())*tan_phi();
    }

    correct_root = qFabs(qSqrt(p.x()*p.x() + p.y()*p.y()) - (z_k() - p.z())*tan_phi()) < 1e-6;
    if (!correct_root) {
        // False root means that the beam goes outward without intersecting the cone's surface
        // (the intersection point is located on the imaginary side).
        // So the resulting point does not have to belong to the cone's surface
        // But it has to be located outside of the cone so that no false beams appear from it and the calculations stop.
        while (d1() > 1e-6 && ((d1() > d2() && beam.z() - t*beam.cos_g() > 0)
               || (d1() < d2() && beam.z() - t*beam.cos_g() < z_offset + length()))) {
            t *= 2;
        }
        p = Point(beam.x() - t*beam.cos_a(), beam.y() - t*beam.cos_b(), beam.z() - t*beam.cos_g());
    }
    return p;
}

Point Plane::intersection(const Beam &beam) const {
    qreal val = (z() - beam.z()) / beam.cos_g();
    qreal x = val * beam.cos_a() + beam.x();
    qreal y = val * beam.cos_b() + beam.y();
    return Point(x, y, z());
}

Point Detector::intersection(const Beam &beam, qreal z) const {
    return Plane(z).intersection(beam);
}

Beam Lens::refracted(const Beam &beam) const {
    Beam meridional = Beam(center(), beam.d_x(), beam.d_y(), beam.d_z());
    Point p2 = focal_plane().intersection(meridional);
    return focus > 0 ? Beam(beam.p1(), p2) : Beam(p2, beam.p1()).on_point(beam.p1());
}

Beam Plane::refracted(const Beam &beam, qreal n1, qreal n2) const {
    if (qFabs(beam.d_y()) < 1e-6 && qFabs(beam.d_x()) < 1e-6) {
        return beam;
    }
    qreal length_xy = sqrt(beam.d_x()*beam.d_x() + beam.d_y()*beam.d_y());
    qreal sin_new_gamma = qSin(qAcos(beam.d_z()))*n1/n2;
    if (sin_new_gamma > 1) {
        return Beam(beam.p1(), Vector(beam.d_x(), beam.d_y(), -beam.d_z()));
    }
    qreal new_gamma = qAsin(sin_new_gamma);
    qreal dx = beam.d_x() * qSin(new_gamma) / length_xy;
    qreal dy = beam.d_y() * qSin(new_gamma) / length_xy;
    return Beam(beam.p1(), Vector(dx, dy, qCos(new_gamma)));
}
