#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::init_objects() {
    init_cone(ui->d_in->value(), ui->d_out->value(), ui->length->value());
    detector = Detector(ui->aperture->value(), ui->length->value(), ui->offset_det->value(),
                        ui->fov->value(), ui->d_det->value());
    qreal focus = lens_focus(ui->auto_focus->isChecked());
    lens.set_focus(focus);
    ui->focal_length->setValue(focus);
    ocular = Lens(ui->ocular_focal_length->value(), cone->length());
    init_cavity(cone);
}

Point MainWindow::starting_point() { return Point(-ui->offset->value(), -ui->height->value(), 0); }

Beam MainWindow::starting_beam() { return Beam(starting_point(), ui->angle->value()); }

void MainWindow::init_cone(qreal d1, qreal d2, qreal length) {
    bool different_diameters = qFabs(d1 - d2) > 1e-6;
    bool type_change_needed = cone && different_diameters != cone->is_conic();
    if (type_change_needed) {
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
    if (ui->glass->isChecked()) {
        cone->set_n(1.5);
    }
}

qreal MainWindow::lens_focus(bool auto_focus) {
    if (auto_focus) {
        return detector.detector_z() * (cone->r1()/(cone->r1() - detector.r() * ui->defocus->value()));
    } else return ui->focal_length->value();
}

void MainWindow::init_cavity(Tube* glass_cone) {
    bool cavity_is_needed = ui->glass->isChecked() && ui->cavity_length->value() > 0;
    if (cavity_is_needed) {
        qreal length = glass_cone->length() - ui->cavity_length->value();
        qreal d1 = glass_cone->d2() * length / ui->cavity_length->value();
        if (cavity) {
            cavity->set_d1(d1);
            cavity->set_d2(0);
            cavity->set_length(length);
        } else cavity = new Cone(d1, 0, length);
    } else if (cavity) {
        delete cavity;
        cavity = nullptr;
    }
}

void MainWindow::build() {
    clear();
    points.clear();
    statuses.clear();
    beam_angles.clear();
    init_graphics();
    ui->rotation->setEnabled(ui->mode->currentIndex() < EXHAUSTIVE_SAMPLING);

    try {
        switch (ui->mode->currentIndex()) {
        case SINGLE_BEAM_CALCULATION:
            if (starting_point().is_in_radius(cone->r1())) {
                Beam beam = starting_beam();
                single_beam_status = calculate_single_beam_path(beam);
                draw(ui->rotation->value());
                if (points.size() > 1) {
                    ui->statusbar->showMessage("Количество отражений: " + QString().setNum(points.size() - 2 - static_cast<int>(ui->ocular->isChecked())));
                }
            } else ui->statusbar->showMessage("Заданная точка входа луча находится вне апертуры.");
            break;
        case PARALLEL_BUNDLE:
            draw_axes(ui->rotation->value());
            show_results(calculate_parallel_beams(ui->angle->value()));
            break;
        case PARALLEL_BUNDLE_EXIT:
            draw_axes(ui->rotation->value());
            calculate_parallel_beams(ui->angle->value());
            if (!beam_angles.empty()) {
                show_results(mean_exit_angle());
            } else ui->statusbar->showMessage("Ни один луч не достиг выходной апертуры.");
            break;
        case DIVERGENT_BUNDLE:
            draw_axes(ui->rotation->value());
            show_results(calculate_divergent_beams(starting_point()));
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
        case D_OUT_OPTIMISATION:
            show_results(optimal_d_out());
            break;
        case FOCUS_OPTIMISATION:
            if (ui->lens->isChecked()) {
                show_results(optimal_focus());
            } else ui->statusbar->showMessage("Для оптимизации линзы необходимо включить её в систему.");
            break;
        case FULL_OPTIMISATION:
            show_results(full_optimisation());
            break;
        default:
            break;
        }
    } catch (Beam& beam) {
        ui->statusbar->showMessage("Возникла ошибка при вычислении хода луча: x = " + QString().setNum(-beam.x())
                                   + ", y = " + QString().setNum(-beam.y()) + ", входной угол = " + QString().setNum(beam.gamma()));
    }
}

void MainWindow::transformation_on_entrance(Beam& beam) {
    if (ui->lens->isChecked()) {
        beam = lens.refracted(beam);
    } else if (ui->glass->isChecked()) {
        beam = cone->entrance().refracted(beam, 1, 1.5);
    }
}

void MainWindow::reflection_cycle(Beam& beam, const Beam& original_beam) {
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
            if (points.back() == original_beam.p1()) {
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
//            qDebug() << beam.d_x() << beam.d_y() << beam.d_z();
        Beam transformed_beam = m*beam.on_point(intersection);
//            qDebug() << transformed_beam;

        line = QLineF(0, 0, transformed_beam.d_z(), transformed_beam.d_x());
//            qDebug() << line.angle();
        qreal theta = qDegreesToRadians(360 - line.angle());
//            qDebug() << qRadiansToDegrees(theta);
        Matrix m_y = Matrix(theta);
        Matrix m_xzy = Matrix(ksi, phi, theta);
        auto test_beam = m_y * transformed_beam;
//            qDebug() << test_beam;

        test_beam = m_xzy * beam.on_point(intersection);
//            qDebug() << test_beam;

        transformed_beam.reflect();
        beam = m.transponed()*transformed_beam;

        // In complex modes there is no need to calculate full path of reflected beams
        if (ui->mode->currentIndex() != SINGLE_BEAM_CALCULATION && beam.cos_g() < 0) break;
    }
}

void MainWindow::transformation_on_exit(Beam& beam, const Beam& original_beam) {
    bool transformation_needed = beam.cos_g() >= 0 && (ui->glass->isChecked() || ui->ocular->isChecked());
    if (transformation_needed) {
        Point exit_intersection = cone->exit().intersection(beam);
        beam = Beam(exit_intersection, beam.d_x(), beam.d_y(), beam.d_z());
        beam = ui->ocular->isEnabled()
                ? ocular.refracted(beam)
                : cone->exit().refracted(beam, 1.5, 1);
        switch (ui->mode->currentIndex()) {
        case SINGLE_BEAM_CALCULATION:
            points.pop_back();
            points.push_back(exit_intersection);
            if (beam.d_z() < 0) {
                try {
                    reflection_cycle(beam, original_beam);
                } catch (beam_exception&) {
                    throw original_beam;
                }
                transformation_on_exit(beam, original_beam);
            } else points.push_back(cone->intersection(beam));
            break;
        case DIVERGENT_BUNDLE:
            if (points.back().z() > cone->length()) {
                points.back() = Plane(cone->length()).intersection(beam);
            }
            break;
        default:
            break;
        }
    }
}

MainWindow::BeamStatus MainWindow::calculate_single_beam_path(Beam& beam) {
    const auto original_beam = beam;
    points.push_back(beam.p1());

    // Perpendicular beams cause infinite loop in tubes
    if (!cone->is_conic() && qFabs(beam.d_y()) > 0.999999) {
        ui->statusbar->showMessage("Некорректный входной угол");
        return REFLECTED;
    }

    transformation_on_entrance(beam);
    try {
        reflection_cycle(beam, original_beam);
    } catch (beam_exception&) {
        throw original_beam;
    }
    transformation_on_exit(beam, original_beam);

    BeamStatus status;
    if (beam.d_z() < 0) {
        status = REFLECTED;
        // Cut the reflected beams' tails at the cone's entrance so the projections are cleaner
        if (ui->mode->currentIndex() == SINGLE_BEAM_CALCULATION) {
            points.back() = cone->entrance().intersection(beam);
        } else if (ui->mode->currentIndex() == PARALLEL_BUNDLE_EXIT) {
            points.pop_back();
        }
    } else {
        if (ui->mode->currentIndex() == PARALLEL_BUNDLE_EXIT) {
            points.back() = cone->exit().intersection(beam);
        }
        if (detector.missed(beam)) {
            status = MISSED;
        } else {
            status = detector.detected(beam) ? DETECTED : HIT;
            // Cut the passed beams' tails at the detectors's plane
            if (ui->mode->currentIndex() == SINGLE_BEAM_CALCULATION
                || (ui->mode->currentIndex() == DIVERGENT_BUNDLE && points.back().z() > cone->length())) {
                points.back() = detector.plane().intersection(beam);
            }
        }
    }
    return status;
}

QPair<int, int> MainWindow::calculate_parallel_beams(qreal angle) {
    int beams_total = 0;
    int beams_passed = 0;
    int count = ui->precision->currentIndex() ? 50 : 25;
    for (int i = 0; i < count; ++i) {
        qreal x = i * cone->r1() / count;
        for (int j = -count; j < count; ++j) {
            qreal y = j * cone->r1() / count;
            Point start = Point(-x, -y, 0);
            if (start.is_in_radius(cone->r1())) {
                Beam beam = Beam(start, angle);
                // The results are simmetrical relative to y axis, hence doubling total count for i > 0
                beams_total += (i > 0 ? 2 : 1);
                BeamStatus status = calculate_single_beam_path(beam);
                if (status == DETECTED) {
                    beams_passed += (i > 0 ? 2 : 1);
                }
                if (ui->mode->currentIndex() == PARALLEL_BUNDLE) {
                    statuses.push_back(status);
                    draw(points.back(), status, ui->rotation->value());
                    if (x > 0) {
                        draw(points.back().x_pair(), status, ui->rotation->value());
                    }
                } else if (ui->mode->currentIndex() == PARALLEL_BUNDLE_EXIT && status > REFLECTED) {
                    qreal beam_angle = beam.gamma();
                    beam_angles.push_back(beam_angle);
                    draw(points.back(), beam_angle, ui->rotation->value());
                    if (x > 0) {
                        draw(points.back().x_pair(), beam_angle, ui->rotation->value());
                    }
                }
            }
        }
    }
    return qMakePair(beams_passed, beams_total);
}

QPair<int, int> MainWindow::calculate_divergent_beams(const Point& start) {
    int beams_total = 0;
    int beams_passed = 0;
    int count = ui->precision->currentIndex() ? 10 : 5;
    int limit = abs(static_cast<int>(ui->angle->value() * count));
    for (int i = -limit; i <= limit; ++i) {
        qreal angle = static_cast<qreal>(i) / count;
        Beam beam = Beam(start, angle);
        ++beams_total;
        BeamStatus status = calculate_single_beam_path(beam);
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
            Point start = Point(-x, -y, 0);
            if (start.is_in_radius(cone->r1())) {
                QPair<int, int> current_result = calculate_divergent_beams(start);
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
        Point start = Point(x * cone->r1(), y * cone->r1(), 0);
        if (start.is_in_radius(cone->r1())) {
            Beam beam = Beam(start, (2 * rng.generateDouble() - 1) * fabs(ui->angle->value()));
            ++beams_total;
            BeamStatus status = calculate_single_beam_path(beam);
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
        int low_limit = iteration == 0 ? qCeil(cone->d1()) : qMax(optimal_value - (first_step - 1), qCeil(cone->d1()));
        int high_limit = iteration == 0 ? length_limit : qMin(optimal_value + (first_step - 1), length_limit);
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
            if (ui->lens->isChecked()) {
                qreal focus = lens_focus(ui->auto_focus->isChecked());
                lens.set_focus(focus);
            }
            if (ui->ocular->isChecked()) {
                ocular.set_position(length);
            }
            // Optimal length criterion №1: Acceptable loss value for parallel bundle at given angle
            if (loss(calculate_parallel_beams(ui->angle->value())) < loss_limit) {
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

QPair<qreal, qreal> MainWindow::optimal_d_out() {
    int count = 2; // considering step = 0.5
    int start = qFloor(ui->d_det->value() * count);
    int end = qCeil(ui->aperture->value()) * count;
    qreal max = 0;
    qreal optimal_value = 0;
    QPair<int, int> result;
    QPair<int, int> max_result;
    bool decrease_started = false;
    for (int i = start; i <= end && !decrease_started; ++i) {
        qreal d_out = static_cast<qreal>(i) / count;
        init_cone(cone->d1(), d_out, cone->length());
        if (loss(calculate_parallel_beams(0)) < loss_limit && loss(calculate_parallel_beams(ui->angle->value())) < loss_limit) {
            result = calculate_every_beam();
            int current_value = result.first;
            if (current_value > max) {
                max = current_value;
                optimal_value = d_out;
                max_result = result;
            } else if (optimal_value > 0) {
                decrease_started = true;
            }
            qDebug() << "(D_out)  " << "Length: " << cone->length() << " D_out: " << cone->d2() << " Beams: " << current_value;
        } else {
            qDebug() << "High loss value at " << d_out << " mm";
        }
    }
    return qMakePair(optimal_value, loss(max_result));
}

QPair<int, qreal> MainWindow::optimal_focus() {
    // The lower bound of focus length is determined by the f-number of the lens (k = f'/D_in >= 1).
    // The upper bound corresponds to forming a beam parallel to the axis on the edge of the lens
    // and is determined by the system's FOV (or input beam angle value) and cone's entrance diameter.
    // Further increasing focus length is totally possible but seems to be pointless in our case.
    int low_limit = qFloor(ui->focal_length->minimum());
    int high_limit = qMin(qCeil(ui->focal_length->maximum()), 500);
    int max = 0;
    int optimal_value = 0;
    QPair<int, int> result;
    QPair<int, int> max_result;
    for (int focus = low_limit; focus <= high_limit; ++focus) {
        lens.set_focus(focus);
        // Optimal length criterion №1: Acceptable loss value for parallel bundle at given angle
        if (loss(calculate_parallel_beams(ui->angle->value())) < loss_limit) {
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

MainWindow::Parameters MainWindow::full_optimisation() {
    int first_step = 5;
    int not_improving_length_limit = 100;
    int not_changing_limit = not_improving_length_limit / first_step;
    Parameters best_result;

    // The optimisation is done in 2 iterations with increasing accuracy
    for (int iteration = 0; iteration < 2; ) {
        int low_limit = iteration == 0 ? 2*qCeil(cone->d1()) : qMax(best_result.length - (first_step - 1), 2*qCeil(cone->d1()));
        int high_limit = iteration == 0 ? length_limit : qMin(best_result.length + (first_step - 1), length_limit);
        int step = iteration == 0 ? first_step : 1;
        int not_changing_count = 0;

        for (int i = low_limit; i <= high_limit && not_changing_count < not_changing_limit; i += step) {
            qreal length = static_cast<qreal>(i);
            cone->set_length(length);
            detector.set_position(length);
            if (ui->lens->isChecked()) {
                qreal focus = lens_focus(ui->auto_focus->isChecked());
                lens.set_focus(focus);
            }
            if (ui->ocular->isChecked()) {
                ocular.set_position(length);
            }
            auto result = optimal_d_out();
            qreal d_out = result.first;
            qreal current_loss_value = result.second;

            if (current_loss_value < best_result.loss) {
                qDebug() << "NEW RECORD";
                best_result = Parameters(i, d_out, current_loss_value);
                not_changing_count = 0;
            } else {
                ++not_changing_count;
            }
            qDebug() << "(Length) " << "Length: " << i << " D_out: " << d_out << " Loss: " << current_loss_value;
        }
        qDebug() << "(Best) " << "Length: " << best_result.length << " D_out: " << best_result.d_out << " Loss: " << best_result.loss;
        if (best_result.length > 0) {
            ++iteration;
        } else break;
    }
    return best_result;
}

MainWindow::Parameters MainWindow::complex_optimisation() {
    // This mode is too heavy to use in its current form so it's currently uneccessible from the UI.
    // The results obtained through tests suggest that the idea of optimising 3 parameters at once is really excessive anyway.
    // Optimising focon's length and exit diameter with autofocused lens works much faster and gives the same results.
    int focus_low_limit = qFloor(ui->focal_length->minimum());
    int focus_high_limit = qMin(qCeil(ui->focal_length->maximum()), 500);
    Parameters best_result;
    for (int focus = focus_low_limit; focus <= focus_high_limit; ++focus) {
        lens.set_focus(focus);
        auto result = full_optimisation();
        if (result.loss < best_result.loss) {
            qDebug() << "NEW RECORD";
            best_result = Parameters(focus, result);
        }
        qDebug() << "(Focus) " << "Length: " << best_result.length << " D_out: " << best_result.d_out << " F': " << focus << " Loss: " << best_result.loss;
    }
    return best_result;
}

qreal MainWindow::loss(const QPair<int, int>& result) {
    int beams_passed = result.first;
    int beams_total = result.second;
    return 10*qLn(static_cast<qreal>(beams_total)/beams_passed)/qLn(10);
}

qreal MainWindow::mean_exit_angle() const {
    qreal result = 0;
    for (const auto& angle : beam_angles) {
        result += angle;
    }
    return result/beam_angles.size();
}
