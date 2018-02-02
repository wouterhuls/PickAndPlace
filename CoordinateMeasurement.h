#ifndef PAP_COORDINATEMEASUREMENT_H
#define PAP_COORDINATEMEASUREMENT_H

#include "Coordinates.h"
#include <QObject>

namespace PAP
{
  class CoordinateMeasurement
  {
  public:
    MSCoordinates mscoordinates ;
    Coordinates2D globalcoordinates ;
    QString markername ;
  } ;
} ;

Q_DECLARE_METATYPE(PAP::CoordinateMeasurement) ;

#endif
