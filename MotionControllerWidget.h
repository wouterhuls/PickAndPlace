#ifndef MOTIONCONTROLLERWIDGET_H
#define MOTIONCONTROLLERWIDGET_H

#include <QWidget>

namespace Ui {
class MotionControllerWidget;
}

class MotionControllerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MotionControllerWidget(QWidget *parent = 0);
    ~MotionControllerWidget();

private:
    Ui::MotionControllerWidget *ui;
};

#endif // MOTIONCONTROLLERWIDGET_H
