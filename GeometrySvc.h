#ifndef PAP_GEOMETRYSVC_H
#define PAP_GEOMETRYSVC_H

#include "Singleton.h"
#include "NamedValue.h"
#include "Coordinates.h"
#include <QTransform>

namespace PAP
{

  /* class that holds a total set of axis coordinates from the motion system */
  

  class GeometrySvc : public PAP::Singleton<GeometrySvc>
  {
    public:
    enum ViewDirection { CSideView=0, NSideView=1 } ;
  public:
    // - the 'global' frame is the frame of the cameraview
    // - the 'MS' frame has coordinates of the main stage X and Y axis
    // - the 'module' frame is the frame of the module (jig)
    
    GeometrySvc() ;
    // these translate a point to a point. but I need more: for the
    // transforms I also need the rotations.
    Coordinates2D toGlobal( const MSMainCoordinates& c) const;
    Coordinates2D toGlobalDelta( const MSMainCoordinates& c) const;
    MSMainCoordinates toMSMainDelta( const Coordinates2D& c) const ;

    QTransform computeMSToGlobal() const {
      // this needs to take into account the angles of the camera
      // system with the motion system arms. we also need a definition
      // of the origin. it may be useful if this corresponds to the
      // rotation axis of the small stack, for certain values of small
      // stack X and Y. but then it may be better if it is the nominal
      // origin
      return QTransform{} ;
    }
    
    QTransform computeGlobalToModule( int view ) ;
    
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
