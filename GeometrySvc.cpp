#include "GeometrySvc.h"
#include "PropertySvc.h"
#include "MotionSystemSvc.h"
#include "NominalMarkers.h"

#include <cmath>

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
  
  GeometrySvc::GeometrySvc()
    : m_mainX0( "Geo.mainX0", 0. ),
      m_mainXA( "Geo.mainXA", 1.0 ),
      m_mainXB( "Geo.mainXB", 0.0 ),
      m_mainY0( "Geo.mainY0", 0. ),
      m_mainYA( "Geo.mainYA", 0.0 ),
      m_mainYB( "Geo.mainYB", -1.0 ),
      m_cameraPhi( "Geo.cameraPhi", 0 ),
      m_modulePhi( "Geo.modulePhi", -0.0027 ),
      m_moduleX( "Geo.moduleX", -32.09), // perhaps we should move these into mainX0 and mainY0
      m_moduleY( "Geo.moduleY", -64.51),
      // these points define the position and orientation of the stack rotation axis in the global frame
      m_stackX0( "Geo.stackX0", -13. ),
      m_stackXA( "Geo.stackXA", -1.0 ),
      m_stackXB( "Geo.stackXB", 0.0 ),
      m_stackY0( "Geo.stackY0", -40. ),
      m_stackYA( "Geo.stackYA", 0.0 ),
      m_stackYB( "Geo.stackYB", -1.0 ),
      m_stackPhi0( "Geo.stackPhi0", 0.491998 )
  {
    PAP::PropertySvc::instance()->add( m_mainX0 ) ;
    PAP::PropertySvc::instance()->add( m_mainXA ) ;
    PAP::PropertySvc::instance()->add( m_mainXB ) ;
    PAP::PropertySvc::instance()->add( m_mainY0 ) ;
    PAP::PropertySvc::instance()->add( m_mainYA ) ;
    PAP::PropertySvc::instance()->add( m_mainYB ) ;
    PAP::PropertySvc::instance()->add( m_cameraPhi ) ;
    PAP::PropertySvc::instance()->add( m_modulePhi ) ;
    PAP::PropertySvc::instance()->add( m_moduleX ) ;
    PAP::PropertySvc::instance()->add( m_moduleY ) ;

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
    m_tileStackPositions["CLI"] = new TileStackPosition{"CLI",13.127,20.282,0.28715} ;
    m_tileStackPositions["CSO"] = new TileStackPosition{"CSO",13.419,12.143,-1.2900} ;
    m_tileStackPositions["NSI"] = new TileStackPosition{"NSI",0,0,0} ;
    m_tileStackPositions["NLO"] = new TileStackPosition{"NLO",0,0,0} ;
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
    double dx = (  m_mainYB * c.x - m_mainYA * c.y)/D ;
    double dy = ( -m_mainXB * c.x + m_mainXA * c.y)/D ;
    return MSMainCoordinates{ dx, dy } ;
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
    transform.translate( m_moduleX, m_moduleY ) ;
    transform.rotateRadians( m_modulePhi ) ;
    QTransform rc = viewmirror * transform ;
    return rc ;
  }

  void GeometrySvc::applyModuleDelta( double dx, double dy, double phi )
  {
    QTransform T ;
    T.translate( m_moduleX, m_moduleY ) ;
    T.rotateRadians( m_modulePhi ) ;
    QTransform dT ;
    dT.translate( dx, dy) ;
    dT.rotateRadians( phi ) ;
    QTransform Tnew = T*dT ;
    m_modulePhi = std::atan2( Tnew.m12(), Tnew.m11() ) ;
    m_moduleX   = Tnew.m31() ;
    m_moduleY   = Tnew.m32() ;
    qDebug() << "GeometrySvc::applyModuleDelta: "
	     << m_moduleX << m_moduleY << m_modulePhi ;
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
    rc.translate( offset.x, offset.y ) ;
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
    rc.translate( coord.x, coord.y ) ;
    rc.rotateRadians( coord.phi ) ;
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
      it->second->setX( it->second->x() + dx ) ;
      it->second->setY( it->second->y() + dy ) ;
      it->second->setPhi( it->second->phi() + dphi ) ;
      qDebug() << "Changing stack parameters for tile: "
	       << dx << dy << dphi
	       << it->second->x() 
	       << it->second->y() 
	       << it->second->phi()  ;
      qWarning() << "We should change this into the /current/ stack position!" ;
    } else {
      qWarning() << "Cannot find tile with name: "
		 << name ;
    }
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
