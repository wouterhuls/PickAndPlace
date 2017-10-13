#include "MotionAxisPositionWidget.h"
#include "MotionControllerWidget.h"
#include "MotionSystemSvc.h"
#include <QTimer>

MotionAxisPositionWidget::MotionAxisPositionWidget(MotionControllerWidget *parent)
    : QLCDNumber(parent)
{
    if(parent) m_axisid = parent->axisId() ;

    setSegmentStyle(Filled);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(showPosition()));
    timer->start(100000);

    showPosition();

}

void MotionAxisPositionWidget::showPosition()
{
    double pos = MotionSystemSvc::instance()->position( m_axisid ) ;
    display(pos) ;
}
