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
    // Definition of frame of references
    // - the 'global' frame. This is the frame in which the two axis of the motion system operate,

    
    // - the 'camera' frame: this has it's origin as the central pixel
    //   in the picture. It's origin in the global frame is given by
    //   the motion system. The frame can be rotated by an angle
    //   phi_cam with respect to the global frame.
    
    // - the 'global' frame is the frame of the cameraview
    // - the 'MS' frame has coordinates of the main stage X and Y axis
    // - the 'module' frame is the frame of the module (jig)
    
    GeometrySvc() ;
    virtual ~GeometrySvc() ;
    // these translate a point to a point. but I need more: for the
    // transforms I also need the rotations.
    Coordinates2D toGlobal( const MSMainCoordinates& c) const;
    Coordinates2D toGlobalDelta( const MSMainCoordinates& c) const;
    MSMainCoordinates toMSMainDelta( const Coordinates2D& c) const ;
    
    QTransform fromCameraToGlobal() const ;
    QTransform fromModuleToGlobal( ViewDirection view ) const ;

    // update calibration
    void applyModuleDelta( double dx, double dy, double phi ) ;
    
    Coordinates2D stackAxisInGlobal( const MSStackCoordinates& coord ) const ;
    QTransform fromStackToGlobal() const ;
    
    //signals:
    //void geometryChanged() ;
    
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
    NamedDouble m_cameraPhi ;
    NamedDouble m_modulePhi ;
    NamedDouble m_moduleX ;
    NamedDouble m_moduleY ;

    // parameters that translate stack parameters into global position of stack rotation axis
    NamedDouble m_stackX0 ;
    NamedDouble m_stackXA ;
    NamedDouble m_stackXB ;
    NamedDouble m_stackY0 ;
    NamedDouble m_stackYA ;
    NamedDouble m_stackYB ;
    NamedDouble m_stackPhi0 ;
    
  } ;


}

#endif
