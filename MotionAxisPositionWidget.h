#ifndef MOTIONAXISPOSITIONWIDGET_H
#define MOTIONAXISPOSITIONWIDGET_H

#include <QLCDNumber>
#include "MotionAxisParameters.h"

class MotionControllerWidget ;

class MotionAxisPositionWidget : public QLCDNumber
{
    Q_OBJECT
public:
    explicit MotionAxisPositionWidget(MotionControllerWidget *parent = 0);

signals:

private slots:
    void showPosition() ;

private:
    MotionAxisID m_axisid ;
};

#endif // MOTIONAXISPOSITIONWIDGET_H
