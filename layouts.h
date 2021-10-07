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
    QPushButton *calc;

    qreal scale;

    QGraphicsScene* scene;
    QGraphicsView* view;
    QGraphicsLineItem * y_axis;
    QGraphicsLineItem * z_axis;
    QGraphicsLineItem * focon_up;
    QGraphicsLineItem * focon_down;

    Point start;
    Cone * cone;
    Beam * beam;
    QVector<QGraphicsLineItem *> beams;
    QGraphicsTextItem * result;

public slots:
    void draw();

signals:

};

#endif // LAYOUTS_H
