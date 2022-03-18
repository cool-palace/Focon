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
#include <QDebug>
#include <QResizeEvent>
#include "geometry.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

constexpr qreal diameter = 150;
constexpr qreal margin = 10;
constexpr QPointF y_axis_label_offset = QPointF(-15,-10);
constexpr QPointF x_axis_label_offset = QPointF(-5,-5);
constexpr qreal loss_limit = 10;
constexpr int length_limit = 500;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;

    enum BeamStatus {
        REFLECTED,      // Failed to pass the focon
        MISSED,         // Passed the focon but failed to hit the detector's surface
        HIT,            // Passed the focon and hit the detector's surface
        DETECTED        // Hit within the detector's FOV
    };

    enum Mode {
        SINGLE_BEAM_CALCULATION,
        PARALLEL_BUNDLE,
        DIVERGENT_BUNDLE,
        EXHAUSTIVE_SAMPLING,
        MONTE_CARLO_METHOD,
        LENGTH_OPTIMISATION,
        D_OUT_OPTIMISATION,
        FOCUS_OPTIMISATION,
        FULL_OPTIMISATION,
        COMPLEX_OPTIMISATION
    };

    struct Parameters {
        int length = 0, focus = 0;
        qreal d_out = 0, loss = 1e10;
        Parameters() {}
        Parameters(int length, qreal d_out, qreal loss) : length(length), d_out(d_out), loss(loss) {}
        Parameters(int length, int focus, qreal d_out, qreal loss) : length(length), focus(focus), d_out(d_out), loss(loss) {}
        Parameters(int focus, const Parameters& p) : length(p.length), focus(focus), d_out(p.d_out), loss(p.loss) {}
    };

    // Basic objects
    Point start;
    Point intersection;
    Tube * cone = nullptr;
    Detector detector;
    Lens lens;
    Lens ocular;
    Beam beam;

    // Calculation results
    qreal scale;
    qreal scale_xoy;
    BeamStatus single_beam_status;
    QVector<QGraphicsLineItem *> beams;
    QVector<QGraphicsLineItem *> beams_xoy;
    QVector<Point> points;
    QVector<BeamStatus> statuses;

    // Graphic objects
    QGraphicsScene* scene;
    QGraphicsLineItem * y_axis;
    QGraphicsLineItem * z_axis;
    QGraphicsLineItem * x_axis_xoy;
    QGraphicsLineItem * y_axis_xoy;
    QGraphicsLineItem * focon_up;
    QGraphicsLineItem * focon_down;
    QGraphicsLineItem * detector_yoz;
    QGraphicsLineItem * window_up;
    QGraphicsLineItem * window_down;
    QGraphicsEllipseItem * circle;
    QGraphicsEllipseItem * circle_out;
    QGraphicsLineItem * lens_yoz;
    QGraphicsLineItem * lens_arrow_up_left;
    QGraphicsLineItem * lens_arrow_up_right;
    QGraphicsLineItem * lens_arrow_down_left;
    QGraphicsLineItem * lens_arrow_down_right;
    QGraphicsLineItem * ocular_yoz;
    QGraphicsLineItem * ocular_arrow_up_left;
    QGraphicsLineItem * ocular_arrow_up_right;
    QGraphicsLineItem * ocular_arrow_down_left;
    QGraphicsLineItem * ocular_arrow_down_right;

    QGraphicsTextItem * x_label_xoy;
    QGraphicsTextItem * y_label_xoy;
    QGraphicsTextItem * y_label_yoz;
    QGraphicsTextItem * z_label_yoz;
    QGraphicsTextItem * origin_label_xoy;
    QGraphicsTextItem * origin_label_yoz;

private slots:
    // Interface
    void showEvent(QShowEvent * event) override;
    void resizeEvent(QResizeEvent * event) override;
    void clear();
    void draw(int rotation_angle);
    void draw(const Point& p, BeamStatus status, int rotation_angle);
    void draw_axes(int rotation_angle);
    void draw_axes(qreal theta);
    void set_beam_color(QGraphicsLineItem * beam, BeamStatus status);
    void init_graphics();
    void set_colors(bool night_theme_on);
    void set_text_size(bool big_fonts);
    void set_lens(bool visible);
    void set_ocular(bool visible);
    void rotate(int rotation_angle);
    void show_results(const QPair<int, int>&);
    void show_results(const QPair<int, qreal>&);
    void show_results(const QPair<qreal, qreal>&);
    void show_results(const Parameters&);

    // Filesystem
    void save_settings();
    void load_settings();
    void save_image();
    void save_image_xoy();

    // Calculations
    void init_objects();
    void init_cone(qreal d1, qreal d2, qreal length);
    qreal lens_focus(bool auto_focus);
    void build();
    BeamStatus calculate_single_beam_path();
//    QPair<int, int> calculate_parallel_beams();
    QPair<int, int> calculate_parallel_beams(qreal angle);
    QPair<int, int> calculate_divergent_beams();
    QPair<int, int> calculate_every_beam();
    QPair<int, int> monte_carlo_method();
    QPair<int, qreal> optimal_length();
    QPair<int, qreal> optimal_focus();
    QPair<qreal, qreal> optimal_d_out();
    Parameters full_optimisation();
    Parameters complex_optimisation(); //
    qreal loss(const QPair<int, int>&);

};
#endif // MAINWINDOW_H
