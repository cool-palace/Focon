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
    void save_image();

private:
    Ui::MainWindow *ui;

    enum BeamStatus {
        REFLECTED,      // Failed to pass the focon
        MISSED,         // Passed the focon but failed to hit the detector's surface
        HIT,            // Passed the focon and hit the detector's surface
        DETECTED        // Hit within the detector's FOV
    };

    Point start;
    Point intersection;
    Tube * cone = nullptr;
    Detector detector;
    Beam beam;
    BeamStatus single_beam_status;
    qreal scale;
    qreal scale_xoy;

    QGraphicsScene* scene;
    QGraphicsLineItem * y_axis;
    QGraphicsLineItem * z_axis;
    QGraphicsLineItem * x_axis_xoy;
    QGraphicsLineItem * y_axis_xoy;
    QGraphicsLineItem * focon_up;
    QGraphicsLineItem * focon_down;
    QGraphicsLineItem * detector_yoz;
    QGraphicsEllipseItem * circle;
    QGraphicsEllipseItem * circle_out;

    QGraphicsSimpleTextItem * x_label_xoy;
    QGraphicsSimpleTextItem * y_label_xoy;
    QGraphicsSimpleTextItem * y_label_yoz;
    QGraphicsSimpleTextItem * z_label_yoz;
    QGraphicsSimpleTextItem * origin_label_xoy;
    QGraphicsSimpleTextItem * origin_label_yoz;

    QVector<QGraphicsLineItem *> beams;
    QVector<QGraphicsLineItem *> beams_xoy;
    QVector<Point> points;
    QVector<BeamStatus> statuses;

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
    void draw(const Point& p, BeamStatus status, int rotation_angle);
    void draw_axes(int rotation_angle);
    void draw_axes(qreal theta);
    void set_beam_color(QGraphicsLineItem * beam, BeamStatus status);
    void init();
    BeamStatus calculate_single_beam_path();
    void calculate_parallel_beams();
    QPair<int, int> calculate_divergent_beams();
    void calculate_every_beam();
    void monte_carlo_method();
    void show_results(int, int);
    //    QGraphicsTextItem * result;
};
#endif // MAINWINDOW_H
