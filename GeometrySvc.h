#ifndef PAP_GEOMETRYSVC_H
#define PAP_GEOMETRYSVC_H

#include "Singleton.h"
#include "NamedValue.h"

namespace PAP
{

  /* class that holds a total set of axis coordinates from the motion system */
  struct Coordinates2D
  {
    Coordinates2D( double _x, double _y ) : x(_x),y(_y) {}
    double x ;
    double y ;
  } ;
  
  struct MSMainCoordinates
  {
    double x ;
    double y ;
  } ;

  struct MSStackCoordinates
  {
    double x ;
    double y ;
    double phi ;
  } ;
  
  struct MSCoordinates
  {
    MSMainCoordinates main ;
    MSStackCoordinates stack ;
  } ;

  class GeometrySvc : public PAP::Singleton<GeometrySvc>
  {
  public:
    GeometrySvc() ;
    Coordinates2D toGlobal( const MSMainCoordinates& c) const;
  private:
    // various calibration parameters
    NamedDouble m_mainX0 ;
    NamedDouble m_mainXA ;
    NamedDouble m_mainXB ;
    NamedDouble m_mainY0 ;
    NamedDouble m_mainYA ;
    NamedDouble m_mainYB ;
  } ;


}

#endif
