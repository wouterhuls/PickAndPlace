#include "GeometrySvc.h"
#include "PropertySvc.h"

namespace PAP
{
  struct VelopixFiducialDefinition
  {
    VelopixFiducialDefinition( QString _name, double _x, double _y)
      : name(_name),x(_x),y(_y) {}
    QString name ;
    double x ;
    double y ;
  } ;

  std::vector<VelopixFiducialDefinition> markers()
  {
    std::vector<VelopixFiducialDefinition>
      defs{
      // CLI CS VP0
      {"CLI_VP00_Fid1",-11.069,41.433},
      {"CLI_VP00_Fid2", -1.152,31.516},
      {"CLI_VP01_Fid1", -0.997,31.361},
      {"CLI_VP01_Fid2",  8.920,21.444},
      {"CLI_VP02_Fid1",  9.076,21.288},
      {"CLI_VP02_Fid2", 18.993,11.371},
      // CSO VP3
      {"CSO_VP30_Fid1", 51.273,  1.464},
      {"CSO_VP30_Fid2", 41.355, -8.454},
      {"CSO_VP31_Fid1", 41.200, -8.609},
      {"CSO_VP31_Fid2", 31.283,-18.526},
      {"CSO_VP32_Fid1", 31.127,-18.682},
      {"CSO_VP32_Fid2", 21.210,-28.599},
      // NSI
      {"NSI_VP20_Fid1", 11.487,-18.676},
      {"NSI_VP20_Fid2", 21.404, -8.959},
      {"NSI_VP21_Fid1", 21.560, -8.804},
      {"NSI_VP21_Fid2", 31.477,  1.113},
      {"NSI_VP22_Fid1", 31.633,  1.269},
      {"NSI_VP22_Fid2", 41.550, 11.186},
      // NLO
      {"NLO_VP10_Fid1", 27.744, 22.066},
      {"NLO_VP10_Fid2", 17.826, 31.983},
      {"NLO_VP11_Fid1", 17.671, 32.138},
      {"NLO_VP11_Fid2",  7.754, 42.055},
      {"NLO_VP12_Fid1",  7.598, 42.211},
      {"NLO_VP12_Fid2", -2.319, 52.128}
    } ;
    return defs ;
  }

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
    : m_mainX0( "Geo.mainX0", 10. ),
      m_mainXA( "Geo.mainXA", 1.0 ),
      m_mainXB( "Geo.mainXB", 0.0 ),
      m_mainY0( "Geo.mainY0", 20. ),
      m_mainYA( "Geo.mainYA", 0.0 ),
      m_mainYB( "Geo.mainYB", 1.0 )
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
}
