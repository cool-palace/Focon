#include "..\include\mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::save_settings() {
    QJsonObject json_file = {
                              {"D1", ui->d_in->value()},
                              {"D2", ui->d_out->value()},
                              {"Length", ui->length->value()},
                              {"Angle", ui->angle->value()},
                              {"X offset", ui->offset->value()},
                              {"Y offset", ui->height->value()},
                              {"Detector's window", ui->aperture->value()},
                              {"Detector's offset", ui->offset_det->value()},
                              {"Detector's FOV", ui->fov->value()},
                              {"Detector's diameter", ui->d_det->value()},
                              {"Mode", ui->mode->currentIndex()},
                              {"Rotation", ui->rotation->value()},
                              {"Lens", ui->lens->isChecked()},
                              {"Focal length", ui->focal_length->value()},
                              {"Auto focus", ui->auto_focus->isChecked()},
                              {"Defocus", ui->defocus->value()},
                              {"Ocular", ui->ocular->isChecked()},
                              {"Ocular focal length", ui->ocular_focal_length->value()},
                              {"Glass", ui->glass->isChecked()},
                              {"Cavity length", ui->cavity_length->value()},
                              {"Precision", ui->precision->currentIndex()}
                            };
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить файл"),
                                                    QCoreApplication::applicationDirPath() + "//untitled.foc",
                                                    tr("Файлы настроек (*.foc)"));
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
                                                    "Файлы настроек (*.foc)");
    QFile file;
    file.setFileName(filepath);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString val = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject json_file = doc.object();

    if (json_file.contains("D1")) {
        ui->d_in->setValue(json_file.value("D1").toDouble());
    }
    if (json_file.contains("D2")) {
        ui->d_out->setValue(json_file.value("D2").toDouble());
    }
    if (json_file.contains("Length")) {
        ui->length->setValue(json_file.value("Length").toDouble());
    }
    if (json_file.contains("Angle")) {
        ui->angle->setValue(json_file.value("Angle").toDouble());
    }
    if (json_file.contains("X offset")) {
        ui->offset->setValue(json_file.value("X offset").toDouble());
    }
    if (json_file.contains("Y offset")) {
        ui->height->setValue(json_file.value("Y offset").toDouble());
    }
    if (json_file.contains("Detector's window")) {
        ui->aperture->setValue(json_file.value("Detector's window").toDouble());
    }
    if (json_file.contains("Detector's offset")) {
        ui->offset_det->setValue(json_file.value("Detector's offset").toDouble());
    }
    if (json_file.contains("Detector's FOV")) {
        ui->fov->setValue(json_file.value("Detector's FOV").toDouble());
    }
    if (json_file.contains("Detector's diameter")) {
        ui->d_det->setValue(json_file.value("Detector's diameter").toDouble());
    }
    if (json_file.contains("Rotation")) {
        ui->rotation->setValue(json_file.value("Rotation").toInt());
    }
    if (json_file.contains("Mode")) {
        ui->mode->setCurrentIndex(json_file.value("Mode").toInt());
    }
    if (json_file.contains("Lens")) {
        ui->lens->setChecked(json_file.value("Lens").toBool());
    }
    if (json_file.contains("Focal length")) {
        ui->focal_length->setValue(json_file.value("Focal length").toDouble());
    }
    if (json_file.contains("Auto focus")) {
        ui->auto_focus->setChecked(json_file.value("Auto focus").toBool());
    }
    if (json_file.contains("Defocus")) {
        ui->defocus->setValue(json_file.value("Defocus").toInt());
    }
    if (json_file.contains("Ocular")) {
        ui->ocular->setChecked(json_file.value("Ocular").toBool());
    }
    if (json_file.contains("Ocular focal length")) {
        ui->ocular_focal_length->setValue(json_file.value("Ocular focal length").toDouble());
    }

    bool glass_on = json_file.value("Glass").toBool();
    ui->glass->setChecked(glass_on);
    if (glass_on && json_file.contains("Cavity length"))
        ui->cavity_length->setValue(json_file.value("Cavity length").toDouble());

    if (json_file.contains("Precision")) {
        ui->precision->setCurrentIndex(json_file.value("Precision").toInt());
    }
    if (json_file.contains("Defocusing")) { // This subfunction provides backwards compatibility with older save files
        auto def = json_file.value("Defocusing").toString();
        ui->defocus->setValue(def == "plus" ? 1 : def == "minus" ? -1 : 0);
    }
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

void MainWindow::save_image_xoy() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить изображение"),
                                                    QCoreApplication::applicationDirPath(),
                                                    tr("PNG-изображение (*.png)"));
    if (!fileName.isNull()) {
        auto point = ui->view->mapFromScene(QPointF(x_axis_xoy->line().x1(), y_axis_xoy->line().y1()));
        auto end = ui->view->mapFromScene(QPointF(x_axis_xoy->line().x2(), y_axis_xoy->line().y2()));
        auto distance = end - point;
        auto size = QSize(distance.x(), distance.y());
        QPixmap pixMap = ui->view->grab(QRect(point,size) += QMargins(10,10,10,10));
        pixMap.save(fileName);
        ui->statusbar->showMessage("Изображение сохранено: " + fileName);
    }
}
