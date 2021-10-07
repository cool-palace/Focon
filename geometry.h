#ifndef GEOMETRY_H
#define GEOMETRY_H
#include <QtGlobal>
#include <QtMath>
#include <QLineF>

class Point {
public:
    qreal x, y, z;
    Point(qreal y, qreal z, qreal x = 0) : x(x), y(y), z(z) {}
    Point() : x(0), y(0), z(0) {};
};

class Cone {
private:
    qreal l, tan_phi, z_vertex;
public:
    Cone(qreal D1, qreal D2, qreal l);

    qreal length() const { return l; }
    qreal z_k() const { return z_vertex; }
    qreal r1() const { return z_vertex * tan_phi; }
    qreal r2() const { return (z_vertex - l) * tan_phi; }
    qreal tan_f() const  { return tan_phi; }
    void setCone(qreal D1, qreal D2, qreal l);
};

class Beam {
private:
    Point p;
    //qreal alpha = M_PI_2, beta, gamma; // углы в радианах
    qreal cos_alpha, cos_beta, cos_gamma;
    bool clockwise;
public:
    Beam(Point p, qreal gamma);
    Beam(Point p, qreal alpha, qreal beta, qreal gamma);
    Beam(QLineF beam_yoz);

    void setBeam(Point p, qreal gamma);

//	Beam(qLineF tang, qLineF refl);
//    Beam operator+(const Beam& b1, const Beam& b2);

//	qreal cos_a() { return qCos(alpha); }
//	qreal cos_b() { return qCos(beta); }
//	qreal cos_g() { return qCos(gamma); }

    qreal cos_a() const { return cos_alpha; }
    qreal cos_b() const { return cos_beta; }
    qreal cos_g() const { return cos_gamma; }

    Point origin() const { return p; }
    qreal x() const { return p.x; }
    qreal y() const { return p.y; }
    qreal z() const { return p.z; }

    bool is_clockwise() const { return clockwise; }

    Beam tangPart();
    Beam reflPart();
    Point intersection(Cone * c);
    static qreal transformation_angle(Point p);

    Beam toUnit();
};


#endif // GEOMETRY_H
