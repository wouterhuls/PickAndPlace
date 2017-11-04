#include "GeometrySvc.h"
#include "PropertySvc.h"
#include "NominalMarkers.h"

namespace PAP
{

  // functions to implement:
  // * get all fiducial markers for a particular chip/side
  //   - by name (or by enum?)
  //   - need LHCb position
  //   - need to translate to position that corrects for 'parity'
  //   - need position in camera frame
  // * isn't that all that I need?
  //
  // Then I need to implement a 'MarkerMeasurement'
  //  - position of all stages
  //  - reference marker (or reference position)
  
  // This needs to be followed by some sort of routine that computes
  // updated calibration constants. So this 'measurementsvc' is
  // actually quite complicated.

  // For the autofocus I need a simple way to compute a 'contrast'. I
  // can either take several 'focus points', or a 'focus area' in the
  // middle of the picture. Let's start with the latter and see how
  // quick it is.
  
  
  GeometrySvc::GeometrySvc()
    : m_mainX0( "Geo.mainX0", 0. ),
      m_mainXA( "Geo.mainXA", 1.0 ),
      m_mainXB( "Geo.mainXB", 0.0 ),
      m_mainY0( "Geo.mainY0", 0. ),
      m_mainYA( "Geo.mainYA", 0.0 ),
      m_mainYB( "Geo.mainYB", -1.0 )
  {
    PAP::PropertySvc::instance()->add( m_mainX0 ) ;
    PAP::PropertySvc::instance()->add( m_mainXA ) ;
    PAP::PropertySvc::instance()->add( m_mainXB ) ;
    PAP::PropertySvc::instance()->add( m_mainY0 ) ;
    PAP::PropertySvc::instance()->add( m_mainYA ) ;
    PAP::PropertySvc::instance()->add( m_mainYB ) ;
  }
  
  Coordinates2D GeometrySvc::toGlobal( const MSMainCoordinates& c) const
  {
    return Coordinates2D{
      m_mainX0 + m_mainXA * c.x + m_mainXB * c.y,
	m_mainY0 + m_mainYA * c.x + m_mainYB * c.y } ; 
  } ;

  Coordinates2D GeometrySvc::toGlobalDelta( const MSMainCoordinates& c) const
  {
    return Coordinates2D{
      m_mainXA * c.x + m_mainXB * c.y,
	m_mainYA * c.x + m_mainYB * c.y } ; 
  } ;
  
  MSMainCoordinates GeometrySvc::toMSMainDelta( const Coordinates2D& c) const
  {
    // now we need the inverse of the matrix
    double D = m_mainYB*m_mainXA - m_mainXB * m_mainYA ;
    double dx = (  m_mainYB * c.x - m_mainYA * c.y)/D ;
    double dy = ( -m_mainXB * c.x + m_mainXA * c.y)/D ;
    return MSMainCoordinates{ dx, dy } ;
  }

  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersNSI() { return Markers::velopixNSI() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersNLO() { return Markers::velopixNLO() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersCLI() { return Markers::velopixCLI() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersCSO() { return Markers::velopixCSO() ; }

  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersNSide()
  {
    auto rc = velopixmarkersNSI() ;
    auto rc2 = velopixmarkersNLO() ;
    rc.insert(rc.end(),rc2.begin(),rc2.end()) ;
    return rc ;
  }

  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersCSide()
  {
    auto rc = velopixmarkersCLI() ;
    auto rc2 = velopixmarkersCSO() ;
    rc.insert(rc.end(),rc2.begin(),rc2.end()) ;
    return rc ;
  }
  
  std::vector<FiducialDefinition>
  GeometrySvc::jigmarkers() { return Markers::jigNCSide() ; }
  
}
