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

Cone::Cone(qreal D1, qreal D2, qreal l) : Tube(D1, l),
    diameter_out(D2) {}

Beam Beam::unit(const Point& p) {
    return Beam(p, cos_a(), cos_b(), cos_g());
}

void Beam::reflect() {
    dy *= -1;
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
        return Point(0, 0, z_k() * (r1() > r2() ? 1 : -1));
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
    qreal t = qFabs(a) > 1e-9 ? (-b + qSqrt(d))/(2*a) : -c/b;
//    qDebug() << a << b << c << d << t;
    Point p = Point(beam.x() + t*beam.cos_a(), beam.y() + t*beam.cos_b(), beam.z() + t*beam.cos_g());
    bool correct_root = qFabs(qSqrt(p.x()*p.x() + p.y()*p.y()) - (z_k() - p.z())*tan_phi()) < 1e-6;
    if (!correct_root) {
        // False root means that the beam goes outward without intersecting the cone's surface
        // (the intersection point is located on the imaginary side).
        // So the resulting point does not have to belong to the cone's surface
        // But it has to be located outside of the cone so that no false beams appear from it and the calculations stop.
        while ((d1() > d2() && beam.z() - t*beam.cos_g() > 0)
               || (d1() < d2() && beam.z() - t*beam.cos_g() < length())) {
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
    Beam meridional = Beam(Point(), beam.d_x(), beam.d_y(), beam.d_z());
    return Beam(beam.p1(), Plane(focus).intersection(meridional));
}
