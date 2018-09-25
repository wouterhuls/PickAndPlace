#include "GeometrySvc.h"
#include "PropertySvc.h"
#include "MotionSystemSvc.h"
#include "NominalMarkers.h"

#include <Eigen/Dense>
#include <cmath>
#include <memory>

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
  
  class TileStackPosition
  {
  private:
    NamedDouble m_stackX ;
    NamedDouble m_stackY ;
    NamedDouble m_stackPhi ;
  public:
    TileStackPosition( const QString& name, double x, double y, double phi )
      :
      m_stackX{QString{"Geo."} + name + "StackX",x},
      m_stackY{QString{"Geo."} + name + "StackY",y},
      m_stackPhi{QString{"Geo."} + name + "StackPhi",phi}
    {
      PAP::PropertySvc* papsvc = PAP::PropertySvc::instance() ;
      papsvc->add( m_stackX ) ;
      papsvc->add( m_stackY ) ;
      papsvc->add( m_stackPhi ) ;
    }
    double x() const { return m_stackX ; }
    double y() const { return m_stackY ; }
    double phi() const { return m_stackPhi ; }
    void setX(double x) { m_stackX = x ; }
    void setY(double y) { m_stackY = y ; }
    void setPhi(double r) { m_stackPhi = r ; }
  } ;

  class ModulePosition
  {
  private:
    NamedDouble m_x;
    NamedDouble m_y ;
    NamedDouble m_phi ;
    NamedDouble m_z ;
    NamedDouble m_dzdx ;
    NamedDouble m_dzdy ;
  public:
    ModulePosition( const QString& side, double x, double y, double phi, double z=0 )
      : m_x{QString{"Geo.ModuleX"}+side,x},
	m_y{QString{"Geo.ModuleY"}+side,y},
	m_phi{QString{"Geo.ModulePhi"}+side,phi},
	m_z{QString{"Geo.ModuleZ"}+side,z},
	m_dzdx{QString{"Geo.ModuledZdX"}+side,0},
	m_dzdy{QString{"Geo.ModuledZdY"}+side,0}
    {
      PAP::PropertySvc* papsvc = PAP::PropertySvc::instance() ;
      papsvc->add( m_x ) ;
      papsvc->add( m_y ) ;
      papsvc->add( m_phi ) ;
      papsvc->add( m_z ) ;
      papsvc->add( m_dzdx ) ;
      papsvc->add( m_dzdy ) ;
    }
    void applyDelta(double dx, double dy, double phi ) {
      QTransform T ;
      T.translate( m_x, m_y ) ;
      T.rotateRadians( m_phi ) ;
      QTransform dT ;
      dT.translate( dx, dy) ;
      dT.rotateRadians( phi ) ;
      QTransform Tnew = T*dT ;
      m_phi = std::atan2( Tnew.m12(), Tnew.m11() ) ;
      m_x = Tnew.m31() ;
      m_y = Tnew.m32() ;
    }
    const NamedDouble& phi() const { return m_phi ; }
    const NamedDouble& x() const { return m_x ; }
    const NamedDouble& y() const { return m_y ; }
    const NamedDouble& z() const { return m_z ; }
    double z(const Coordinates2D& globalpos) const {
      return m_z + globalpos.x()*m_dzdx + globalpos.y()*m_dzdy ; }
    void setZ( double z, double dzdx=0, double dzdy=0 ) {
      m_z = z ;
      m_dzdx = dzdx ;
      m_dzdy = dzdy ;
    }
  } ;
  
  GeometrySvc::GeometrySvc()
    : m_mainX0( "Geo.mainX0", 0. ),
      m_mainXA( "Geo.mainXA", 1.0 ),
      m_mainXB( "Geo.mainXB", 0.0 ),
      m_mainY0( "Geo.mainY0", 0. ),
      m_mainYA( "Geo.mainYA", 0.0 ),
      m_mainYB( "Geo.mainYB", -1.0 ),
      m_cameraPhi( "Geo.cameraPhi", 0 ),
      // these points define the position and orientation of the stack rotation axis in the global frame
      m_stackX0( "Geo.stackX0", -15.054 ),
      m_stackXA( "Geo.stackXA", -1.0 ),
      m_stackXB( "Geo.stackXB", 0.00059 ),
      m_stackY0( "Geo.stackY0", -40.356 ),
      m_stackYA( "Geo.stackYA", -0.012 ),
      m_stackYB( "Geo.stackYB", -1.0 ),
      m_stackPhi0( "Geo.stackPhi0", 0.496 )
  {
    PAP::PropertySvc::instance()->add( m_mainX0 ) ;
    PAP::PropertySvc::instance()->add( m_mainXA ) ;
    PAP::PropertySvc::instance()->add( m_mainXB ) ;
    PAP::PropertySvc::instance()->add( m_mainY0 ) ;
    PAP::PropertySvc::instance()->add( m_mainYA ) ;
    PAP::PropertySvc::instance()->add( m_mainYB ) ;
    PAP::PropertySvc::instance()->add( m_cameraPhi ) ;
    m_moduleposition[ViewDirection::NSideView] = std::unique_ptr<ModulePosition>(new ModulePosition{"NSide",-32.47, -63.69, -0.0005975, 24.0}) ;
    m_moduleposition[ViewDirection::CSideView] = std::unique_ptr<ModulePosition>(new ModulePosition{"CSide",-32.47, -63.69, -0.0005975, 24.0}) ;
    
    // stack
    PAP::PropertySvc::instance()->add( m_stackX0 ) ;
    PAP::PropertySvc::instance()->add( m_stackXA ) ;
    PAP::PropertySvc::instance()->add( m_stackXB ) ;
    PAP::PropertySvc::instance()->add( m_stackY0 ) ;
    PAP::PropertySvc::instance()->add( m_stackYA ) ;
    PAP::PropertySvc::instance()->add( m_stackYB ) ;
    PAP::PropertySvc::instance()->add( m_stackPhi0 ) ;

    // these are the stack parameters for the CSI chip. not yet
    // exactly clear what I mean by that, but it works, for now
    m_tileStackPositions["CLI"] = new TileStackPosition{"CLI",13.176,20.209,0.27377} ;
    m_tileStackPositions["CSO"] = new TileStackPosition{"CSO",13.504,12.179,-1.3030} ;
    m_tileStackPositions["NSI"] = new TileStackPosition{"NSI",3.657,10.136, 0.0010} ;
    m_tileStackPositions["NLO"] = new TileStackPosition{"NLO",24.554,1.687,-1.57576} ;

    // focus values:
    // - main marker jigs C-side: 14.405
    // - main marker jigs N-side: 15.846
    // - fiducials: 14.275 (for the mechanical asic: not sure if this is thinned)
    // - fiducials for Raphael's pieces: 14.77
    //
  }

  GeometrySvc::~GeometrySvc() {}

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
    double dx = (  m_mainYB * c.x() - m_mainXB * c.y())/D ;
    double dy = ( -m_mainYA * c.x() + m_mainXA * c.y())/D ;
    
    // Eigen::Matrix<double,2,2> A ;
    // A << m_mainXA.value(), m_mainXB.value(),m_mainYA.value(),m_mainYB.value() ;
    // auto B = A.inverse() ;
    // dx = B(0,0)*c.x() + B(0,1)*c.y() ;
    // dy = B(1,0)*c.x() + B(1,1)*c.y() ;
    // qDebug() << "B00: " << B(0,0) << " " << m_mainYB.value() / D << "\n"
    // 	     << "B01: " << B(0,1) << " " << -m_mainYA.value() / D << "\n"
    // 	     << "B10: " << B(1,0) << " " << -m_mainXB.value() / D << "\n"
    // 	     << "B11: " << B(1,1) << " " << m_mainXA.value() / D << "\n" ;
    return MSMainCoordinates{ dx, dy } ;
  }
  
  MSMainCoordinates GeometrySvc::toMSMain( const Coordinates2D& c) const
  {
    // now we need the inverse of the matrix
    Coordinates2D tmp = c ;
    tmp.x() -= m_mainX0 ;
    tmp.y() -= m_mainY0 ;
    return toMSMainDelta( tmp ) ;
  }
  
  QTransform GeometrySvc::fromModuleToGlobal( ViewDirection view ) const
  {
    // depending on the value of 'view' we need to mirror the y
    // coordinate. I am not entirely sure how to do that yet.
    QTransform viewmirror ;
    if( view == PAP::CSideView ) viewmirror.scale(1,-1) ;
    // the remainder is a rotation and a translation. these are the
    // numbers that we need to calibrate by looking at the
    // markers. make sure to set the translation first. also that has
    // to do with the order: things set last will be applied on the
    // right.
    QTransform transform ;
    transform.translate( m_moduleposition[view]->x(), m_moduleposition[view]->y() ) ;
    transform.rotateRadians( m_moduleposition[view]->phi() ) ;
    QTransform rc = viewmirror * transform ;
    return rc ;
  }

  void GeometrySvc::applyModuleDelta(ViewDirection view, double dx, double dy, double phi )
  {
    m_moduleposition[view]->applyDelta(dx,dy,phi) ;
    qDebug() << "GeometrySvc::applyModuleDelta: "
	     << view
	     << m_moduleposition[view]->x() 
	     << m_moduleposition[view]->y()
	     << m_moduleposition[view]->phi() ;
  }
  
  void GeometrySvc::setModuleZ(ViewDirection view, double z, double dzdx, double dzdy) {
    m_moduleposition[view]->setZ(z,dzdx,dzdy) ;
  }
  
  double GeometrySvc::moduleZ(ViewDirection view) const {
    return m_moduleposition[view]->z() ;
  }
  
  QTransform GeometrySvc::fromCameraToGlobal() const
  {
    // this needs to take into account the angles of the camera
    // system with the motion system arms. we also need a definition
    // of the origin. it may be useful if this corresponds to the
    // rotation axis of the small stack, for certain values of small
    // stack X and Y. but then it may be better if it is the nominal
    // origin
    QTransform rc ;
    MSMainCoordinates main = MotionSystemSvc::instance()->maincoordinates() ;
    Coordinates2D offset = toGlobal( main ) ;
    rc.rotateRadians( m_cameraPhi ) ;
    rc.translate( offset.x(), offset.y() ) ;
    //qDebug() << "fromCameraToGlobal: " << rc ;
    return rc ;
  }

  Coordinates2D GeometrySvc::stackAxisInGlobal( const MSStackCoordinates& c ) const
  {
    return Coordinates2D{
      m_stackX0 + m_stackXA * c.x + m_stackXB * c.y,
	m_stackY0 + m_stackYA * c.x + m_stackYB * c.y,
	m_stackPhi0 + c.phi
	} ; 
  }
  
  Coordinates2D GeometrySvc::stackAxisInGlobal() const
  {
    return stackAxisInGlobal( MotionSystemSvc::instance()->stackcoordinates() ) ;
  }

  QTransform GeometrySvc::fromStackToGlobal() const
  {
    Coordinates2D coord = stackAxisInGlobal() ;
    QTransform rc ;
    rc.translate( coord.x(), coord.y() ) ;
    rc.rotateRadians( -coord.phi() ) ; // I do not know why I need the minus sign here :-(
    return rc ;
  }

  void GeometrySvc::positionStackForTile( const QString& name) const
  {
    auto it = m_tileStackPositions.find( name ) ;
    if ( it != m_tileStackPositions.end() ) {
      // this is tricky. actually I need to properly deal with the
      // inverse transfomation here. for now, we'll use absolute
      // coordinates, sort of
      auto mssvc = MotionSystemSvc::instance() ;
      mssvc->stackXAxis().moveTo( it->second->x() ) ;
      mssvc->stackYAxis().moveTo( it->second->y() ) ;
      mssvc->stackRAxis().moveTo( it->second->phi() ) ;
    } else {
      qWarning() << "Cannot find tile with name: "
		 << name ;
    }
  }

  void GeometrySvc::applyStackDeltaForTile( const QString& name,
					    double dx, double dy, double dphi )
  {
    auto it = m_tileStackPositions.find( name ) ;
    if ( it != m_tileStackPositions.end() ) {
      qWarning() << "We should change this into the /current/ stack position!" ;
      /*
      it->second->setX( it->second->x() + dx ) ;
      it->second->setY( it->second->y() + dy ) ;
      it->second->setPhi( it->second->phi() + dphi ) ;
      */
      auto current = MotionSystemSvc::instance()->stackcoordinates() ;
      qDebug() << "Preconfigured: "
	       << it->second->x() 
	       << it->second->y() 
	       << it->second->phi() ;
      qDebug() << "Current:       "
	       << current.x
	       << current.y 
	       << current.phi ;
      
      it->second->setX( current.x + dx ) ;
      it->second->setY( current.y + dy ) ;
      it->second->setPhi( current.phi + dphi ) ;
      qDebug() << "Changing stack parameters for tile: "
	       << dx << dy << dphi
	       << it->second->x() 
	       << it->second->y() 
	       << it->second->phi()  ;
    } else {
      qWarning() << "Cannot find tile with name: "
		 << name ;
    }
  }

  void GeometrySvc::updateStackCalibration( double X0, double XA, double XB,
					    double Y0, double YA, double YB, double Phi0)
  {
    // update the preset parameters for all the tiles. for that we
    // need to invert the transformation.
    // FIXME: he stack positioning does not use the calibration, so this doesn't work yet!
    /*
    for( auto it : m_tileStackPositions ) {
      auto& stack = it.second ;
      // The linear transformation is
      //  x'    = X0 + XA * x + XB * y 
      //  y'   = Y0 + YA * x + YB * y
      //  phi'   = Phi0 + phi
      // Now adjust x and y, such that x' and y' are constant while
      // changing X0, XA and XB, for each of the tiles.
      
      // phi is easy
      stack->setPhi( stack->phi() + m_stackPhi0 - Phi0 ) ;
      // now solve
      //     X0 + XA * x + XB * y = X0_new + XA_new * x_new + XB_new * y_new
      //     Y0 + YA * x + YB * y = Y0_new + YA_new * x_new + YB_new * y_new
      // for x_new and y_new
      Eigen::Matrix<double,2,2> A ;
      A << XA, XB, YA, YB ;
      Eigen::Matrix<double,2,1> b ;
      b << m_stackX0 + m_stackXA*stack->x() + m_stackXB*stack->y() - X0,
	m_stackY0 + m_stackYA*stack->x() + m_stackYB*stack->y() - Y0 ;
      Eigen::Matrix<double,2,1> x = A.colPivHouseholderQr().solve(b);
      stack->setX( x(0) ) ;
      stack->setY( x(1) ) ;
    }
    */

    // update the parameters
    m_stackX0 = X0 ;
    m_stackXA=XA;
    m_stackXB = XB;
    m_stackY0 = Y0 ;
    m_stackYA=YA;
    m_stackYB = YB;
    m_stackPhi0 =Phi0 ;
  }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersNSI() const { return Markers::velopixNSI() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersNLO() const { return Markers::velopixNLO() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersCLI() const { return Markers::velopixCLI() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersCSO() const { return Markers::velopixCSO() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersCLISensor() const { return Markers::velopixCLISensor() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersNSISensor() const { return Markers::velopixNSISensor() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersNSide() const
  {
    auto rc = velopixmarkersNSI() ;
    auto rc2 = velopixmarkersNLO() ;
    rc.insert(rc.end(),rc2.begin(),rc2.end()) ;
    auto rc3 = velopixmarkersCLISensor() ;
    rc.insert(rc.end(),rc3.begin(),rc3.end()) ;
    return rc ;
  }

  std::vector<FiducialDefinition>
  GeometrySvc::velopixmarkersCSide() const
  {
    auto rc = velopixmarkersCLI() ;
    auto rc2 = velopixmarkersCSO() ;
    rc.insert(rc.end(),rc2.begin(),rc2.end()) ;
    auto rc3 = velopixmarkersNSISensor() ;
    rc.insert(rc.end(),rc3.begin(),rc3.end()) ;
    return rc ;
  }
  
  std::vector<FiducialDefinition>
  GeometrySvc::jigmarkers() const { return Markers::jigNCSide() ; }

  std::vector<FiducialDefinition>
  GeometrySvc::mcpointsNSide() const { return Markers::microchannelNSide() ; }
  
  std::vector<FiducialDefinition>
  GeometrySvc::mcpointsCSide() const { return Markers::microchannelCSide() ; }

  
}
