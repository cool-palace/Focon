#include "mainwindow.h"
#include "ui_mainwindow.h"


void MainWindow::init_objects() {
    // updating geometrical objects and detector
    start = Point(-ui->offset->value(), -ui->height->value(), 0);
    init_cone(ui->d_in->value(), ui->d_out->value(), ui->length->value());
    beam = Beam(start, ui->angle->value());
    detector = Detector(ui->aperture->value(), ui->length->value(), ui->offset_det->value(),
                        ui->fov->value(), ui->d_det->value());
    qreal focus = lens_focus(ui->auto_focus->isChecked());
    lens.set_focus(focus);
    ui->focal_length->setValue(focus);
}

void MainWindow::init_cone(qreal d1, qreal d2, qreal length) {
    bool different_diameters = qFabs(d1 - d2) > 1e-6;
    bool type_change_needed = cone && different_diameters != cone->is_conic();
    if (cone && type_change_needed) {
        delete cone;
        cone = nullptr;
    }
    if (!cone) {
        cone = different_diameters
                ? new Cone(d1, d2, length)
                : new Tube(d1, length);
    } else {
        cone->set_d1(d1);
        cone->set_d2(d2);
        cone->set_length(length);
    }
}

qreal MainWindow::lens_focus(bool auto_focus) {
    if (auto_focus) {
        int l = static_cast<int>(ui->defocus_minus->isChecked());
        int m = static_cast<int>(ui->defocus_plus->isChecked());
        return detector.detector_z() * (cone->r1()/(cone->r1() + detector.r() * (l - m)));
    } else return ui->focal_length->value();
}

void MainWindow::build() {
    clear();
    points.clear();
    statuses.clear();
    init_graphics();
    ui->rotation->setEnabled(ui->mode->currentIndex() < EXHAUSTIVE_SAMPLING);

    switch (ui->mode->currentIndex()) {
    case SINGLE_BEAM_CALCULATION:
        if (start.is_in_radius(cone->r1())) {
            single_beam_status = calculate_single_beam_path();
            draw(ui->rotation->value());
            if (points.size() > 1) {
                ui->statusbar->showMessage("Количество отражений: " + QString().setNum(points.size()-2));
            }
        } else ui->statusbar->showMessage("Заданная точка входа луча находится вне апертуры.");
        break;
    case PARALLEL_BUNDLE:
        show_results(calculate_parallel_beams());
        break;
    case DIVERGENT_BUNDLE:
        show_results(calculate_divergent_beams());
        break;
    case EXHAUSTIVE_SAMPLING:
        show_results(calculate_every_beam());
        break;
    case MONTE_CARLO_METHOD:
        show_results(monte_carlo_method());
        break;
    case LENGTH_OPTIMISATION:
        show_results(optimal_length());
        break;
    case FOCUS_OPTIMISATION:
        if (ui->lens->isChecked()) {
            show_results(optimal_focus());
        } else ui->statusbar->showMessage("Для оптимизации линзы необходимо включить её в систему.");
        break;
    case D_OUT_OPTIMISATION:
        show_results(optimal_d_out());
        break;
    default:
        break;
    }
}

MainWindow::BeamStatus MainWindow::calculate_single_beam_path() {
    points.push_back(start);
    // Perpendicular beams cause infinite loop in tubes
    if (qFabs(cone->r1() - cone->r2()) < 1e-6 && qFabs(beam.d_y()) > 0.999999) {
        ui->statusbar->showMessage("Некорректный входной угол");
        return REFLECTED;
    }

    if (ui->lens->isChecked()) {
        beam = lens.refracted(beam).unit(beam.p1());
    }

    int i = 0;
//    qDebug() << "Beam start " << beam.x() << beam.y() << beam.z();
    while(true /*&& i < 60*/) {
        ++i;
//        qDebug() << i;
//        if (i > 48) qDebug() << "Beam " << beam.x() << beam.y() << beam.z();
        intersection = cone->intersection(beam);
//        if (i > 48) qDebug() << "Intersection " << intersection.x() << intersection.y() << intersection.z();

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

        // In complex modes there is no need to calculate full path of reflected beams
        if (ui->mode->currentIndex() != SINGLE_BEAM_CALCULATION && beam.cos_g() < 0) break;
    }

    BeamStatus status;
    if (intersection.z() < cone->length()) {
        status = REFLECTED;
        // Cut the reflected beams' tails at the cone's entrance so the projections are cleaner
        if (ui->mode->currentIndex() == SINGLE_BEAM_CALCULATION) {
            points.back() = Plane(0).intersection(beam);
        }
    } else if (detector.missed(beam)) {
        status = MISSED;
    } else if (detector.detected(beam)) {
        status = DETECTED;
    } else status = HIT;

    return status;
}

QPair<int, int> MainWindow::calculate_parallel_beams() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = ui->precision->currentIndex() ? 50 : 25;
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
                if (ui->mode->currentIndex() == PARALLEL_BUNDLE) {
                    draw(points.back(), status, ui->rotation->value());
                    if (x > 0) {
                        draw(points.back().x_pair(), status, ui->rotation->value());
                    }
                }
            }
        }
    }
    return qMakePair(beams_passed, beams_total);
}

QPair<int, int> MainWindow::calculate_divergent_beams() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = ui->precision->currentIndex() ? 10 : 5;
    int limit = abs(static_cast<int>(ui->angle->value() * count));
    for (int i = -limit; i <= limit; ++i) {
        qreal angle = static_cast<qreal>(i) / count;
        beam = Beam(start, angle);
        ++beams_total;
        BeamStatus status = calculate_single_beam_path();
        statuses.push_back(status);
        if (status == DETECTED) {
            ++beams_passed;
        }
        draw(points.back(), status, ui->rotation->value());
    }
    return qMakePair(beams_passed, beams_total);
}

QPair<int, int> MainWindow::calculate_every_beam() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = ui->precision->currentIndex() ? 25 : 20;
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
    return qMakePair(beams_passed, beams_total);
}

QPair<int, int> MainWindow::monte_carlo_method() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = ui->precision->currentIndex() ? 100000 : 10000;
    QRandomGenerator rng;
    for (int i = 0; i < count; ++i) {
        qreal x = 2 * rng.generateDouble() - 1;
        qreal y = 2 * rng.generateDouble() - 1;
        start = Point(x * cone->r1(), y * cone->r1(), 0);
        if (start.is_in_radius(cone->r1())) {
            beam = Beam(start, (2 * rng.generateDouble() - 1) * fabs(ui->angle->value()));
            ++beams_total;
            BeamStatus status = calculate_single_beam_path();
            if (status == DETECTED) {
                ++beams_passed;
            }
        } else --i;
    }
    return qMakePair(beams_passed, beams_total);
}

QPair<int, qreal> MainWindow::optimal_length() {
    int max = 0;
    int optimal_value = 0;
    int first_step = 5;
    int not_improving_length_limit = 150;
    int not_changing_limit = not_improving_length_limit / first_step;
    QPair<int, int> result;
    QPair<int, int> max_result;
    // The optimisation is done in 2 iterations with increasing accuracy
    for (int iteration = 0; iteration < 2; ) {
        int low_limit = iteration == 0 ? 2*qCeil(cone->d1()) : optimal_value - (first_step - 1);
        int high_limit = iteration == 0 ? length_limit : optimal_value + (first_step - 1);
        int step = iteration == 0 ? first_step : 1;
        int not_changing_count = 0;
        // Conditions for breaking the cycle:
        // 1. Length value is out of range
        // 2. It is the first iteration and the results did not improve for 150 mm (the value is arbitrary)
        // Increasing cone's length tends to increase both the computation time and the loss value for non-zero beam bundles
        // so it's reasonable to cut the calculations short when the results become predictable
        for (int i = low_limit; i <= high_limit && (iteration == 1 || not_changing_count < not_changing_limit); i += step) {
            qreal length = static_cast<qreal>(i);
            cone->set_length(length);
            detector.set_position(length);
            lens.set_focus(lens_focus(ui->auto_focus->isChecked()));
            // Optimal length criterion №1: Acceptable loss value for parallel bundle at given angle
            if (loss(calculate_parallel_beams()) < loss_limit) {
                result = calculate_every_beam();
                int current_value = result.first;
                // Optimal length criterion №2: Minimum loss (maximum number of beams passing) in exhaustive sampling
                if (current_value > max || (current_value == max && i <= optimal_value)) {
                    max = current_value;
                    optimal_value = i;
                    max_result = result;
                    not_changing_count = 0;
                } else {
                    ++not_changing_count;
                }
                qDebug() << i << current_value;
            } else {
                qDebug() << "High loss value at " << i << " mm";
            }
        }
        qDebug() << optimal_value << max;

        if (iteration == 0) {
            // If the first iteration gives a positive result then start the second, else show a fail message
            if (max > 0) {
                ++iteration;
            } else return qMakePair(length_limit, loss_limit);
        } else break;
    }
    return qMakePair(optimal_value, loss(max_result));
}

QPair<int, qreal> MainWindow::optimal_focus() {
    // The lower bound of focus length is determined by the f-number of the lens (k = f'/D_in >= 1).
    // The upper bound corresponds to forming a beam parallel to the axis on the edge of the lens
    // and is determined by the system's FOV (or input beam angle value) and cone's entrance diameter.
    // Further increasing focus length is totally possible but seems to be pointless in our case.
    int low_limit = qCeil(cone->d1());
    int high_limit = qCeil(cone->r1()/qTan(qAcos(beam.cos_g())));
    int max = 0;
    int optimal_value = 0;
    QPair<int, int> result;
    QPair<int, int> max_result;
    for (int focus = low_limit; focus <= high_limit; ++focus) {
        lens.set_focus(focus);
        // Optimal length criterion №1: Acceptable loss value for parallel bundle at given angle
        if (loss(calculate_parallel_beams()) < loss_limit) {
            result = calculate_every_beam();
            int current_value = result.first;
            // Optimal length criterion №2: Minimum loss (maximum number of beams passing) in exhaustive sampling
            if (current_value > max || (current_value == max && focus <= optimal_value)) {
                max = current_value;
                optimal_value = focus;
                max_result = result;
            }
            qDebug() << focus << current_value;
        } else {
            qDebug() << "High loss value at " << focus << " mm";
        }
    }
    return qMakePair(optimal_value, loss(max_result));
}

QPair<int, qreal> MainWindow::optimal_d_out() {
    int count = 2;
    int start = qFloor(ui->d_det->value() * count);
    int end = qCeil(ui->aperture->value()) * count;
    qreal max = 0;
    qreal optimal_value = 0;
    QPair<int, int> result;
    QPair<int, int> max_result;
    bool decrease_started = false;
    for (int i = start; i <= end || !decrease_started; ++i) {
        qreal d_out = static_cast<qreal>(i) / count;
        init_cone(cone->d1(), d_out, cone->length());
        result = calculate_every_beam();
        int current_value = result.first;
        if (current_value > max) {
            max = current_value;
            optimal_value = d_out;
            max_result = result;
        } else {
            decrease_started = true;
        }
        qDebug() << d_out << current_value;
    }
    return qMakePair(optimal_value, loss(max_result));
}

qreal MainWindow::loss(QPair<int, int> result) {
    int beams_passed = result.first;
    int beams_total = result.second;
    return 10*qLn(static_cast<qreal>(beams_total)/beams_passed)/qLn(10);
}

