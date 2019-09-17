#ifndef METROLOGYREPORT_H
#define METROLOGYREPORT_H

/***************************************************************/
/* We somehow need to disinguish between the actual measurements that
   we do (marker positions, z-position of sensors, z-position of
   substrate) and the parameters that we derive (tile alignment
   parameters, glue layer thickness. For now I put those in one place,
   then make a separate 'analyse' funciton that computes the derived
   parameters from the others.
*/
/***************************************************************/

#include "Coordinates.h"
#include <QString>
#include <QTextEdit>

namespace PAP
{
  // FIXME: we already have a class
  // 'CoordinateMeasurement'. cannot we merge those? or at least make
  // the names a bit more descriptive?
  class ReportCoordinate
  {
  public:
    enum Status { Uninitialized, Initialized, Ready, Failed } ;
  private:
    QString m_name{"Unknown"} ;
    // These coordinates in the GLOBAL LHCb frame? That's most logical
    // but perhaps not the most convenient?
    Coordinates3D m_position ;
    Status m_status{Uninitialized} ;
  public:
    ReportCoordinate( const FiducialDefinition& def, float z )
      : m_name{def.name}, m_position{static_cast<float>(def.x),static_cast<float>(def.y),z}, m_status{Initialized} {}
    ReportCoordinate( const QString& name, const Coordinates3D& x)
      : m_name{name}, m_position{x}, m_status{Initialized} {}
    ReportCoordinate( const QString& name, float x, float y, float z, Status s = Initialized )
      : m_name{name}, m_position{x,y,z}, m_status{s} {}
    ReportCoordinate() = default ;
    const QString& name() const { return m_name ; }
    double x() const { return m_position.x() ;}
    double y() const { return m_position.y() ;}
    double z() const { return m_position.z() ;}
    void setPosition( float x, float y, float z ) { m_position = Coordinates3D{x,y,z} ; }
    void setStatus( Status s ) { m_status = s; }
    const Coordinates3D& position() const { return m_position ; }
    Status status() const { return m_status ; }
    ReportCoordinate& operator=( const Coordinates3D& x ) { m_position=x ; return *this ; }
  } ;

  /*
  struct TileMetrologyReport
  {
    std::vector<ReportCoordinate> markers{2} ;
    std::vector<ReportCoordinate> sensorsurface{4} ;
  } ;

  struct SideMetrologyReport
  {
    TileMetrologyReport LTile ;
    TileMetrologyReport STile ;
    std::vector<ReportCoordinate> substrate ;
  } ;
  
  class MetrologyReport
  {
  public:
    SideMetrologyReport sides[2] ;
    QString   modulename ;
    QTextEdit comments ;
    // void writeToFile
    // void analyse 
    
    
  } ;
  */
}

#endif
