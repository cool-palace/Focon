#ifndef LAYOUTS_H
#define LAYOUTS_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QObject>
#include <QVector>
#include <QDial>
#include "geometry.h"

class Layouts : public QWidget
{
    Q_OBJECT
public:
    Layouts(QWidget *parent = nullptr);

    QDoubleSpinBox * d_in;
    QDoubleSpinBox * d_out;
    QDoubleSpinBox * length;
    QDoubleSpinBox * angle;
    QDoubleSpinBox * height;
    QDoubleSpinBox * offset;
    QDial * rotation;
    QPushButton *calc;

    qreal scale;
    qreal scale_xoy;

    QGraphicsScene* scene;
    QGraphicsView* view;
    QGraphicsLineItem * y_axis;
    QGraphicsLineItem * z_axis;
    QGraphicsLineItem * focon_up;
    QGraphicsLineItem * focon_down;
    QGraphicsEllipseItem * circle;
    QGraphicsEllipseItem * circle_out;

    Point start;
    Cone cone;
    Beam beam;
    QVector<QGraphicsLineItem *> beams;
    QVector<QGraphicsLineItem *> beams_xoy;
    QVector<Point> points;
    QGraphicsTextItem * result;

public slots:
    void clear();
    void draw(int rotation_angle);
    void rotate(int rotation_angle);
    void build();

signals:

};

#endif // LAYOUTS_H
