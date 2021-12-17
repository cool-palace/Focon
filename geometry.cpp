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

Cone::Cone() : l(1), tan_f(1), z_vertex(1) {}

Cone::Cone(qreal D1, qreal D2, qreal l) :
    l(l), tan_f((D1-D2)/(2*l)), z_vertex(D1*l/(D1-D2)) {}

Beam Beam::unit(const Point& p) {
    return Beam(p, cos_a(), cos_b(), cos_g());
}

void Beam::reflect() {
    dy *= -1;
}

Point Beam::intersection(const Cone& cone) {
    qreal a = pow(cos_a(), 2) + pow(cos_b(), 2) - pow(cos_g()*cone.tan_phi(), 2);
    qreal b = 2 * (x() * cos_a() + y() * cos_b() - (z() - cone.z_k()) * cos_g() * pow(cone.tan_phi(), 2));
    qreal c = pow(x(), 2) + pow(y(), 2) - pow((z() - cone.z_k()) * cone.tan_phi(), 2);
    qreal d = pow(b, 2) - 4*a*c;
    qreal t1 = (-b + qSqrt(d)) / (2*a);
    Point p1 = {x() + t1*cos_a(), y() + t1*cos_b(), z() + t1*cos_g()};
//    qDebug() << qSqrt(p1.x()*p1.x() + p1.y()*p1.y()) << ' ' << (cone.z_k() - p1.z())*cone.tan_phi();
    // The following condition fixes the bug when the beam goes outward
    // if its direction becomes almost parallel to the cone's inner surface
    bool correct_root = qFabs(qSqrt(p1.x()*p1.x() + p1.y()*p1.y()) - (cone.z_k() - p1.z())*cone.tan_phi()) < 1e-6;
    if (!correct_root) {
        p1 = {x() - t1*cos_a(), y() - t1*cos_b(), z() - t1*cos_g()};
    }
    return p1;
}

