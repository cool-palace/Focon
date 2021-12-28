#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QResizeEvent>

constexpr qreal diameter = 150;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new QGraphicsScene())
    , y_axis(new QGraphicsLineItem())
    , z_axis(new QGraphicsLineItem())
    , focon_up(new QGraphicsLineItem())
    , focon_down(new QGraphicsLineItem())
    , circle(new QGraphicsEllipseItem())
    , circle_out(new QGraphicsEllipseItem())

{
    ui->setupUi(this);
    connect(ui->mode, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int mode) {
        switch (mode) {
        case PARALLEL_BUNDLE: case EXHAUSTIVE_SAMPLING: case MONTE_CARLO_METHOD:
            ui->height->setEnabled(false);
            ui->offset->setEnabled(false);
            ui->rotation->setEnabled(mode == PARALLEL_BUNDLE);
            break;
        default:
            ui->height->setEnabled(true);
            ui->offset->setEnabled(true);
            ui->rotation->setEnabled(true);
            break;
        }
    });

    ui->height->setMaximum(ui->d_in->value()/2);
    ui->height->setMinimum(-ui->d_in->value()/2);
    ui->offset->setMaximum(ui->d_in->value()/2);
    ui->offset->setMinimum(-ui->d_in->value()/2);

    connect(ui->d_in, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged), [=](qreal new_diameter) {
        ui->height->setMaximum(new_diameter/2);
        ui->height->setMinimum(-new_diameter/2);
        ui->offset->setMaximum(new_diameter/2);
        ui->offset->setMinimum(-new_diameter/2);
    });

    ui->view->setScene(scene);
    scene->setSceneRect(0,0,ui->view->width(), ui->view->height());
    init();

//    //result = new QGraphicsTextItem();
    y_axis->setPen(QPen(Qt::DashDotLine));
    z_axis->setPen(QPen(Qt::DashDotLine));
    scene->addItem(y_axis);
    scene->addItem(z_axis);
    scene->addItem(focon_up);
    scene->addItem(focon_down);
    scene->addItem(circle);
    scene->addItem(circle_out);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent * event) {
    ui->view->fitInView(ui->view->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    ui->view->fitInView(ui->view->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::init() {
    qreal x_axis_length = scene->width();
    qreal y_axis_length = scene->height();
    qreal x_axis_pos = scene->height()/2;

    // rescaling
    qreal scale_based_on_length = x_axis_length / ui->length->value();
    qreal scale_based_on_diameter = y_axis_length / qMax(ui->d_in->value(), ui->d_out->value());
    scale = qMin(scale_based_on_length, scale_based_on_diameter);
    scale_xoy = diameter / qMax(ui->d_in->value(), ui->d_out->value());

    // updating cone's xoy projection
    qreal diameter_outer = qMin(ui->d_out->value(),ui->d_in->value()) * scale_xoy;
    qreal circle_out_x = scene->width() - diameter/2 - diameter_outer/2;
    qreal circle_out_y = diameter/2 - diameter_outer/2;
    circle_out->setRect(circle_out_x,circle_out_y, diameter_outer, diameter_outer);
    circle->setRect(scene->width()-diameter, 0, diameter, diameter);

    // updating geometrical objects
    start = Point(-ui->offset->value(), -ui->height->value(), 0);
    if (cone != nullptr) delete cone;
    cone = (ui->d_in->value() != ui->d_out->value()
                ? new Cone(ui->d_in->value(), ui->d_out->value(), ui->length->value())
                : new Tube(ui->d_in->value(), ui->length->value()));
    beam = Beam(start, ui->angle->value());

    // updating axes' and cone's yoz projections
    y_axis->setLine(0,0,0,x_axis_pos*2);
    z_axis->setLine(0,x_axis_pos,x_axis_length,x_axis_pos);

    focon_up->setLine(0, x_axis_pos - cone->r1() * scale, ui->length->value() * scale, x_axis_pos - cone->r2() * scale);
    focon_down->setLine(0, x_axis_pos + cone->r1() * scale, ui->length->value() * scale, x_axis_pos + cone->r2() * scale);
}

void MainWindow::clear() {
    for (int i = 0; i < beams.size(); ++i) {
        delete beams[i];
    }
    beams.clear();

    for (int i = 0; i < beams_xoy.size(); ++i) {
        delete beams_xoy[i];
    }
    beams_xoy.clear();
}

void MainWindow::draw(int rotation_angle) {
    qreal theta = qDegreesToRadians(static_cast<qreal>(rotation_angle));
    for (int i = 0; i < points.size()-1; ++i) {        
        QLineF line = QLineF(points[i].z() * scale, -(-points[i].y()*qCos(theta) + points[i].x()*qSin(theta)) * scale + scene->height()/2,
                             points[i+1].z()* scale, -(-points[i+1].y()*qCos(theta) + points[i+1].x()*qSin(theta))* scale + scene->height()/2);
        beams.push_back(new QGraphicsLineItem(line));
        scene->addItem(beams.back());

        QLineF line_xoy = QLineF(-(points[i].x()*qCos(theta) + points[i].y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                 -(points[i].x()*qSin(theta) - points[i].y()*qCos(theta))*scale_xoy + diameter/2,
                                 -(points[i+1].x()*qCos(theta) + points[i+1].y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                 -(points[i+1].x()*qSin(theta) - points[i+1].y()*qCos(theta))*scale_xoy + diameter/2);
        beams_xoy.push_back(new QGraphicsLineItem(line_xoy));
        scene->addItem(beams_xoy.back());

        if (points.back().z() > cone->length()) {
            beams[i]->setPen(QPen(Qt::green));
            beams_xoy[i]->setPen(QPen(Qt::green));
        } else {
            beams[i]->setPen(QPen(Qt::red));
            beams_xoy[i]->setPen(QPen(Qt::red));
        }
    }
}

void MainWindow::draw(const Point& point, bool has_passed, int rotation_angle) {
    qreal theta = qDegreesToRadians(static_cast<qreal>(rotation_angle));

    switch (ui->mode->currentIndex()) {
        case PARALLEL_BUNDLE: {
            QLineF line_xoy = QLineF(-(point.x()*qCos(theta) + point.y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                     -(point.x()*qSin(theta) - point.y()*qCos(theta))*scale_xoy + diameter/2,
                                     -(point.x()*qCos(theta) + point.y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                     -(point.x()*qSin(theta) - point.y()*qCos(theta))*scale_xoy + diameter/2);
            beams_xoy.push_back(new QGraphicsLineItem(line_xoy));
            scene->addItem(beams_xoy.back());

            if (has_passed) {
                beams_xoy.back()->setPen(QPen(Qt::green));
            } else {
                beams_xoy.back()->setPen(QPen(Qt::red));
            }
        } break;
        case DIVERGENT_BUNDLE: {
            QLineF line = QLineF(start.z() * scale, -(-start.y()*qCos(theta) + start.x()*qSin(theta)) * scale + scene->height()/2,
                                 point.z()* scale, -(-point.y()*qCos(theta) + point.x()*qSin(theta))* scale + scene->height()/2);
            beams.push_back(new QGraphicsLineItem(line));
            scene->addItem(beams.back());

            if (has_passed) {
                beams.back()->setPen(QPen(Qt::green));
            } else {
                beams.back()->setPen(QPen(Qt::red));
            }
        } break;
    }

}

void MainWindow::rotate(int rotation_angle) {
//    if (ui->mode->currentIndex() != SINGLE_BEAM_CALCULATION) return;
    clear();
    switch (ui->mode->currentIndex()) {
        case SINGLE_BEAM_CALCULATION:
            draw(rotation_angle);
            break;
        case PARALLEL_BUNDLE:
            for (int i = 0; i < points.size(); ++i) {
                draw(points[i], beam_has_passed[i], rotation_angle);
                if (points[i].x() != 0) {
                    draw(points[i].x_pair(), beam_has_passed[i], rotation_angle);
                }
            }
            break;
        case DIVERGENT_BUNDLE:
            for (int i = 0; i < points.size(); ++i) {
                draw(points[i], beam_has_passed[i], rotation_angle);
            }
            break;
        }
}

void MainWindow::build() {
    clear();
    points.clear();
    beam_has_passed.clear();
    init();

    switch (ui->mode->currentIndex()) {
    case SINGLE_BEAM_CALCULATION:
        calculate_single_beam_path();
        draw(ui->rotation->value());
        if (points.size() > 1) {
            ui->statusbar->showMessage("Количество отражений: " + QString().setNum(points.size()-2));
        }
        break;
    case PARALLEL_BUNDLE:
        calculate_parallel_beams();
        break;
    case DIVERGENT_BUNDLE:
        calculate_divergent_beams();
        break;
    case EXHAUSTIVE_SAMPLING:
        calculate_every_beam();
        break;
    case MONTE_CARLO_METHOD:
        monte_carlo_method();
        break;
    default:
        break;
    }

}

bool MainWindow::calculate_single_beam_path() {
    points.push_back(start);
    do {
        // Perpendicular beams cause infinite loop in tubes
        if (cone->r1() == cone->r2() && fabs(beam.d_y()) == 1) {
            ui->statusbar->showMessage("Некорректный входной угол");
            return false;
        }

        intersection = cone->intersection(beam);
//      qDebug() << "(пересечение " << points.size()  << ")" << i_point.x() << ' ' << i_point.y() << ' ' << i_point.z();

        switch (ui->mode->currentIndex()) {
        case SINGLE_BEAM_CALCULATION:
            // Points array forms complete beam path
            points.push_back(intersection);
            break;
        case PARALLEL_BUNDLE:
            // Points array contains possible entry points
            break;
        case DIVERGENT_BUNDLE:
            // Points array contains first intersection points
            if (points.back() == start) {
                points.back() = intersection;
            }
            break;
        }

        QLineF line = QLineF(0, 0, intersection.x(), intersection.y());
//      qDebug() << line.angle();
        qreal ksi = qDegreesToRadians(-90 + line.angle());
        qreal phi = cone->phi();
//      qDebug() << "углы " << qRadiansToDegrees(ksi) << ' ' << qRadiansToDegrees(phi);
        Matrix m = Matrix(ksi, phi);

        Beam transformed_beam = m*beam.unit(intersection);
        transformed_beam.reflect();
        beam = m.transponed()*transformed_beam;
    } while (intersection.z() > 0 && intersection.z() < cone->length());

    return intersection.z() > cone->length() || cone->r1() < cone->r2();
}

void MainWindow::calculate_parallel_beams() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = 20;
    for (int i = 0; i < count; ++i) {
        qreal x = i * cone->r1() / count;
        for (int j = -count; j < count; ++j) {
            qreal y = j * cone->r1() / count;
            if (x*x + y*y < cone->r1()*cone->r1()) {
                start = Point(-x, -y, 0);
                beam = Beam(start, ui->angle->value());
                // The results are simmetrical relative to y axis, hence doubling total count for i > 0
                beams_total += (i > 0 ? 2 : 1);
                bool has_passed = calculate_single_beam_path();
                beam_has_passed.push_back(has_passed);
                if (has_passed) {
                    beams_passed += (i > 0 ? 2 : 1);
                }
                draw(points.back(), has_passed, ui->rotation->value());
                if (x > 0) {
                    draw(points.back().x_pair(), has_passed, ui->rotation->value());
                }
            }
        }
    }
    ui->statusbar->showMessage("Прошло " + QString().setNum(beams_passed) + " лучей из " + QString().setNum(beams_total)
                               + ". Потери составляют " + QString().setNum(10*qLn(static_cast<qreal>(beams_total)/beams_passed)/qLn(10)) + " дБ.");
}

QPair<int, int> MainWindow::calculate_divergent_beams() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = 4;
    int limit = abs(ui->angle->value()) * count;
    for (int i = -limit; i <= limit; ++i) {
        qreal x = static_cast<qreal>(i) / count;
        beam = Beam(start, x);
        ++beams_total;
        bool has_passed = calculate_single_beam_path();
        beam_has_passed.push_back(has_passed);
        if (has_passed) {
            ++beams_passed;
        }
        draw(points.back(), has_passed, ui->rotation->value());
    }
    ui->statusbar->showMessage("Прошло " + QString().setNum(beams_passed) + " лучей из " + QString().setNum(beams_total)
                               + ". Потери составляют " + QString().setNum(10*qLn(static_cast<qreal>(beams_total)/beams_passed)/qLn(10)) + " дБ.");
    return qMakePair(beams_passed, beams_total);
}

void MainWindow::calculate_every_beam() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = 20;
    for (int i = 0; i < count; ++i) {
        qreal x = i * cone->r1() / count;
        for (int j = -count; j < count; ++j) {
            qreal y = j * cone->r1() / count;
            if (x*x + y*y < cone->r1()*cone->r1()) {
                start = Point(-x, -y, 0);
                QPair<int, int> current_result = calculate_divergent_beams();
                // The results are simmetrical relative to y axis, hence doubling total count for i > 0
                beams_passed += (i > 0 ? 2 : 1) * current_result.first;
                beams_total += (i > 0 ? 2 : 1) * current_result.second;
            }
        }
    }
    ui->statusbar->showMessage("Прошло " + QString().setNum(beams_passed) + " лучей из " + QString().setNum(beams_total)
                               + ". Потери составляют " + QString().setNum(10*qLn(static_cast<qreal>(beams_total)/beams_passed)/qLn(10)) + " дБ.");
}

void MainWindow::monte_carlo_method() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = 100000;
    QRandomGenerator rng;
    for (int i = 0; i < count; ++i) {
        qreal x = 2 * rng.generateDouble() - 1;
        qreal y = 2 * rng.generateDouble() - 1;
        start = Point(x * cone->r1(), y * cone->r1(), 0);
        if (x*x + y*y < cone->r1()*cone->r1()) {
            beam = Beam(start, (2 * rng.generateDouble() - 1) * fabs(ui->angle->value()));
            ++beams_total;
            bool has_passed = calculate_single_beam_path();
            if (has_passed) {
                ++beams_passed;
            }
        } else --i;
    }
    ui->statusbar->showMessage("Прошло " + QString().setNum(beams_passed) + " лучей из " + QString().setNum(beams_total)
                               + ". Потери составляют " + QString().setNum(10*qLn(static_cast<qreal>(beams_total)/beams_passed)/qLn(10)) + " дБ.");
}
