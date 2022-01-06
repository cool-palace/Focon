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
#include <QRandomGenerator>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>

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

private slots:
    void save_settings();
    void load_settings();

private:
    Ui::MainWindow *ui;

    Point start;
    Point intersection;
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
    QVector<bool> beam_has_passed;

    enum Mode {
        SINGLE_BEAM_CALCULATION,
        PARALLEL_BUNDLE,
        DIVERGENT_BUNDLE,
        EXHAUSTIVE_SAMPLING,
        MONTE_CARLO_METHOD
    };

    void showEvent(QShowEvent * event) override;
    void resizeEvent(QResizeEvent * event) override;
    void clear();
    void draw(int rotation_angle);
    void draw(const Point& p, bool has_passed, int rotation_angle);
    void init();
    bool calculate_single_beam_path();
    void calculate_parallel_beams();
    QPair<int, int> calculate_divergent_beams();
    void calculate_every_beam();
    void monte_carlo_method();
    void show_results(int, int);
    //    QGraphicsTextItem * result;
};
#endif // MAINWINDOW_H
