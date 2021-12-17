#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include "geometry.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void rotate(int rotation_angle);
    void build();

private:
    Ui::MainWindow *ui;

    Point start;
    Tube * cone = nullptr;
    Beam beam;
    qreal scale;
    qreal scale_xoy;

    QGraphicsScene* scene;
    QGraphicsLineItem * y_axis;
    QGraphicsLineItem * z_axis;
    QGraphicsLineItem * focon_up;
    QGraphicsLineItem * focon_down;
    QGraphicsEllipseItem * circle;
    QGraphicsEllipseItem * circle_out;

    QVector<QGraphicsLineItem *> beams;
    QVector<QGraphicsLineItem *> beams_xoy;
    QVector<Point> points;

    void clear();
    void draw(int rotation_angle);
    void init();
    void set_outer_circle();
    //    QGraphicsTextItem * result;
};
#endif // MAINWINDOW_H
