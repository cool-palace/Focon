#include "mainwindow.h"
#include "ui_mainwindow.h"


void MainWindow::init_objects() {
    // updating geometrical objects and detector
    start = Point(-ui->offset->value(), -ui->height->value(), 0);
    if (cone != nullptr) delete cone;
    cone = (qFabs(ui->d_in->value() - ui->d_out->value()) > 1e-6
                ? new Cone(ui->d_in->value(), ui->d_out->value(), ui->length->value())
                : new Tube(ui->d_in->value(), ui->length->value()));
    beam = Beam(start, ui->angle->value());
    detector = Detector(ui->aperture->value(), ui->length->value(), ui->offset_det->value(),
                        ui->fov->value(), ui->d_det->value());
    init_lens(detector.detector_z(), ui->defocus_minus->isChecked(), ui->defocus_plus->isChecked());

}

void MainWindow::init_lens(qreal distance, bool less, bool more) {
    int l = static_cast<int>(less);
    int m = static_cast<int>(more);
    lens.set_focus(distance * (cone->r1()/(cone->r1() + detector.r() * (l - m))));
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
    while(true) {
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
        if (intersection.z() < 0 || intersection.z() > cone->length() /*|| i > 60 */ ) break;

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

QPair<int, int> MainWindow::calculate_parallel_beams() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = /* cone->length() / cone->d2() >= 100 ? 20 : */ 50;
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
    int count = 4;
    int limit = abs(static_cast<int>(ui->angle->value() * count));
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
    return qMakePair(beams_passed, beams_total);
}

QPair<int, int> MainWindow::calculate_every_beam() {
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
    return qMakePair(beams_passed, beams_total);
}

QPair<int, int> MainWindow::monte_carlo_method() {
    int beams_total = 0;
    int beams_passed = 0;
    int count = 100000;
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

qreal MainWindow::optimal_length_cycle(int& max, int& optimal_value) {
    // For the first iteration the max and optimal value are initilized by zero
    int low_limit = optimal_value == 0 ? 2*qCeil(cone->d1()) : optimal_value - 4;
    int high_limit = optimal_value == 0 ? length_limit : optimal_value + 4;
    int step = optimal_value == 0 ? 5 : 1;
    QPair<int, int> result;
    QPair<int, int> max_result;
    int not_changing_count = 0;
    // Conditions for breaking the cycle:
    // 1. Length value is out of range
    // 2. It is the first iteration and the results did not improve for 150 mm (the value is arbitrary)
    // Increasing cone's length leads to increasing both the computation time and the loss value for non-zero beam bundles
    // so it's reasonable to cut the calculations short when the results become predictable
    for (int i = low_limit; i <= high_limit && (step == 1 || not_changing_count < 30); i += step) {
        qreal length = static_cast<qreal>(i);
        cone->set_length(length);
        detector.set_position(length);
        init_lens(detector.detector_z(), ui->defocus_minus->isChecked(), ui->defocus_plus->isChecked());
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
    return loss(max_result);
}

QPair<int, qreal> MainWindow::optimal_length() {
    int max = 0;
    int optimal_value = 0;

    // First iteration gives the approximate optimal value within the accuracy of ±9
    optimal_length_cycle(max, optimal_value);
    // If it fails, return
    if (max == 0) return qMakePair(length_limit, loss_limit);

    // If it doesn't, then the second gives the optimal integer value
    qreal loss = optimal_length_cycle(max, optimal_value);
    return qMakePair(optimal_value, loss);
}

qreal MainWindow::loss(QPair<int, int> result) {
    int beams_passed = result.first;
    int beams_total = result.second;
    return 10*qLn(static_cast<qreal>(beams_total)/beams_passed)/qLn(10);
}

