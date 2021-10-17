#ifndef GEOMETRY_H
#define GEOMETRY_H
#include <QtGlobal>
#include <QtMath>
#include <QLineF>
#include <QTextStream>

class Point {
public:
    qreal x_, y_, z_;
    Point(qreal x, qreal y, qreal z) : x_(x), y_(y), z_(z) {}
    Point() : x_(0), y_(0), z_(0) {};

    qreal x() const { return x_; }
    qreal y() const { return y_; }
    qreal z() const { return z_; }
};

class Cone {
private:
    qreal l, tan_f, z_vertex;
public:
    Cone();
    Cone(qreal D1, qreal D2, qreal l);

    qreal length() const { return l; }
    qreal z_k() const { return z_vertex; }
    qreal r1() const { return z_vertex * tan_f; }
    qreal r2() const { return (z_vertex - l) * tan_f; }
    qreal tan_phi() const  { return tan_f; }
    qreal phi() const { return qAtan(tan_f); }

    //void setCone(qreal D1, qreal D2, qreal l);
};

class Beam {
private:
    Point p;
    qreal dx, dy, dz;
public:
    Beam() : p(Point()), dx(0), dy(0), dz(0) {};
    Beam(Point p, qreal angle) : p(p), dx(0), dy(qSin(qDegreesToRadians(-angle))), dz(qCos(qDegreesToRadians(-angle))) {}
    Beam(Point p, qreal dx, qreal dy, qreal dz) : p(p), dx(dx), dy(dy), dz(dz) {}

    qreal length() const { return sqrt(dx*dx + dy*dy + dz*dz); }

    qreal cos_a() const { return dx/length(); }
    qreal cos_b() const { return dy/length(); }
    qreal cos_g() const { return dz/length(); }

    qreal d_x() const { return dx; }
    qreal d_y() const { return dy; }
    qreal d_z() const { return dz; }

    Point p1() const { return p; }
    Point p2() const { return Point(p.y()+dy, p.z()+dz, p.x()+dx); }
    qreal x() const { return p.x(); }
    qreal y() const { return p.y(); }
    qreal z() const { return p.z(); }

    Point intersection(const Cone& c);

    Beam unit(Point p);

    void reflect();
    //void setBeam(Point p, qreal dx, qreal dy, qreal dz);
};

class Matrix {
private:
    qreal a[3][3];
public:
    Matrix(qreal ksi, qreal phi);

    Matrix transponed();

    Beam operator* (const Beam& b);
};

#endif // GEOMETRY_H
