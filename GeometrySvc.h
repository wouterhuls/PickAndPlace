#ifndef PAP_GEOMETRYSVC_H
#define PAP_GEOMETRYSVC_H

#include "Singleton.h"
#include "NamedValue.h"
#include "Coordinates.h"
#include <QTransform>
#include <memory>

namespace PAP
{

  /* class that holds a total set of axis coordinates from the motion system */
  class TileStackPosition ;
  class ModulePosition ;
  
  class GeometrySvc : public PAP::Singleton<GeometrySvc>
  {
  public:
    // Definition of frame of references
    // - the 'global' frame. This is the frame in which the two axis of the motion system operate,
    
    // - the 'camera' frame: this has it's origin as the central pixel
    //   in the picture. It's origin in the global frame is given by
    //   the motion system. The frame can be rotated by an angle
    //   phi_cam with respect to the global frame.
    
    // - the 'module' frame is the frame of the module (jig,
    //   LHCb coordinates). This is the frame in which the coordinates of the
    //   fiducials are defined.

    GeometrySvc() ;
    virtual ~GeometrySvc() ;
    // these translate a point to a point. but I need more: for the
    // transforms I also need the rotations.
    Coordinates2D toGlobal( const MSMainCoordinates& c) const;
    Coordinates2D toGlobalDelta( const MSMainCoordinates& c) const;
    MSMainCoordinates toMSMainDelta( const Coordinates2D& c) const ;
    MSMainCoordinates toMSMain( const Coordinates2D& c) const ;
    
    QTransform fromCameraToGlobal() const ;
    QTransform fromModuleToGlobal( ViewDirection view ) const ;
    QTransform fromModuleToCamera( ViewDirection view ) const {
      return fromModuleToGlobal(view) * fromCameraToGlobal().inverted() ; 
    }
    QTransform fromModuleToGlobal() const {
      return fromModuleToGlobal(m_viewDirection.value()) ;
    }
    QTransform fromModuleToCamera() const {
      return fromModuleToCamera(m_viewDirection.value()) ;
    }
				  
    // update calibration
    void applyModuleDelta(ViewDirection dir, double dx, double dy, double dphi ) ;
    void applyModuleZCalibration(ViewDirection dir, double z, double dzdx, double dzdy ) ;
    std::array<double,3> getModuleZCalibration(ViewDirection dir) const ;

    // access to the camera z position in the module frame
    double moduleZ(ViewDirection dir ) const ;
    double moduleZ(ViewDirection view, double focus) const ;
    double moduleZ(ViewDirection view, double focus, MSMainCoordinates main ) const ;
    double moduleZ() const { return moduleZ(m_viewDirection) ; }
    double moduleZ(double focus) const { return moduleZ(m_viewDirection,focus) ; }
    double moduleZ(double focus, MSMainCoordinates main ) const { return moduleZ(m_viewDirection,focus,main); }
    
    
    // access to the rotation axis of the stack
    Coordinates2D stackAxisInGlobal() const ;
    Coordinates2D stackAxisInGlobal( const MSStackCoordinates& coord ) const ;
    QTransform fromStackToGlobal() const ;
    
    //signals:
    //void geometryChanged() ;

    void positionStackForTile( const QString& name ) const ;
    void applyStackDeltaForTile( const QString& name,
				 double dx, double dy, double dphi ) ;

    void updateMainAxisCalibration( double xA, double xB, double yA, double yB ) {
      m_mainXA = xA ; m_mainXB = xB; m_mainYB = yA; m_mainYB = yB ; }
    
    void updateStackCalibration( double X0, double XA, double XB,
				 double Y0, double YA, double YB, double Phi0) ;

    // Access the turn jig version
    enum TurnJigVersion { VersionA, VersionB } ;
    const auto& turnJigVersion() const { return m_turnJigVersion; }
    auto& turnJigVersion() { return m_turnJigVersion; }
    void setTurnJigVersion( int version ) { m_turnJigVersion = version ; }
    const auto& viewDirection() const { return m_viewDirection ; }
    void setViewDirection( ViewDirection dir ) { m_viewDirection.setValue(dir) ; }

  public:
    // access to various marker positions in the 'Module' frame. these
    // have already been corrected for the 'view'.
    std::vector<FiducialDefinition> velopixmarkersNSI() const ;
    std::vector<FiducialDefinition> velopixmarkersNLO() const ;
    std::vector<FiducialDefinition> velopixmarkersCLI() const ;
    std::vector<FiducialDefinition> velopixmarkersCSO() const ;
    std::vector<FiducialDefinition> velopixmarkersCLISensor() const ;
    std::vector<FiducialDefinition> velopixmarkersNSISensor() const ;
    std::vector<FiducialDefinition> velopixmarkersNSide() const ;
    std::vector<FiducialDefinition> velopixmarkersCSide() const ;
    std::vector<FiducialDefinition> velopixmarkers( ViewDirection view ) const {
      return view==NSideView ? velopixmarkersNSide() : velopixmarkersCSide()  ;
    }
    std::vector<FiducialDefinition> jigmarkers() const ;
    std::vector<FiducialDefinition> mcpointsNSide() const ;
    std::vector<FiducialDefinition> mcpointsCSide() const ;
    std::vector<FiducialDefinition> substratemarkers( ViewDirection view ) const {
      return view==NSideView ? mcpointsNSide() : mcpointsCSide() ; }

    const NamedDouble& stackX0() const { return m_stackX0 ; }
    const NamedDouble& stackXA() const { return m_stackXA ; }
    const NamedDouble& stackXB() const { return m_stackXB ; }
    const NamedDouble& stackY0() const { return m_stackY0 ; }
    const NamedDouble& stackYA() const { return m_stackYA ; }
    const NamedDouble& stackYB() const { return m_stackYB ; }
    const NamedDouble& stackPhi0() const { return m_stackPhi0 ; }

    const auto& resolutionXY() const { return m_resolutionXY ; }
    const auto& resolutionZ() const { return m_resolutionZ ; }
  private:
    // Some flags
    NamedInteger m_turnJigVersion{"Geo.TurnJigVersion",TurnJigVersion::VersionB} ;
    NamedValue<ViewDirection> m_viewDirection{"Geo.ViewDirection",ViewDirection::NSideView} ;
    
    // various calibration parameters
    NamedDouble m_mainX0 ;
    NamedDouble m_mainXA ;
    NamedDouble m_mainXB ;
    NamedDouble m_mainY0 ;
    NamedDouble m_mainYA ;
    NamedDouble m_mainYB ;
    NamedDouble m_cameraPhi ;
    std::unique_ptr<ModulePosition> m_moduleposition[2] ;

    // parameters that translate stack parameters into global position of stack rotation axis
    NamedDouble m_stackX0 ;
    NamedDouble m_stackXA ;
    NamedDouble m_stackXB ;
    NamedDouble m_stackY0 ;
    NamedDouble m_stackYA ;
    NamedDouble m_stackYB ;
    NamedDouble m_stackPhi0 ;

  // paramaters for all the stacks. we better tabulate these end access by name
    std::map<QString,TileStackPosition*> m_tileStackPositions ;

    // Coordinate resolution
    NamedDouble m_resolutionXY{"Geo.ResolutionXY",0.002} ;
    NamedDouble m_resolutionZ{"Geo.ResolutionZ",0.002} ;
        
  } ;


}

#endif
