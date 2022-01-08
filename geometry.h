#ifndef GEOMETRY_H
#define GEOMETRY_H
#include <QtGlobal>
#include <QtMath>
#include <QLineF>
#include <QTextStream>

class Point {
private:
    qreal x_, y_, z_;
public:
    Point(qreal x, qreal y, qreal z) : x_(x), y_(y), z_(z) {}
    Point() : x_(0), y_(0), z_(0) {};
    qreal x() const { return x_; }
    qreal y() const { return y_; }
    qreal z() const { return z_; }
    Point x_pair() const { return Point(-x_, y_, z_);};
    bool operator==(const Point& p) { return x() == p.x() && y() == p.y() && z() == p.z(); }
    bool is_in_radius(qreal radius) const { return x()*x() + y()*y() < radius * radius; }
};

class Beam {
private:
    Point p;
    qreal dx, dy, dz;
public:
    Beam() : p(Point()), dx(0), dy(0), dz(0) {};
    Beam(Point p, qreal angle)
        : p(p), dx(0), dy(qSin(qDegreesToRadians(-angle))), dz(qCos(qDegreesToRadians(-angle))) {}
    Beam(Point p, qreal dx, qreal dy, qreal dz) : p(p), dx(dx), dy(dy), dz(dz) {}

    qreal length() const { return sqrt(dx*dx + dy*dy + dz*dz); }

    qreal cos_a() const { return dx/length(); }
    qreal cos_b() const { return dy/length(); }
    qreal cos_g() const { return dz/length(); }

    qreal gamma() const { return qRadiansToDegrees(qAcos(cos_g())); }

    qreal d_x() const { return dx; }
    qreal d_y() const { return dy; }
    qreal d_z() const { return dz; }

    Point p1() const { return p; }
    Point p2() const { return Point(p.y()+dy, p.z()+dz, p.x()+dx); }
    qreal x() const { return p.x(); }
    qreal y() const { return p.y(); }
    qreal z() const { return p.z(); }

    Beam unit(const Point& p);

    void reflect();
};

class Tube {
private:
    qreal diameter_in, length_;

public:
    Tube(qreal D1 = 25, qreal l = 50);
    virtual ~Tube() = default;
    qreal length() const { return length_; }
    qreal d1() const { return diameter_in; }
    qreal r1() const { return diameter_in/2; }
    virtual qreal r2() const { return diameter_in/2; }
    virtual qreal phi() const { return 0; }
    virtual Point intersection(const Beam& beam) const;

};

class Cone : public Tube {
private:
    qreal diameter_out;
    qreal z_k() const { return r1()/tan_phi(); }

public:
    Cone(qreal D1, qreal D2, qreal l);
    ~Cone() = default;

    virtual qreal r2() const override { return diameter_out/2; }
    qreal d2() const {return diameter_out; }
    qreal tan_phi() const  { return (d1() - d2())/(2*length()); }
    virtual qreal phi() const override { return qAtan(tan_phi()); }
    virtual Point intersection(const Beam& beam) const override;
    Point vertex() const { return Point(0, 0, z_k()); }
};

class Matrix {
private:
    qreal a[3][3];
public:
    Matrix(qreal ksi, qreal phi);
    Matrix transponed();
    Beam operator* (const Beam& b);
};

class Detector {
private:
    qreal field_of_view;
    qreal diameter;
    qreal z_position;

public:
    Detector() = default;
    Detector(qreal fov, qreal diameter, qreal z)
        : field_of_view(fov), diameter(diameter), z_position(z) {};
    qreal fov() const { return field_of_view; }
    qreal r() const { return diameter/2; }
    qreal z() const { return z_position; }

    Point intersection(const Beam& beam) const;
    bool hit(const Beam& beam) { return intersection(beam).is_in_radius(r()); }
    bool missed(const Beam& beam) { return !hit(beam); }
    bool detected(const Beam& beam) { return hit(beam) && beam.gamma() < fov(); }
};

#endif // GEOMETRY_H
