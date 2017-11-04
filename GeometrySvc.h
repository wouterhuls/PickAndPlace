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
    MSMainCoordinates( double _x, double _y ) : x(_x),y(_y) {}
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

  struct FiducialDefinition
  {
    FiducialDefinition( QString _name, double _x, double _y)
    : name(_name),x(_x),y(_y) {}
    QString name ;
    double x ;
    double y ;
  } ;

  class GeometrySvc : public PAP::Singleton<GeometrySvc>
  {
  public:
    GeometrySvc() ;
    Coordinates2D toGlobal( const MSMainCoordinates& c) const;
    Coordinates2D toGlobalDelta( const MSMainCoordinates& c) const;
    MSMainCoordinates toMSMainDelta( const Coordinates2D& c) const ;

  public:
    // access to various marker positions in the 'Module' frame. these
    // have already been corrected for the 'view'.
    std::vector<FiducialDefinition> velopixmarkersNSI() ;
    std::vector<FiducialDefinition> velopixmarkersNLO() ;
    std::vector<FiducialDefinition> velopixmarkersCLI() ;
    std::vector<FiducialDefinition> velopixmarkersCSO() ;
    std::vector<FiducialDefinition> velopixmarkersNSide() ;
    std::vector<FiducialDefinition> velopixmarkersCSide() ;
    std::vector<FiducialDefinition> jigmarkers() ;

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
