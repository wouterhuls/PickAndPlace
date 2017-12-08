#ifndef __MOTIONSYSTEMCALIBRATION_H__
#define __MOTIONSYSTEMCALIBRATION_H__

#include <QDialog>

namespace PAP
{
  class GeometrySvc ;
  
  class MotionSystemCalibration : public QDialog
  {
  public:
    MotionSystemCalibration( QWidget* parent=0 ) ;
  } ;
}

#endif
