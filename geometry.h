#ifndef GEOMETRY_H
#define GEOMETRY_H
#include <QtGlobal>
#include <QtMath>
#include <QLineF>
#include <QTextStream>

class bad_intersection : std::exception {
    const char* what() const noexcept override { return "Beam calculation error"; }
};

class Point {
private:
    qreal x_, y_, z_;

public:
    Point(qreal x, qreal y, qreal z) : x_(x), y_(y), z_(z) {}
    Point() : x_(0), y_(0), z_(0) {}
    qreal x() const { return x_; }
    qreal y() const { return y_; }
    qreal z() const { return z_; }
    Point x_pair() const { return Point(-x_, y_, z_); }
    bool operator==(const Point& p) { return x() == p.x() && y() == p.y() && z() == p.z(); }
    bool operator!=(const Point& p) { return !(*this == p); }
    qreal r_sqr() const { return x()*x() + y()*y(); }
    qreal r() const { return qSqrt(r_sqr()); }
    bool is_in_radius(qreal radius) const { return r_sqr() + 1e-6 < radius * radius; }
    friend QDebug& operator<<(QDebug debug, const Point& p);
};

class Vector {
private:
    qreal dx, dy, dz;

public:
    Vector() : dx(0), dy(0), dz(1) {}
    Vector(qreal dx, qreal dy, qreal dz);
    Vector(const Point& p1, const Point& p2);
    Vector(qreal angle) : dx(0), dy(qSin(qDegreesToRadians(-angle))), dz(qCos(qDegreesToRadians(-angle))) {}
    qreal d_x() const { return dx; }
    qreal d_y() const { return dy; }
    qreal d_z() const { return dz; }
    qreal gamma() const { return qRadiansToDegrees(qAcos(dz)); }
    qreal length() const  { return 1; }
    void reflect() { dy *= -1; }
};

class Beam {
private:
    Point p;
    Vector v;

public:
    Beam() : p(Point()), v(Vector()) {}
    Beam(const Point& p, const Vector& v) : p(p), v(v) {}
    Beam(const Point& p, qreal angle) : p(p), v(Vector(angle)) {}
    Beam(const Point& p, qreal dx, qreal dy, qreal dz) : p(p), v(Vector(dx,dy,dz)) {}
    Beam(const Point& p1, const Point& p2) : p(p1), v(p1, p2) {}
    qreal length() const { return v.length(); }
    qreal cos_a() const { return v.d_x(); }
    qreal cos_b() const { return v.d_y(); }
    qreal cos_g() const { return v.d_z(); }
    qreal gamma() const { return v.gamma(); }
    qreal d_x() const { return v.d_x(); }
    qreal d_y() const { return v.d_y(); }
    qreal d_z() const { return v.d_z(); }
    Point p1() const { return p; }
    qreal x() const { return p.x(); }
    qreal y() const { return p.y(); }
    qreal z() const { return p.z(); }
    void reflect() { v.reflect(); }
    Beam on_point(const Point& point) const { return Beam(point, v); }
    friend QDebug& operator<<(QDebug debug, const Beam& m);
};

class Plane {
private:
    qreal z_;

public:
    Plane(qreal z = 0) : z_(z) {}
    qreal z() const { return z_; }
    Point intersection(const Beam& beam) const;
    Beam refracted(const Beam& beam, qreal n1, qreal n2) const;
};

class Tube {
private:
    qreal diameter_in, length_;
    qreal refraction_index;

public:
    Tube(qreal D1, qreal l, qreal n = 1) : diameter_in(D1), length_(l), refraction_index(n) {}
    virtual ~Tube() = default;
    qreal length() const { return length_; }
    qreal d1() const { return diameter_in; }
    virtual qreal d2() const { return diameter_in; }
    qreal r1() const { return diameter_in/2; }
    virtual qreal r2() const { return diameter_in/2; }
    virtual qreal phi() const { return 0; }
    qreal n() const { return refraction_index; }
    Plane entrance() const { return Plane(0); }
    Plane exit() const { return Plane(length()); }
    virtual Point intersection(const Beam& beam) const;
    virtual bool is_conic();
    void set_d1(qreal d1) { diameter_in = d1; }
    virtual void set_d2(qreal d2) { /* do nothing */ }
    void set_length(qreal new_length) { length_ = new_length; }
    void set_n(qreal n) { refraction_index = n; }
    Beam reflected(const Beam& beam, const Point& intersection) const;
    Beam refracted(const Beam& beam) const;
};

class Cone : public Tube {
private:
    qreal diameter_out;
    qreal z_offset;

public:
    Cone(qreal D1, qreal D2, qreal l, qreal z = 0, qreal n = 1) : Tube(D1, l, n), diameter_out(D2), z_offset(z) {}
    ~Cone() override = default;
    qreal r2() const override { return diameter_out/2; }
    qreal d2() const override { return diameter_out; }
    qreal tan_phi() const  { return (d1() - d2())/(2*length()); }
    qreal phi() const override { return qAtan(tan_phi()); }
    Point intersection(const Beam& beam) const override;
    qreal z_k() const { return z_offset + r1()/tan_phi(); }
    Point vertex() const { return Point(0, 0, z_k()); }
    void set_d2(qreal d2) override { diameter_out = d2; }
    void set_z(qreal z) { z_offset = z; }
};

class Matrix {
private:
    qreal a[3][3];

public:
    Matrix(qreal theta);
    Matrix(qreal ksi, qreal phi);
    Matrix(qreal ksi, qreal phi, qreal theta);
    Matrix transponed();
    Beam operator* (const Beam& b);
    friend QDebug& operator<<(QDebug debug, const Matrix& m);
};

class Detector {
private:
    qreal aperture, z_pos, z_offset;
    qreal field_of_view, diameter;

public:
    Detector() = default;
    Detector(qreal aperture, qreal z_pos, qreal z_offset, qreal fov, qreal diameter)
        : aperture(aperture), z_pos(z_pos), z_offset(z_offset), field_of_view(fov), diameter(diameter) {}
    qreal fov() const { return field_of_view; }
    qreal r() const { return diameter/2; }
    qreal window_radius() const { return aperture/2; }
    qreal window_diameter() const { return aperture; }
    qreal window_z() const { return z_pos; }
    qreal detector_z() const { return z_pos + z_offset; }
    Plane plane() const { return Plane(detector_z()); }
    Point intersection(const Beam& beam, qreal z) const;
    bool hit(const Beam& beam) { return intersection(beam,window_z()).is_in_radius(window_radius())
                                        && intersection(beam,detector_z()).is_in_radius(r()); }
    bool missed(const Beam& beam) { return !hit(beam); }
    bool detected(const Beam& beam) { return hit(beam) && qFabs(beam.gamma()) < fov(); }
    void set_position(qreal z) { z_pos = z; }
};

class Lens {
private:
    qreal focus, z_pos;

public:
    Lens() = default;
    Lens(qreal f, qreal z_pos = 0) : focus(f), z_pos(z_pos) {}
    qreal F() const { return 1.0/focus; }
    qreal z() const { return z_pos; }
    void set_focus(qreal f) { focus = f; }
    void set_position(qreal z) { z_pos = z; }
    Point center() const { return Point(0, 0, z_pos); }
    Plane focal_plane() const { return Plane(z_pos + focus); }
    Beam refracted(const Beam &beam) const;
};

#endif // GEOMETRY_H
