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
    enum Status { Uninitialized, Ready, Failed } ;
  public:
    QString m_name ;
    // These coordinates in the GLOBAL LHCb frame? That's most logical
    // but perhaps not the most convenient?
    double m_x ;
    double m_y ;
    double m_z ;
    Status m_status ;
  public:
  ReportCoordinate( const FiducialDefinition& def, double z )
      : m_name{def.name}, m_x{def.x}, m_y{def.y}, m_z{z}, m_status{Uninitialized} {}
  ReportCoordinate( const QString& name, double x, double y, double z )
      : m_name{name}, m_x{x}, m_y{y}, m_z{z}, m_status{Uninitialized} {}
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
