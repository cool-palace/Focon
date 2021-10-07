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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QDoubleSpinBox * d_in;
    QDoubleSpinBox * d_out;
    QDoubleSpinBox * length;
    QDoubleSpinBox * angle;
    QPushButton *calc;

    QGraphicsScene* scene;
    QGraphicsView* view;
    QGraphicsLineItem * y_axis;
    QGraphicsLineItem * z_axis;

public slots:
    void move();

};
#endif // MAINWINDOW_H
