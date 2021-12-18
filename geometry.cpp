#include "geometry.h"
#include <QDebug>

Matrix::Matrix(qreal ksi, qreal phi) {
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

Matrix Matrix::transponed() {
    std::swap(a[0][1], a[1][0]);
    std::swap(a[0][2], a[2][0]);
    std::swap(a[1][2], a[2][1]);
    return *this;
}

Beam Matrix::operator* (const Beam& b) {
//    qDebug() << "dx =" << b.d_x() << "dy =" << b.d_y() << "dz =" << b.d_z();
    qreal dx = b.d_x()*a[0][0] + b.d_y()*a[0][1] + b.d_z()*a[0][2];
    qreal dy = b.d_x()*a[1][0] + b.d_y()*a[1][1] + b.d_z()*a[1][2];
    qreal dz = b.d_x()*a[2][0] + b.d_y()*a[2][1] + b.d_z()*a[2][2];
//    qDebug() << "dx' =" << dx << "dy' =" << dy << "dz' =" << dz;
    return Beam(b.p1(), dx, dy, dz);
}

Tube::Tube(qreal D1, qreal l) : diameter_in(D1), length_(l) {}

qreal Tube::q_a(const Beam& beam) const { return pow(beam.cos_a(), 2) + pow(beam.cos_b(), 2); }

qreal Tube::q_b(const Beam& beam) const { return 2 * (beam.x() * beam.cos_a() + beam.y() * beam.cos_b()); }

qreal Tube::q_c(const Beam& beam) const { return pow(beam.x(), 2) + pow(beam.y(), 2) - pow(r1(), 2); }

qreal Tube::q_d(const Beam& beam) const { return pow(q_b(beam), 2) - 4*q_a(beam)*q_c(beam); }

qreal Tube::q_t(const Beam &beam) const { return (-q_b(beam) + qSqrt(q_d(beam))) / (2*q_a(beam)); }

Cone::Cone(qreal D1, qreal D2, qreal l) : Tube(D1, l),
    diameter_out(D2) {}

qreal Cone::q_a(const Beam& beam) const { return pow(beam.cos_a(), 2) + pow(beam.cos_b(), 2) - pow(beam.cos_g()*tan_phi(), 2); }

qreal Cone::q_b(const Beam& beam) const { return 2 * (beam.x() * beam.cos_a() + beam.y() * beam.cos_b() - (beam.z() - z_k()) * beam.cos_g() * pow(tan_phi(), 2)); }

qreal Cone::q_c(const Beam& beam) const { return pow(beam.x(), 2) + pow(beam.y(), 2) - pow((beam.z() - z_k()) * tan_phi(), 2); }

Beam Beam::unit(const Point& p) {
    return Beam(p, cos_a(), cos_b(), cos_g());
}

void Beam::reflect() {
    dy *= -1;
}

Point Tube::intersection(const Beam &beam) const {
    return Point(beam.x() + q_t(beam)*beam.cos_a(), beam.y() + q_t(beam)*beam.cos_b(), beam.z() + q_t(beam)*beam.cos_g());
}

Point Cone::intersection(const Beam& beam) const {
    Point p = Tube::intersection(beam);
//    qDebug() << qSqrt(p.x()*p.x() + p.y()*p.y()) << ' ' << (z_k() - p.z())*tan_phi();
    bool correct_root = qFabs(qSqrt(p.x()*p.x() + p.y()*p.y()) - (z_k() - p.z())*tan_phi()) < 1e-6;
    if (!correct_root) {
        // False root means that
        // the beam goes outward without intersecting the cone's surface
        // (the intersection point is located on the imaginary side).
        // So the resulting point does not have to belong to the cone's surface
        // But it has to be located outside of the cone so that no false beams appear from it and the calculations stop.
        qreal q_t_corrected = q_t(beam);
        while ((d1() > d2() && beam.z() - q_t_corrected*beam.cos_g() > 0)
               || (d1() < d2() && beam.z() - q_t_corrected*beam.cos_g() < length())) {
            q_t_corrected *= 2;
        }
        p = Point(beam.x() - q_t_corrected*beam.cos_a(), beam.y() - q_t_corrected*beam.cos_b(), beam.z() - q_t_corrected*beam.cos_g());
    }
    return p;
}
