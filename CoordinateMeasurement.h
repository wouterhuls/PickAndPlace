#ifndef PAP_COORDINATEMEASUREMENT_H
#define PAP_COORDINATEMEASUREMENT_H

#include "Coordinates.h"

namespace PAP
{
  struct CoordinateMeasurement
  {
    MSCoordinates mscoordinates ;
    Coordinates2D globalcoordinates ;
  } ;
} ;

Q_DECLARE_METATYPE(PAP::CoordinateMeasurement) ;

#endif
