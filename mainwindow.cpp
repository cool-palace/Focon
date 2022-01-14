#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QResizeEvent>

constexpr qreal diameter = 150;
constexpr qreal margin = 10;
constexpr QPointF y_axis_label_offset = QPointF(-10,-5);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new QGraphicsScene())
    , y_axis(new QGraphicsLineItem())
    , z_axis(new QGraphicsLineItem())
    , x_axis_xoy(new QGraphicsLineItem())
    , y_axis_xoy(new QGraphicsLineItem())
    , focon_up(new QGraphicsLineItem())
    , focon_down(new QGraphicsLineItem())
    , detector_yoz(new QGraphicsLineItem())
    , window_up(new QGraphicsLineItem())
    , window_down(new QGraphicsLineItem())
    , circle(new QGraphicsEllipseItem())
    , circle_out(new QGraphicsEllipseItem())
    , x_label_xoy(new QGraphicsSimpleTextItem("x"))
    , y_label_xoy(new QGraphicsSimpleTextItem("y"))
    , y_label_yoz(new QGraphicsSimpleTextItem("y"))
    , z_label_yoz(new QGraphicsSimpleTextItem("z"))
    , origin_label_xoy(new QGraphicsSimpleTextItem("0"))
    , origin_label_yoz(new QGraphicsSimpleTextItem("0"))

{
    ui->setupUi(this);

    connect(ui->load, SIGNAL(triggered(bool)), this, SLOT(load_settings()));
    connect(ui->save, SIGNAL(triggered(bool)), this, SLOT(save_settings()));
    connect(ui->save_image, SIGNAL(triggered(bool)), this, SLOT(save_image()));

    connect(ui->mode, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int mode) {
        clear();
        // Rotation should be disabled until recalculation in new mode
        ui->rotation->setEnabled(false);

        switch (mode) {
        case PARALLEL_BUNDLE: case EXHAUSTIVE_SAMPLING: case MONTE_CARLO_METHOD:
            ui->height->setEnabled(false);
            ui->offset->setEnabled(false);
            break;
        default:
            ui->height->setEnabled(true);
            ui->offset->setEnabled(true);
            break;
        }
    });

    ui->height->setMaximum(ui->d_in->value()/2);
    ui->height->setMinimum(-ui->d_in->value()/2);
    ui->offset->setMaximum(ui->d_in->value()/2);
    ui->offset->setMinimum(-ui->d_in->value()/2);

    connect(ui->d_in, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged), [&](qreal new_diameter) {
        ui->height->setMaximum(new_diameter/2);
        ui->height->setMinimum(-new_diameter/2);
        ui->offset->setMaximum(new_diameter/2);
        ui->offset->setMinimum(-new_diameter/2);
    });

    ui->view->setScene(scene);
    scene->setSceneRect(0,0,ui->view->width()-margin, ui->view->height());
    init();

    qreal x_axis_pos = scene->height()/2;

    // setting axes' projections
    y_axis->setLine(0, 0, 0, x_axis_pos*2);
    x_axis_xoy->setLine(circle->rect().x() - margin, diameter/2 + margin, circle->rect().x() + diameter + margin, diameter/2 + margin);
    y_axis_xoy->setLine(circle->rect().x()+diameter/2, 0, circle->rect().x() + diameter/2, diameter + 2*margin);
    x_axis_xoy->setTransformOriginPoint(x_axis_xoy->line().center());
    y_axis_xoy->setTransformOriginPoint(y_axis_xoy->line().center());

    // setting axes' labels
    x_label_xoy->setPos(x_axis_xoy->line().p2());
    y_label_xoy->setPos(y_axis_xoy->line().p1() + y_axis_label_offset);
    y_label_yoz->setPos(y_axis->line().p1() + y_axis_label_offset);
    z_label_yoz->setPos(z_axis->line().p2());
    x_label_xoy->setTransformOriginPoint(x_axis_xoy->line().center() - x_label_xoy->pos());
    y_label_xoy->setTransformOriginPoint(y_axis_xoy->line().center() - y_label_xoy->pos());

    // setting font
    auto font = QFont("Calibri",10);
    x_label_xoy->setFont(font);
    y_label_xoy->setFont(font);
    y_label_yoz->setFont(font);
    z_label_yoz->setFont(font);
    origin_label_xoy->setFont(font);
    origin_label_yoz->setFont(font);

    y_axis->setPen(QPen(Qt::DashDotLine));
    z_axis->setPen(QPen(Qt::DashDotLine));
    x_axis_xoy->setPen(QPen(Qt::DashDotLine));
    y_axis_xoy->setPen(QPen(Qt::DashDotLine));
    scene->addItem(y_axis);
    scene->addItem(z_axis);
    scene->addItem(x_axis_xoy);
    scene->addItem(y_axis_xoy);
    scene->addItem(focon_up);
    scene->addItem(focon_down);
    scene->addItem(window_up);
    scene->addItem(window_down);
    scene->addItem(detector_yoz);
    scene->addItem(circle);
    scene->addItem(circle_out);
    scene->addItem(x_label_xoy);
    scene->addItem(y_label_xoy);
    scene->addItem(y_label_yoz);
    scene->addItem(z_label_yoz);
    scene->addItem(origin_label_xoy);
    scene->addItem(origin_label_yoz);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent *event) {
    Q_UNUSED(event);
    ui->view->fitInView(ui->view->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    ui->view->fitInView(ui->view->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::init() {
    qreal x_axis_length = scene->width();
    qreal y_axis_length = scene->height();
    qreal x_axis_pos = scene->height()/2;

    // rescaling
    qreal scale_based_on_length = x_axis_length / (ui->length->value() + ui->offset_det->value());
    qreal scale_based_on_diameter = y_axis_length / qMax(ui->d_in->value(), ui->d_out->value());
    scale = qMin(scale_based_on_length, scale_based_on_diameter);
    scale_xoy = diameter / qMax(ui->d_in->value(), ui->d_out->value());

    // updating cone's xoy projection
    qreal diameter_outer = qMin(ui->d_out->value(),ui->d_in->value()) * scale_xoy;
    qreal circle_out_x = scene->width() - diameter/2 - diameter_outer/2;
    qreal circle_out_y = diameter/2 - diameter_outer/2;
    circle_out->setRect(circle_out_x, circle_out_y + margin, diameter_outer, diameter_outer);
    circle->setRect(scene->width()-diameter, margin, diameter, diameter);

    // updating geometrical objects and detector
    start = Point(-ui->offset->value(), -ui->height->value(), 0);
    if (cone != nullptr) delete cone;
    cone = (ui->d_in->value() != ui->d_out->value()
                ? new Cone(ui->d_in->value(), ui->d_out->value(), ui->length->value())
                : new Tube(ui->d_in->value(), ui->length->value()));
    beam = Beam(start, ui->angle->value());
    detector = Detector(ui->aperture->value(), ui->length->value(), ui->offset_det->value(),
                        ui->fov->value(), ui->d_det->value());

    // updating z axis' projection and origin points
    z_axis->setLine(-margin, x_axis_pos, x_axis_length + margin, x_axis_pos);
    origin_label_xoy->setPos(circle->rect().center() + QPointF(-10,0));
    origin_label_yoz->setPos(z_axis->line().p1());

    // updating cone's and detector's yoz projections
    qreal z_end = ui->length->value() * scale;
    focon_up->setLine(0, x_axis_pos - cone->r1() * scale, z_end, x_axis_pos - cone->r2() * scale);
    focon_down->setLine(0, x_axis_pos + cone->r1() * scale, z_end, x_axis_pos + cone->r2() * scale);
    window_up->setLine(z_end, x_axis_pos - cone->r2() * scale, z_end, x_axis_pos - detector.window_radius() * scale);
    window_down->setLine(z_end, x_axis_pos + cone->r2() * scale, z_end, x_axis_pos + detector.window_radius() * scale);
    detector_yoz->setLine(detector.detector_z() * scale, x_axis_pos + detector.r() * scale, detector.detector_z() * scale, x_axis_pos - detector.r() * scale);
}

void MainWindow::clear() {
    for (auto& beam : beams) {
        delete beam;
    }
    beams.clear();

    for (auto& beam : beams_xoy) {
        delete beam;
    }
    beams_xoy.clear();
}

void MainWindow::draw(int rotation_angle) {
    qreal theta = qDegreesToRadians(static_cast<qreal>(rotation_angle));
    for (int i = 0; i < points.size()-1; ++i) {        
        QLineF line = QLineF(points[i].z() * scale, -(-points[i].y()*qCos(theta) + points[i].x()*qSin(theta)) * scale + scene->height()/2,
                             points[i+1].z() * scale, -(-points[i+1].y()*qCos(theta) + points[i+1].x()*qSin(theta))* scale + scene->height()/2);
        beams.push_back(new QGraphicsLineItem(line));
        scene->addItem(beams.back());

        QLineF line_xoy = QLineF(-(points[i].x()*qCos(theta) + points[i].y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                 -(points[i].x()*qSin(theta) - points[i].y()*qCos(theta))*scale_xoy + diameter/2 + margin,
                                 -(points[i+1].x()*qCos(theta) + points[i+1].y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                 -(points[i+1].x()*qSin(theta) - points[i+1].y()*qCos(theta))*scale_xoy + diameter/2 + margin);
        beams_xoy.push_back(new QGraphicsLineItem(line_xoy));
        scene->addItem(beams_xoy.back());

        set_beam_color(beams[i], single_beam_status);
        set_beam_color(beams_xoy[i], single_beam_status);
    }
    draw_axes(rotation_angle);
    draw_axes(theta);
}

void MainWindow::draw(const Point& point, BeamStatus status, int rotation_angle) {
    qreal theta = qDegreesToRadians(static_cast<qreal>(rotation_angle));
    switch (ui->mode->currentIndex()) {
        case PARALLEL_BUNDLE: {
            QLineF line_xoy = QLineF(-(point.x()*qCos(theta) + point.y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                     -(point.x()*qSin(theta) - point.y()*qCos(theta))*scale_xoy + diameter/2 + margin,
                                     -(point.x()*qCos(theta) + point.y()*qSin(theta))*scale_xoy + scene->width() - diameter/2,
                                     -(point.x()*qSin(theta) - point.y()*qCos(theta))*scale_xoy + diameter/2 + margin);
            beams_xoy.push_back(new QGraphicsLineItem(line_xoy));
            scene->addItem(beams_xoy.back());

            set_beam_color(beams_xoy.back(), status);
        } break;
        case DIVERGENT_BUNDLE: {
            QLineF line = QLineF(start.z() * scale, -(-start.y()*qCos(theta) + start.x()*qSin(theta)) * scale + scene->height()/2,
                                 point.z()* scale, -(-point.y()*qCos(theta) + point.x()*qSin(theta))* scale + scene->height()/2);
            beams.push_back(new QGraphicsLineItem(line));
            scene->addItem(beams.back());

            set_beam_color(beams.back(), status);
        } break;
    }
    draw_axes(rotation_angle);
    draw_axes(theta);
}

void MainWindow::draw_axes(int rotation_angle) {
    x_axis_xoy->setRotation(rotation_angle);
    y_axis_xoy->setRotation(rotation_angle);
    x_label_xoy->setRotation(rotation_angle);
    y_label_xoy->setRotation(rotation_angle);
}

void MainWindow::draw_axes(qreal theta) {
    qreal x_axis_pos = scene->height()/2;
    y_axis->setLine(0,x_axis_pos-x_axis_pos*qCos(theta),0,x_axis_pos+x_axis_pos*qCos(theta));
    y_label_yoz->setPos(y_axis->line().p1() + y_axis_label_offset);
}

void MainWindow::set_beam_color(QGraphicsLineItem * beam, BeamStatus status) {
    QPen pen;
    switch (status) {
    case REFLECTED:
        pen.setColor(Qt::red);
        break;
    case MISSED:
        pen.setColor(QColor(255, 165, 0));
        break;
    case HIT:
        pen.setColor(Qt::yellow);
        break;
    case DETECTED:
        pen.setColor(Qt::green);
        break;
    }
    beam->setPen(pen);
}

void MainWindow::rotate(int rotation_angle) {
    clear();
    switch (ui->mode->currentIndex()) {
    case SINGLE_BEAM_CALCULATION:
        draw(rotation_angle);
        break;
    case PARALLEL_BUNDLE:
        for (int i = 0; i < points.size(); ++i) {
            draw(points[i], statuses[i], rotation_angle);
            if (points[i].x() != 0) {
                draw(points[i].x_pair(), statuses[i], rotation_angle);
            }
        }
        break;
    case DIVERGENT_BUNDLE:
        for (int i = 0; i < points.size(); ++i) {
            draw(points[i], statuses[i], rotation_angle);
        }
        break;
    }
}

void MainWindow::build() {
    clear();
    points.clear();
    statuses.clear();
    init();
    ui->rotation->setEnabled(ui->mode->currentIndex() < EXHAUSTIVE_SAMPLING);

    switch (ui->mode->currentIndex()) {
    case SINGLE_BEAM_CALCULATION:
        single_beam_status = calculate_single_beam_path();
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

void MainWindow::save_settings() {
    QJsonObject json_file = { {"D1", ui->d_in->value()},
                              {"D2", ui->d_out->value()},
                              {"Length", ui->length->value()},
                              {"Angle", ui->angle->value()},
                              {"X offset", ui->offset->value()},
                              {"Y offset", ui->height->value()},
                              {"Detector's FOV", ui->fov->value()},
                              {"Detector's diameter", ui->d_det->value()},
                              {"Mode", ui->mode->currentIndex()},
                              {"Rotation", ui->rotation->value()}
                            };
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить файл"),
                                                    QCoreApplication::applicationDirPath() + "//untitled.foc",
                                                    tr("Файлы настроек фокона (*.foc)"));
    if (!fileName.isNull()) {
        QFile file(fileName);
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&file);
            out << QJsonDocument(json_file).toJson();
            file.close();
        }
        ui->statusbar->showMessage("Настройки сохранены");
    }
}

void MainWindow::load_settings() {
    QString filepath = QFileDialog::getOpenFileName(nullptr, "Открыть файл настроек",
                                                    QCoreApplication::applicationDirPath(),
                                                    "Файлы настроек фокона (*.foc)");
    QFile file;
    file.setFileName(filepath);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString val = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject json_file = doc.object();

    ui->d_in->setValue(json_file.value("D1").toDouble());
    ui->d_out->setValue(json_file.value("D2").toDouble());
    ui->length->setValue(json_file.value("Length").toDouble());
    ui->angle->setValue(json_file.value("Angle").toDouble());
    ui->offset->setValue(json_file.value("X offset").toDouble());
    ui->height->setValue(json_file.value("Y offset").toDouble());
    ui->fov->setValue(json_file.value("Detector's FOV").toDouble());
    ui->d_det->setValue(json_file.value("Detector's diameter").toDouble());
    ui->rotation->setValue(json_file.value("Rotation").toDouble());
    ui->mode->setCurrentIndex(json_file.value("Mode").toInt());

    ui->statusbar->showMessage("Настройки загружены");
}

void MainWindow::save_image() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить изображение"),
                                                    QCoreApplication::applicationDirPath(),
                                                    tr("PNG-изображение (*.png)"));
    if (!fileName.isNull()) {
        QPixmap pixMap = ui->view->grab();
        pixMap.save(fileName);
        ui->statusbar->showMessage("Изображение сохранено: " + fileName);
    }
}

MainWindow::BeamStatus MainWindow::calculate_single_beam_path() {
    points.push_back(start);
    // Perpendicular beams cause infinite loop in tubes
    if (cone->r1() == cone->r2() && fabs(beam.d_y()) == 1) {
        ui->statusbar->showMessage("Некорректный входной угол");
        return REFLECTED;
    }

    while(true) {
        intersection = cone->intersection(beam);

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

        // Transforming the beam after hitting a point outside of the cone is not necessary
        if (intersection.z() < 0 || intersection.z() > cone->length()) break;

        QLineF line = QLineF(0, 0, intersection.x(), intersection.y());
        qreal ksi = qDegreesToRadians(-90 + line.angle());
        qreal phi = cone->phi();
        Matrix m = Matrix(ksi, phi);

        Beam transformed_beam = m*beam.unit(intersection);
        transformed_beam.reflect();
        beam = m.transponed()*transformed_beam;
    }


    BeamStatus status;
    if (intersection.z() < cone->length()) {
        status = REFLECTED;
    } else if (detector.missed(beam)) {
        status = MISSED;
    } else if (detector.detected(beam)) {
        status = DETECTED;
    } else status = HIT;

    return status;
}

void MainWindow::calculate_parallel_beams() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = 50;
    for (int i = 0; i < count; ++i) {
        qreal x = i * cone->r1() / count;
        for (int j = -count; j < count; ++j) {
            qreal y = j * cone->r1() / count;
            start = Point(-x, -y, 0);
            if (start.is_in_radius(cone->r1())) {
                beam = Beam(start, ui->angle->value());
                // The results are simmetrical relative to y axis, hence doubling total count for i > 0
                beams_total += (i > 0 ? 2 : 1);
                BeamStatus status = calculate_single_beam_path();
                statuses.push_back(status);
                if (status == DETECTED) {
                    beams_passed += (i > 0 ? 2 : 1);
                }
                draw(points.back(), status, ui->rotation->value());
                if (x > 0) {
                    draw(points.back().x_pair(), status, ui->rotation->value());
                }
            }
        }
    }
    show_results(beams_passed, beams_total);
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
        BeamStatus status = calculate_single_beam_path();
        statuses.push_back(status);
        if (status == DETECTED) {
            ++beams_passed;
        }
        draw(points.back(), status, ui->rotation->value());
    }
    show_results(beams_passed, beams_total);
    return qMakePair(beams_passed, beams_total);
}

void MainWindow::calculate_every_beam() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = 20;
    for (int i = 0; i < count; ++i) {
        qreal x = i * cone->r1() / count;
        for (int j = 0; j < count; ++j) {
            qreal y = j * cone->r1() / count;
            start = Point(-x, -y, 0);
            if (start.is_in_radius(cone->r1())) {
                QPair<int, int> current_result = calculate_divergent_beams();
                // The results are simmetrical relative to y axis, hence doubling count for i > 0
                // The results are also simmetrical relative to x axis due to divergent beam modelling method used
                beams_passed += (i > 0 ? 2 : 1) * (j > 0 ? 2 : 1) * current_result.first;
                beams_total += (i > 0 ? 2 : 1) * (j > 0 ? 2 : 1) * current_result.second;
            }
        }
    }
    show_results(beams_passed, beams_total);
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
            BeamStatus status = calculate_single_beam_path();
            if (status == DETECTED) {
                ++beams_passed;
            }
        } else --i;
    }
    show_results(beams_passed, beams_total);
}

void MainWindow::show_results(int beams_passed, int beams_total) {
    QString passed = "Принято ";
    QString beams_of = " лучей из ";

    if ((beams_passed % 100 - beams_passed % 10) != 10) {
        if (beams_passed % 10 == 1) {
            passed = "Принят ";
            beams_of = " луч из ";
        } else if (beams_passed % 10 > 1 && beams_passed % 10 < 5) {
            beams_of = " луча из ";
        }
    }

    ui->statusbar->showMessage(passed + QString().setNum(beams_passed) + beams_of + QString().setNum(beams_total)
                               + ". Потери составляют " + QString().setNum(10*qLn(static_cast<qreal>(beams_total)/beams_passed)/qLn(10)) + " дБ.");
}
