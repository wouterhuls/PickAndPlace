#include "GeometrySvc.h"
#include "PropertySvc.h"

namespace PAP
{
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
}
