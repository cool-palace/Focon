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

    ui->view->setScene(scene);
    scene->setSceneRect(0,0,ui->view->width(), ui->view->height());
    init();

//    //result = new QGraphicsTextItem();

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

    // updating spinboxes
    ui->height->setMaximum(ui->d_in->value()/2);
    ui->height->setMinimum(-ui->d_in->value()/2);
    ui->offset->setMaximum(ui->d_in->value()/2);
    ui->offset->setMinimum(-ui->d_in->value()/2);

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
    for (int i = 0; i < points.size()-1; ++i) {
        qreal theta = qDegreesToRadians(static_cast<qreal>(rotation_angle));
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
}

void MainWindow::rotate(int rotation_angle) {
    if (ui->mode->currentIndex() != SINGLE_BEAM_CALCULATION) return;
    clear();
    draw(rotation_angle);
}

void MainWindow::build() {
    clear();
    points.clear();
    init();

    switch (ui->mode->currentIndex()) {
    case SINGLE_BEAM_CALCULATION:
        calculate_single_beam_path();
        draw(ui->rotation->value());
        if (points.size() > 1) {
            ui->statusbar->showMessage("Количество отражений: " + QString().setNum(points.size()-2));
        }
        break;
    case SAMPLING_WITH_GIVEN_ANGLE:
        calculate_beams_with_given_angle();
        break;
    default:
        break;
    }

}

void MainWindow::calculate_single_beam_path() {
    points.push_back(start);
    do {
        if (cone->r1() == cone->r2() && fabs(ui->angle->value()) == 90) {
            ui->statusbar->showMessage("Некорректный входной угол");
            break;
        }
        intersection = cone->intersection(beam);
//      qDebug() << "(пересечение " << points.size()  << ")" << i_point.x() << ' ' << i_point.y() << ' ' << i_point.z();

        if (ui->mode->currentIndex() == SINGLE_BEAM_CALCULATION) {
            // Points array forms complete beam path
            points.push_back(intersection);
        } else if (ui->mode->currentIndex() == SAMPLING_WITH_GIVEN_ANGLE) {
            // Points array consists possible entry points
//            points[points.size()-1] = intersection;
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


}

void MainWindow::calculate_beams_with_given_angle() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = 20;
    bool need_to_calculate_beams_in_row = true;
//    int iterations = 0;
    for (int i = 0; i < count; ++i) {
        qreal x = i * cone->r1() / count;
        for (int j = -count; j < count; ++j) {
            qreal y = j * cone->r1() / count;
            if (x*x + y*y < cone->r1()*cone->r1()) {
                start = Point(-x, -y, 0);
                // The sign of the angle value doesn't affect results
                // Using its absolute value allows to optimize the calculations
                beam = Beam(start, fabs(ui->angle->value()));
                // The results are simmetrical relative to y axis, hence doubling total count for i > 0
                beams_total += (i > 0 ? 2 : 1);
//                ++iterations;
                if (need_to_calculate_beams_in_row) {
                    calculate_single_beam_path();
                    bool has_passed = false;
                    // If the current beam fails to pass then it's safe to assume
                    // that calculating any higher beams is useless for current i value
                    // При нулевых углах плохо работает, либо исправить, либо забить
//                    need_to_calculate_beams_in_row = points.back().z() > 0;
                    if (intersection.z() > 0) {
                        has_passed = true;
                        beams_passed += (i > 0 ? 2 : 1);
                    }
                    draw(points.back(), has_passed, ui->rotation->value());
                    if (x > 0) draw(points.back().x_pair(), has_passed, ui->rotation->value());
                }
            }
        }
        need_to_calculate_beams_in_row = true;
    }
    ui->statusbar->showMessage("Прошло " + QString().setNum(beams_passed) + " лучей из " + QString().setNum(beams_total)
                               + ". Потери составляют " + QString().setNum(10*qLn(static_cast<qreal>(beams_total)/beams_passed)/qLn(10)) + " дБ.");
//                               + ". Проведено " + QString().setNum(iterations) + " итераций.");

}
