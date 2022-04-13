#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , lens(Lens(1))
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
    , lens_yoz(new QGraphicsLineItem())
    , lens_arrow_up_left(new QGraphicsLineItem())
    , lens_arrow_up_right(new QGraphicsLineItem())
    , lens_arrow_down_left(new QGraphicsLineItem())
    , lens_arrow_down_right(new QGraphicsLineItem())
    , ocular_yoz(new QGraphicsLineItem())
    , ocular_arrow_up_left(new QGraphicsLineItem())
    , ocular_arrow_up_right(new QGraphicsLineItem())
    , ocular_arrow_down_left(new QGraphicsLineItem())
    , ocular_arrow_down_right(new QGraphicsLineItem())
    , x_label_xoy(new QGraphicsTextItem("x"))
    , y_label_xoy(new QGraphicsTextItem("y"))
    , y_label_yoz(new QGraphicsTextItem("y"))
    , z_label_yoz(new QGraphicsTextItem("z"))
    , origin_label_xoy(new QGraphicsTextItem("0"))
    , origin_label_yoz(new QGraphicsTextItem("0"))
    , polygon(new QGraphicsPolygonItem())

{
    ui->setupUi(this);

    connect(ui->load, SIGNAL(triggered(bool)), this, SLOT(load_settings()));
    connect(ui->save, SIGNAL(triggered(bool)), this, SLOT(save_settings()));
    connect(ui->save_whole_image, SIGNAL(triggered(bool)), this, SLOT(save_image()));
    connect(ui->save_image_xoy, SIGNAL(triggered(bool)), this, SLOT(save_image_xoy()));
    connect(ui->night_mode, SIGNAL(toggled(bool)), this, SLOT(set_colors(bool)));
    connect(ui->big_text, SIGNAL(toggled(bool)), this, SLOT(set_text_size(bool)));
    connect(ui->lens, SIGNAL(toggled(bool)), this, SLOT(set_lens(bool)));
    connect(ui->ocular, SIGNAL(toggled(bool)), this, SLOT(set_ocular(bool)));
    connect(ui->glass, SIGNAL(toggled(bool)), this, SLOT(set_glass(bool)));

    connect(ui->ocular_focal_length, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged), [&](qreal new_focus) {
        // The range (-d_out/2, d_out/2) should not be accessible
        if (qFabs(new_focus) < ui->d_out->value()/2) {
            ui->ocular_focal_length->setValue(ui->d_out->value()/2 * (new_focus > 0 ? -1 : 1));
        }
    });

    connect(ui->auto_focus, QOverload<bool>::of(&QCheckBox::toggled), this, [&](bool auto_focus) {
        ui->defocus->setEnabled(auto_focus);
        ui->focal_length->setEnabled(!auto_focus);
    });

    connect(ui->Hamamatsu_G12180_005A, QOverload<bool>::of(&QAction::triggered), this, [&]() {
        ui->aperture->setValue(2.2);
        ui->offset_det->setValue(1.1);
        ui->d_det->setValue(0.5);
    });

    connect(ui->Hamamatsu_G12180_010A, QOverload<bool>::of(&QAction::triggered), this, [&]() {
        ui->aperture->setValue(2.2);
        ui->offset_det->setValue(1.1);
        ui->d_det->setValue(1);
    });

    connect(ui->Hamamatsu_G12180_020A, QOverload<bool>::of(&QAction::triggered), this, [&]() {
        ui->aperture->setValue(4.5);
        ui->offset_det->setValue(2.4);
        ui->d_det->setValue(2);
    });

    // The default presicion is high and the setting is disabled for single beam mode
    ui->precision->setCurrentIndex(1);
    ui->precision->setEnabled(ui->mode->currentIndex() != SINGLE_BEAM_CALCULATION);

    connect(ui->mode, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int mode) {
        clear();
        // Rotation should be disabled until recalculation in new mode
        ui->rotation->setEnabled(false);
        ui->precision->setEnabled(mode != SINGLE_BEAM_CALCULATION);
        bool point_coordinates_enabled = mode == SINGLE_BEAM_CALCULATION || mode == DIVERGENT_BUNDLE;
        ui->height->setEnabled(point_coordinates_enabled);
        ui->offset->setEnabled(point_coordinates_enabled);
        ui->length->setEnabled(mode != LENGTH_OPTIMISATION && mode != FULL_OPTIMISATION);
        ui->d_out->setEnabled(mode != D_OUT_OPTIMISATION && mode != FULL_OPTIMISATION);
        ui->focal_length->setEnabled(mode != FOCUS_OPTIMISATION && !ui->auto_focus->isChecked());
        ui->auto_focus->setEnabled(mode != FOCUS_OPTIMISATION && ui->lens->isChecked());
        ui->defocus->setEnabled(mode != FOCUS_OPTIMISATION && ui->lens->isChecked() && ui->auto_focus->isChecked());
        circle_out->setVisible(mode != PARALLEL_BUNDLE_EXIT);
        ui->detector_parameters->setEnabled(mode != PARALLEL_BUNDLE_EXIT);
    });

    ui->height->setMaximum(ui->d_in->value()/2);
    ui->height->setMinimum(-ui->d_in->value()/2);
    ui->offset->setMaximum(ui->d_in->value()/2);
    ui->offset->setMinimum(-ui->d_in->value()/2);

    ui->focal_length->setMinimum(ui->d_in->value());

    connect(ui->d_in, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged), [&](qreal new_diameter) {
        ui->height->setMaximum(new_diameter/2);
        ui->height->setMinimum(-new_diameter/2);
        ui->offset->setMaximum(new_diameter/2);
        ui->offset->setMinimum(-new_diameter/2);
        ui->focal_length->setMinimum(new_diameter/2);
    });

    connect(ui->length, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged), [&](qreal length) {
        ui->focal_length->setMaximum(length + 100);
    });

    ui->view->setScene(scene);
    scene->setSceneRect(0,0,ui->view->width()-margin, ui->view->height());
    init_graphics();

    qreal x_axis_pos = scene->height()/2;

    // setting axes' projections
    y_axis->setLine(0, 0, 0, x_axis_pos*2);
    x_axis_xoy->setLine(circle->rect().x() - margin, diameter/2 + margin + margin, circle->rect().x() + diameter + margin, diameter/2 + margin + margin);
    y_axis_xoy->setLine(circle->rect().x()+diameter/2, margin, circle->rect().x() + diameter/2, diameter + 2*margin + margin);
    x_axis_xoy->setTransformOriginPoint(x_axis_xoy->line().center());
    y_axis_xoy->setTransformOriginPoint(y_axis_xoy->line().center());

    // setting lens arrows
    lens_arrow_up_left->setLine(0,0,-5,10);
    lens_arrow_up_right->setLine(0,0,5,10);
    lens_arrow_down_left->setLine(0,0,-5,-10);
    lens_arrow_down_right->setLine(0,0,5,-10);

    // setting ocular arrows
    ocular_arrow_up_left->setLine(0,0,-5,10);
    ocular_arrow_up_right->setLine(0,0,5,10);
    ocular_arrow_down_left->setLine(0,0,-5,-10);
    ocular_arrow_down_right->setLine(0,0,5,-10);

    // setting axes' labels
    x_label_xoy->setPos(x_axis_xoy->line().p2() + x_axis_label_offset);
    y_label_xoy->setPos(y_axis_xoy->line().p1() + y_axis_label_offset);
    y_label_yoz->setPos(y_axis->line().p1() + y_axis_label_offset);
    z_label_yoz->setPos(z_axis->line().p2() + x_axis_label_offset);
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

    polygon->setPen(QColor(0, 0, 0, 0));

    set_lens(ui->lens->isChecked());
    set_ocular(ui->ocular->isChecked());
    set_colors(ui->night_mode->isChecked());
    polygon->hide();

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
    scene->addItem(lens_yoz);
    scene->addItem(lens_arrow_up_left);
    scene->addItem(lens_arrow_up_right);
    scene->addItem(lens_arrow_down_left);
    scene->addItem(lens_arrow_down_right);
    scene->addItem(ocular_yoz);
    scene->addItem(ocular_arrow_up_left);
    scene->addItem(ocular_arrow_up_right);
    scene->addItem(ocular_arrow_down_left);
    scene->addItem(ocular_arrow_down_right);
    scene->addItem(x_label_xoy);
    scene->addItem(y_label_xoy);
    scene->addItem(y_label_yoz);
    scene->addItem(z_label_yoz);
    scene->addItem(origin_label_xoy);
    scene->addItem(origin_label_yoz);
    scene->addItem(polygon);
}

MainWindow::~MainWindow() {
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

void MainWindow::init_graphics() {
    init_objects();

    qreal x_axis_length = scene->width();
    qreal y_axis_length = scene->height();
    qreal x_axis_pos = scene->height()/2;

    // rescaling
    qreal scale_based_on_length = x_axis_length / (detector.detector_z());
    qreal scale_based_on_diameter = y_axis_length / qMax(cone->d1(), cone->d2());
    scale = qMin(scale_based_on_length, scale_based_on_diameter);
    scale_xoy = diameter / qMax(cone->d1(), cone->d2());
    scale_exit_xoy = diameter / cone->d2();

    // updating cone's xoy projection
    qreal diameter_outer = qMin(cone->d1(), cone->d2()) * scale_xoy;
    qreal circle_out_x = scene->width() - diameter/2 - diameter_outer/2;
    qreal circle_out_y = diameter/2 - diameter_outer/2;
    circle_out->setRect(circle_out_x - margin, circle_out_y + margin + margin, diameter_outer, diameter_outer);
    circle->setRect(scene->width()-diameter - margin, margin + margin, diameter, diameter);

    // updating z axis' projection and origin points
    z_axis->setLine(-margin, x_axis_pos, x_axis_length + margin, x_axis_pos);
    origin_label_xoy->setPos(circle->rect().center() + QPointF(-13,-5));
    origin_label_yoz->setPos(z_axis->line().p1() + QPointF(-5,-2));

    // updating cone's and detector's yoz projections
    qreal z_end = cone->length() * scale;
    focon_up->setLine(0, x_axis_pos - cone->r1() * scale, z_end, x_axis_pos - cone->r2() * scale);
    focon_down->setLine(0, x_axis_pos + cone->r1() * scale, z_end, x_axis_pos + cone->r2() * scale);
    window_up->setLine(z_end, x_axis_pos - cone->r2() * scale, z_end, x_axis_pos - detector.window_radius() * scale);
    window_down->setLine(z_end, x_axis_pos + cone->r2() * scale, z_end, x_axis_pos + detector.window_radius() * scale);
    detector_yoz->setLine(detector.detector_z() * scale, x_axis_pos + detector.r() * scale, detector.detector_z() * scale, x_axis_pos - detector.r() * scale);

    // updating lens representation
    QLineF line = QLineF(focon_up->line().p1(), focon_down->line().p1());
    lens_yoz->setLine(line);
    lens_arrow_up_left->setPos(focon_up->line().p1());
    lens_arrow_up_right->setPos(focon_up->line().p1());
    lens_arrow_down_left->setPos(focon_down->line().p1());
    lens_arrow_down_right->setPos(focon_down->line().p1());

    // updating ocular representation
    line = QLineF(focon_up->line().p2(), focon_down->line().p2());
    ocular_yoz->setLine(line);
    bool ocular_positive = ui->ocular_focal_length->value() > 0;
    ocular_arrow_up_left->setPos((ocular_positive ? focon_up : focon_down)->line().p2());
    ocular_arrow_up_right->setPos((ocular_positive ? focon_up : focon_down)->line().p2());
    ocular_arrow_down_left->setPos((ocular_positive ? focon_down : focon_up)->line().p2());
    ocular_arrow_down_right->setPos((ocular_positive ? focon_down : focon_up)->line().p2());

    set_glass(ui->glass->isChecked());
}

void MainWindow::set_colors(bool night_theme_on) {
    scene->setBackgroundBrush(QBrush(night_theme_on ? QColor(64,64,64) : Qt::white));
    auto color = night_theme_on ? Qt::white : Qt::black;

    QPen axis_pen(QBrush(color),1,Qt::DashDotLine);
    y_axis->setPen(axis_pen);
    z_axis->setPen(axis_pen);
    x_axis_xoy->setPen(axis_pen);
    y_axis_xoy->setPen(axis_pen);

    QPen pen = QPen(color);
    circle->setPen(pen);
    circle_out->setPen(pen);
    focon_up->setPen(pen);
    focon_down->setPen(pen);
    window_up->setPen(pen);
    window_down->setPen(pen);
    detector_yoz->setPen(pen);
    lens_yoz->setPen(pen);
    lens_arrow_up_left->setPen(pen);
    lens_arrow_up_right->setPen(pen);
    lens_arrow_down_left->setPen(pen);
    lens_arrow_down_right->setPen(pen);
    ocular_yoz->setPen(pen);
    ocular_arrow_up_left->setPen(pen);
    ocular_arrow_up_right->setPen(pen);
    ocular_arrow_down_left->setPen(pen);
    ocular_arrow_down_right->setPen(pen);

    set_glass(ui->glass->isChecked());
    polygon->setPen(pen);

    x_label_xoy->setDefaultTextColor(color);
    y_label_xoy->setDefaultTextColor(color);
    y_label_yoz->setDefaultTextColor(color);
    z_label_yoz->setDefaultTextColor(color);
    x_label_xoy->setDefaultTextColor(color);
    y_label_xoy->setDefaultTextColor(color);
    origin_label_xoy->setDefaultTextColor(color);
    origin_label_yoz->setDefaultTextColor(color);
}

void MainWindow::set_text_size(bool big_font) {
    auto font = QFont("MS Shell Dlg 2", big_font ? 12 : 8);
    auto widgets = children();
    for (auto& widget : widgets) {
        widget->setProperty("font", font);
    }
}

void MainWindow::set_lens(bool visible) {
    lens_yoz->setVisible(visible);
    lens_arrow_up_left->setVisible(visible);
    lens_arrow_up_right->setVisible(visible);
    lens_arrow_down_left->setVisible(visible);
    lens_arrow_down_right->setVisible(visible);
    if (ui->mode->currentIndex() == FOCUS_OPTIMISATION) return;
    ui->auto_focus->setEnabled(visible);
    ui->defocus->setEnabled(visible && ui->auto_focus->isChecked());
}

void MainWindow::set_ocular(bool visible) {
    ocular_yoz->setVisible(visible);
    ocular_arrow_up_left->setVisible(visible);
    ocular_arrow_up_right->setVisible(visible);
    ocular_arrow_down_left->setVisible(visible);
    ocular_arrow_down_right->setVisible(visible);
}

void MainWindow::set_glass(bool glass_on) {
    QVector<QPointF> polygon_points = {focon_up->line().p2(), focon_up->line().p1(), focon_down->line().p1(), focon_down->line().p2()};
    if (cavity) {
        QPointF cavity_vertex((cavity->z_k()) * scale, scene->height()/2);
        polygon_points.push_back(cavity_vertex);
    }

//    auto polygon = new QGraphicsPolygonItem(QPolygonF(polygon_points));
    polygon->setPolygon(QPolygonF(polygon_points));
    polygon->setBrush(glass_on
                      ? (ui->night_mode->isChecked() ? glass_brush_white : glass_brush_black)
                      : QBrush());

//    QPointF cavity_vertex = ((cone->length() - cavity.length()) * scale, scene->height()/2);
    ui->lens->setDisabled(glass_on);
    ui->ocular->setDisabled(glass_on);
    focon_up->setVisible(!glass_on);
    focon_down->setVisible(!glass_on);
    polygon->setVisible(glass_on);
//    lens_yoz->setVisible(glass_on);
//    ocular_yoz->setVisible(glass_on);
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

        QLineF line_xoy = QLineF(-(points[i].x()*qCos(theta) + points[i].y()*qSin(theta))*scale_xoy + scene->width() - diameter/2 - margin,
                                 -(points[i].x()*qSin(theta) - points[i].y()*qCos(theta))*scale_xoy + diameter/2 + margin + margin,
                                 -(points[i+1].x()*qCos(theta) + points[i+1].y()*qSin(theta))*scale_xoy + scene->width() - diameter/2 - margin,
                                 -(points[i+1].x()*qSin(theta) - points[i+1].y()*qCos(theta))*scale_xoy + diameter/2 + margin + margin);
        beams_xoy.push_back(new QGraphicsLineItem(line_xoy));
        scene->addItem(beams_xoy.back());

        set_beam_color(beams[i], single_beam_status);
        set_beam_color(beams_xoy[i], single_beam_status);
    }
}

void MainWindow::draw(const Point& point, BeamStatus status, int rotation_angle) {
    qreal theta = qDegreesToRadians(static_cast<qreal>(rotation_angle));
    switch (ui->mode->currentIndex()) {
        case PARALLEL_BUNDLE: {
            QLineF line_xoy = QLineF(-(point.x()*qCos(theta) + point.y()*qSin(theta))*scale_xoy + scene->width() - diameter/2 - margin,
                                     -(point.x()*qSin(theta) - point.y()*qCos(theta))*scale_xoy + diameter/2 + margin + margin,
                                     -(point.x()*qCos(theta) + point.y()*qSin(theta))*scale_xoy + scene->width() - diameter/2 - margin,
                                     -(point.x()*qSin(theta) - point.y()*qCos(theta))*scale_xoy + diameter/2 + margin + margin);
            beams_xoy.push_back(new QGraphicsLineItem(line_xoy));
            scene->addItem(beams_xoy.back());
            set_beam_color(beams_xoy.back(), status);
        } break;
        case DIVERGENT_BUNDLE: {
            auto start = starting_point();
            QLineF line = QLineF(start.z() * scale, -(-start.y()*qCos(theta) + start.x()*qSin(theta)) * scale + scene->height()/2,
                                 point.z()* scale, -(-point.y()*qCos(theta) + point.x()*qSin(theta))* scale + scene->height()/2);
            beams.push_back(new QGraphicsLineItem(line));
            scene->addItem(beams.back());
            set_beam_color(beams.back(), status);
        } break;
    }
}

void MainWindow::draw(const Point& point, qreal beam_angle, int rotation_angle) {
    qreal theta = qDegreesToRadians(static_cast<qreal>(rotation_angle));
    QLineF line_xoy = QLineF(-(point.x()*qCos(theta) + point.y()*qSin(theta))*scale_exit_xoy + scene->width() - diameter/2 - margin,
                             -(point.x()*qSin(theta) - point.y()*qCos(theta))*scale_exit_xoy + diameter/2 + margin + margin,
                             -(point.x()*qCos(theta) + point.y()*qSin(theta))*scale_exit_xoy + scene->width() - diameter/2 - margin,
                             -(point.x()*qSin(theta) - point.y()*qCos(theta))*scale_exit_xoy + diameter/2 + margin + margin);
    beams_xoy.push_back(new QGraphicsLineItem(line_xoy));
    scene->addItem(beams_xoy.back());
    set_beam_color(beams_xoy.back(), beam_angle);
}

void MainWindow::draw_axes(int rotation_angle) {
    x_axis_xoy->setRotation(rotation_angle);
    y_axis_xoy->setRotation(rotation_angle);
    x_label_xoy->setRotation(rotation_angle);
    y_label_xoy->setRotation(rotation_angle);

    qreal theta = qDegreesToRadians(static_cast<qreal>(rotation_angle));
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

void MainWindow::set_beam_color(QGraphicsLineItem * beam, qreal angle) {
    int hue = 270 - qFloor(qFabs(angle)*3);
    QPen pen;
    pen.setColor(QColor::fromHsv(hue, 255, 255));
    beam->setPen(pen);
}

void MainWindow::rotate(int rotation_angle) {
    clear();
    draw_axes(rotation_angle);
    switch (ui->mode->currentIndex()) {
    case SINGLE_BEAM_CALCULATION:
        draw(rotation_angle);
        break;
    case PARALLEL_BUNDLE:
        for (int i = 0; i < points.size(); ++i) {
            draw(points[i], statuses[i], rotation_angle);
            if (qFabs(points[i].x()) > 1e-6) {
                draw(points[i].x_pair(), statuses[i], rotation_angle);
            }
        }
        break;
    case PARALLEL_BUNDLE_EXIT:
        for (int i = 0; i < points.size(); ++i) {
            draw(points[i], beam_angles[i], rotation_angle);
            if (qFabs(points[i].x()) > 1e-6) {
                draw(points[i].x_pair(), beam_angles[i], rotation_angle);
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

void MainWindow::show_results(const QPair<int, int>& result) {
    int beams_passed = result.first;
    int beams_total = result.second;
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
                               + ". Потери составляют " + QString().setNum(loss(result)) + " дБ.");
}

void MainWindow::show_results(const QPair<int, qreal>& result) {
    switch (ui->mode->currentIndex()) {
    case LENGTH_OPTIMISATION:
        if (result.first != length_limit || qFabs(result.second - loss_limit) > 1e-6) {
            ui->statusbar->showMessage("Оптимальная длина фокона составляет " + QString().setNum(result.first)
                                       + " мм. Потери составляют " + QString().setNum(result.second) + " дБ.");
        } else {
            ui->statusbar->showMessage("Оптимального значения длины в пределах до " + QString().setNum(length_limit)
                                       + " мм не найдено: потери для боковых пучков превышают "
                                       + QString().setNum(loss_limit) + " дБ. Попробуйте уменьшить входной угол пучка или увеличить допуск потерь.");
        }
        break;
    case FOCUS_OPTIMISATION:
        if (result.first > 0) {
            ui->statusbar->showMessage("Оптимальное фокусное расстояние составляет " + QString().setNum(result.first)
                                           + " мм. Потери составляют " + QString().setNum(result.second) + " дБ.");
        } else {
            ui->statusbar->showMessage("Оптимального значения фокусного расстояния не найдено: потери для боковых пучков превышают "
                                       + QString().setNum(loss_limit) + " дБ. Попробуйте уменьшить входной угол пучка или увеличить допуск потерь.");
        }
        break;
    default:
        break;
    }
}

void MainWindow::show_results(const QPair<qreal, qreal>& result) {
    if (result.first > 0) {
        ui->statusbar->showMessage("Оптимальный диаметр выходного окна оставляет " + QString().setNum(result.first)
                                               + " мм. Потери составляют " + QString().setNum(result.second) + " дБ.");
    } else {
        ui->statusbar->showMessage("Оптимального значения диаметра выходного окна не найдено: потери для боковых пучков превышают "
                                   + QString().setNum(loss_limit) + " дБ. Попробуйте уменьшить входной угол пучка или увеличить допуск потерь.");
    }
}

void MainWindow::show_results(const MainWindow::Parameters& result) {
    if (result.length > 0) {
        if (result.focus > 0) {
            ui->statusbar->showMessage("Оптимальные параметры: длина = " + QString().setNum(result.length)
                                       + " мм, выходной диаметр = " + QString().setNum(result.d_out)
                                       + " мм, фокусное расстояние = " + QString().setNum(result.focus)
                                       + " мм. Потери составляют " + QString().setNum(result.loss) + " дБ.");
        } else {
            ui->statusbar->showMessage("Оптимальные параметры: длина = " + QString().setNum(result.length)
                                       + " мм, выходной диаметр = " + QString().setNum(result.d_out)
                                       + " мм. Потери составляют " + QString().setNum(result.loss) + " дБ.");
        }
    } else {
        ui->statusbar->showMessage("Оптимальной комбинации параметров не найдено: потери для боковых пучков превышают "
                                   + QString().setNum(loss_limit) + " дБ. Попробуйте уменьшить входной угол пучка или увеличить допуск потерь.");
    }
}

void MainWindow::show_results(qreal result) {
    ui->statusbar->showMessage("Средний выходной угол = " + QString().setNum(result) + " градусов.");
}
