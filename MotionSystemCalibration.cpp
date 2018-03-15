#include <QTabWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QDebug>

#include "GeometrySvc.h"
#include "MotionSystemCalibration.h"
#include "CoordinateMeasurement.h"
#include "MotionSystemSvc.h"
#include "Eigen/Dense"
#include <iostream>

namespace PAP
{
  
  // Helper class for calibrating parameters of the stack. For now just the axis of rotation?
  // Main idea:
  // - choose a marker that is not located directly on the axis
  // - mark its 'mainstage' position
  // - rotate the stack. mark the position again
  // - do this a couple of times
  // - fit the center of the circle: this is the rotation axis

  struct GlassPlateMeasurement 
  {
    // actually, we just need the main stage
    GlassPlateMeasurement( MSMainCoordinates c, int x, int y ) :
      coordinates(c),markerX(x),markerY(y) {}
    MSMainCoordinates coordinates ;
    int markerX ;
    int markerY ;
  } ;

  class MainStageCalibrationPage : public QWidget
  {
  private:
    std::vector<GlassPlateMeasurement> m_measurements ;
    QSpinBox* m_markerX ;
    QSpinBox* m_markerY ;
    QLabel* m_axisPositionLabel ;
    QTextEdit* m_measurementsdisplaybox ;
    Eigen::Vector3d m_parX   ;
    Eigen::Vector3d m_parY   ;
  public:
    MainStageCalibrationPage( QWidget* parent=0 )
      : QWidget(parent),
	m_parX{0,1,0},
	m_parY{0,0,1}
    {
      auto layout = new QVBoxLayout{} ;
      this->setLayout(layout) ;
      
      auto tmptext1 = new QLabel{this} ;
      tmptext1->setText("a. place the glass plate on the machine\n"
			"b. take a number of measurements of markers\n"
			"   - move to a marker \n"
			"   - record the coordinates by pressing right mouse button and 'record' \n"
			"   - manually insert the coordinates (X,y in mm) written on the glass plate \n"
			"   - press 'accept'\n"
			"c. press the 'calibrate' button\n"
			"d. use the 'test' field to check for arbitrary glass plate coordinates that you did the right thing\n"
			"e. FINALLY press the 'Update' button. without this, the constants used by the geometrysvc will not be updated!") ;
      tmptext1->setWordWrap(true);
      layout->addWidget( tmptext1 ) ;
      
      auto hlayout = new QHBoxLayout{} ;
      layout->addLayout(hlayout) ;

      hlayout->addWidget( new QLabel{"X: ", this} ) ;
      m_markerX = new QSpinBox{this} ;
      m_markerX->setRange(0,120) ;
      hlayout->addWidget(m_markerX) ;
      
      hlayout->addWidget( new QLabel{"Y: ", this} ) ;
      m_markerY = new QSpinBox{this} ;
      m_markerY->setRange(0,120) ;
      hlayout->addWidget(m_markerY) ;
      
      m_axisPositionLabel = new QLabel{this} ;
      m_axisPositionLabel->setWordWrap(true) ;
      updatePositionLabel() ;
      connect( MotionSystemSvc::instance(), &MotionSystemSvc::mainStageMoved,
	       [=](){ this->updatePositionLabel() ; } ) ;
      hlayout->addWidget( m_axisPositionLabel ) ;
      
      auto testbutton = new QPushButton{ "Test", this } ;
      connect( testbutton, &QPushButton::clicked, [=](){ this->test() ; } ) ;
      hlayout->addWidget( testbutton ) ;
      
      auto addbutton = new QPushButton{ "Add", this } ;
      connect( addbutton, &QPushButton::clicked, [=](){ this->add() ; } ) ;
      hlayout->addWidget( addbutton ) ;

      m_measurementsdisplaybox = new QTextEdit{this} ;
      layout->addWidget( m_measurementsdisplaybox ) ;
      m_measurementsdisplaybox->resize(500,100) ;

      auto hlayout2 = new QHBoxLayout{} ;
      layout->addLayout(hlayout2) ;

      auto clearbutton = new QPushButton{ "Clear", this } ;
      connect( clearbutton, &QPushButton::clicked, [=](){ this->clear() ; } ) ;
      hlayout2->addWidget( clearbutton ) ;

      auto calibratebutton = new QPushButton{ "Calibrate", this } ;
      connect( calibratebutton, &QPushButton::clicked, [=](){ this->calibrate() ; } ) ;
      hlayout2->addWidget( calibratebutton ) ;

      auto readybutton = new QPushButton{ "Update", this } ;
      connect( readybutton, &QPushButton::clicked, [=](){ this->updateGeometry() ; } ) ;
      hlayout2->addWidget( readybutton ) ;

      // need something to insert a new measurement
      // need a button to clear the list of measurements, to calibrate, and to test
   
    }
        
    void updatePositionLabel()
    {
      char textje[256] ;
      auto mssvc = MotionSystemSvc::instance() ;
      sprintf(textje,"axisX: %6.3f\naxisY: %6.3f",mssvc->mainXAxis().position().value(),mssvc->mainYAxis().position().value()) ;
      m_axisPositionLabel->setText(textje) ;
      
    }
    
    void add()
    {
      m_measurements.push_back( {MotionSystemSvc::instance()->maincoordinates(),
	    m_markerX->value(),m_markerY->value()} ) ;
      // we also add a line to the box showing all measurements
      char textje[256] ;
      const auto& m=m_measurements.back() ;
      sprintf( textje, "%2d : markerX = %03d, markerY = %03d, axisX = axisX: %06.3f, axisY: %06.3f",
	       int(m_measurements.size()), m.markerX, m.markerY, m.coordinates.x, m.coordinates.y ) ;
      m_measurementsdisplaybox->append(textje) ;
    }

    void test()
    {
      // now we need the inverse of
      // X = parX[0] + parX[1]*mainX + parX[2]*mainY
      // Y = parY[0] + parY[1]*mainX + parY[2]*mainY
      double D = m_parX(1)*m_parY(2) - m_parX(2)*m_parY(1) ;
      double dX = m_markerX->value() - m_parX(0) ;
      double dY = m_markerY->value() - m_parY(0) ;
      double mainX = + m_parY(2)/D * dX - m_parX(2)/D * dY ;
      double mainY = - m_parY(1)/D * dX + m_parX(1)/D * dY ;
      MotionSystemSvc::instance()->mainXAxis().moveTo( mainX ) ;
      MotionSystemSvc::instance()->mainYAxis().moveTo( mainY ) ;
    }

    void clear()
    {
      m_measurements.clear() ;
      m_measurementsdisplaybox->clear() ;
    }

    void calibrate()
    {
      // this is where it is all happening. require at least 4 measurements.
      if( m_measurements.size()>=2 ) {
	// the easiest model is 
	//   X = [0] + [1]*mainX + [2]*mainY
	//   Y = [3] + [4]*mainX + [5]*mainY
	// which is conveniently also entirely linear.  since X and Y
	// are independent, we could as well compute them
	// separately.
	const double sigma  = 0.005 ; // 5 micron per marker
	
	Eigen::Vector3d halfdchi2dparX = Eigen::Vector3d::Zero() ;
	Eigen::Vector3d halfdchi2dparY = Eigen::Vector3d::Zero()  ;
	Eigen::Matrix3d halfd2chi2dpar2 = Eigen::Matrix3d::Zero();
	for( const auto& m : m_measurements ) {
	  Eigen::Vector3d deriv ;
	  deriv(0) = 1 ;
	  deriv(1) = m.coordinates.x ;
	  deriv(2) = m.coordinates.y ;
	  halfdchi2dparX += m.markerX * deriv ;
	  halfdchi2dparY += m.markerY * deriv ;
	  for(int irow=0; irow<3; ++irow)
	    for(int icol=0; icol<3; ++icol)
	      halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
	}
	m_parX = halfd2chi2dpar2.ldlt().solve(halfdchi2dparX) ;
	m_parY = halfd2chi2dpar2.ldlt().solve(halfdchi2dparY) ;
	auto cov = halfd2chi2dpar2.inverse() ;
	
	qDebug() << "derivX: "
		 << halfdchi2dparX(0)
		 << halfdchi2dparX(1)
		 << halfdchi2dparX(2) ;
	qDebug() << "derivX: "
		 << halfdchi2dparY(0)
		 << halfdchi2dparY(1)
		 << halfdchi2dparY(2) ;
	qDebug() << "2nd derivative: "
		 << halfd2chi2dpar2(0,0)
		 << halfd2chi2dpar2(0,1)
		 << halfd2chi2dpar2(0,2)
		 << halfd2chi2dpar2(1,0)
		 << halfd2chi2dpar2(1,1)
		 << halfd2chi2dpar2(1,2)
		 << halfd2chi2dpar2(2,0)
		 << halfd2chi2dpar2(2,1)
		 << halfd2chi2dpar2(2,2) ;
	
	//clear() ;
	// add some text to the display box:
	char textje[256] ;
	
	for(int i=0; i<3; ++i) {
	  sprintf( textje, "%d: x: %f  y: %f sigma: %f", i,
		   m_parX(i), m_parY(i), sigma*std::sqrt(cov(i,i)) ) ;
	  m_measurementsdisplaybox->append(textje) ;
	}
      } else {
	// need to pop up a dialog
      }
    }

    void updateGeometry() 
    {
      // this is actually the more complicated part. I want to keep
      // the scale factors, but rotate the axis such that X remains
      // horizontal, i.e. parX(2) must become zero. it cannot be that
      // hard, can it?

      // suppose phiX is the angle between mainAxisX-axis and the
      // markerX-axis and phiY the angle between mainAxisY-axis and
      // the markerX-axis, then the model is
      //   markerX = offsetX + cos(phiX) * scaleX * axisX + cos(phiY) * scale Y * axis Y
      //   markerY = offsetY + sin(phiX) * scaleX * axisX + sin(phiY) * scale Y * axis Y
      // note that phiX is close to zero, but phiY is close to 90 degrees
      
      // therefore:
      double scaleX = std::sqrt( m_parX(1)*m_parX(1) + m_parY(1)*m_parY(1) ) ;
      double scaleY = std::sqrt( m_parX(2)*m_parX(2) + m_parY(2)*m_parY(2) ) ;
      //double phiX   = std::atan2( m_parY(1), m_parX(1) ) ;
      //double phiY   = std::atan2( m_parX(2), m_parY(2) ) ;
      double cosphiX  = m_parX(1) / scaleX ;
      double sinphiX  = m_parY(1) / scaleX ;
      double cosphiY  = m_parX(2) / scaleY ;
      double sinphiY  = m_parY(2) / scaleY ;
      
      // we keep the offsets at zero, for now.
      //double x0(0),y0(0) ;
      // by construction:
      double xA = scaleX ;
      double yA = 0 ; 
      // xB = scaleY * cos( phiY - phiX ) = scaleY * [ cos(phiY)*cos(phiX) - sin(phiY)*sin(phiX) ] 
      double xB = scaleY*(cosphiY*cosphiX+sinphiY*sinphiX);
      // yB = scaleY * sin( phiY - phiX ) = scaleY * [ sin(phiY)*cos(phiX) - sin(phiX)*cos(phiY)
      double yB = scaleY*(sinphiY*cosphiX-sinphiX*cosphiY) ;
      GeometrySvc::instance()->updateMainAxisCalibration( xA, xB, yA, yB ) ;

      // something is wrong!!

      double phiX   = std::atan2( m_parY(1), m_parX(1) ) ;
      double phiY   = std::atan2( m_parY(2), m_parX(2) ) ;

      qDebug() << "scales: " << scaleX << scaleY ;
      qDebug() << "angles: " << phiX << phiY ;
      qDebug() << "xB : " << xB << scaleY*sin(phiY-phiX) ;
      qDebug() << "yB : " << yB << scaleY*cos(phiY-phiX) ;
      
      // add some text to the display box:
      char textje[256] ;
      sprintf( textje, "parX geo: %f %f\n", xA, xB ) ;
      m_measurementsdisplaybox->append(textje) ;
      sprintf( textje, "parY geo: %f %f\n", yA, yB ) ;
      m_measurementsdisplaybox->append(textje) ;
    }
    
  } ;
  
  // Functionality that we need:
  // - some way to make a list of measurements. each measurement just records the position of all axes
  // - a way to display these measurements such that you can also remove those that were a mistake
  // - a fit

  class StackCalibrationPage : public QWidget
  {
  private:
    QPointF m_taggedCameraInStack ;
    std::vector<MSCoordinates> m_measurements ;
  public:
    StackCalibrationPage( QWidget* parent=0 )
      : QWidget( parent )
    {
      auto hlayout = new QVBoxLayout{} ;
      this->setLayout(hlayout) ;

      auto tmptext1 = new QLabel{this} ;
      tmptext1->setText("The idea is that by taking a number of different measurements at different positions,"
			"we should be able to calibrate the small stage such that we can follow a marker with the main stage"
			"while arbitrarily moving the marker with the stack") ;
      tmptext1->setWordWrap(true);
      hlayout->addWidget( tmptext1 ) ;

      auto recordbutton = new QPushButton{"Record"} ;
      hlayout->addWidget(recordbutton) ;
      connect(recordbutton,&QPushButton::clicked,
	      [=]{ this->m_measurements.push_back( MotionSystemSvc::instance()->coordinates() ) ; } ) ;

      auto clearbutton = new QPushButton{"Clear"} ;
      hlayout->addWidget(clearbutton) ;
      connect(clearbutton,&QPushButton::clicked,
	      [=]{ this->m_measurements.clear() ; } ) ;

      auto calibratebutton = new QPushButton{"Calibrate"} ;
      hlayout->addWidget(calibratebutton) ;
      connect(calibratebutton,&QPushButton::clicked,
	      [=]{ this->calibrate() ; } ) ;
      
      auto tagbutton = new QPushButton{"Tag"} ;
      hlayout->addWidget(tagbutton) ;
      connect(tagbutton,&QPushButton::clicked,
	      [=]{ this->tag() ; } ) ;

      auto trackbutton = new QPushButton{"Track"} ;
      hlayout->addWidget(trackbutton) ;
      connect(trackbutton,&QPushButton::clicked,
	      [=]{ this->track() ; } ) ;
      /*
      {
	// make a few buttons for predefined positions, such that this takes me less time
	auto mssvc = MotionSysteSvc::instance() ;
	auto stepbuttonlayout = new QHBoxLayout{} ;
	hlayout->addLayout(stepbuttonlayout) ;
	auto P1 = new QPushButton{"P1"} ;
	stepbuttonlayout->addWidget(P1) ;
	connext(P1,&QPushButton::clicked,
		[=]{ mssvc
	

      }
      */
    }

    // calibrate from a number of measurements
    void calibrate()
    {
      qDebug() << "Number of measurements: " << m_measurements.size() ;
      // below we use transforms to go from one frame to another. but
      // to compute the parameters we need to explicitly model the
      // transforms, compute derivatives etc. in that process, we
      // hopefully also find the mistake in the transforms.

      // This is the transformation 'geomsvc::fromCameratoGlobal. We
      // do not touch that in this calibration step. Let's assume it is right.
      //  QTransform rc ;
      //  MSMainCoordinates main = MotionSystemSvc::instance()->maincoordinates() ;
      //  Coordinates2D offset = toGlobal( main ) ;
      //  rc.rotateRadians( m_cameraPhi ) ;
      //  rc.translate( offset.x(), offset.y() ) ;

      // This computes the transform for the stack axis.
      /*
	Coordinates2D GeometrySvc::stackAxisInGlobal( const MSStackCoordinates& c ) const
	{
 	 return Coordinates2D{
	  m_stackX0 + m_stackXA * c.x + m_stackXB * c.y,
	  m_stackY0 + m_stackYA * c.x + m_stackYB * c.y,
	  m_stackPhi0 + c.phi
	 } ; 
	}
	QTransform GeometrySvc::fromStackToGlobal() const
	{
	Coordinates2D coord = stackAxisInGlobal() ;
	QTransform rc ;
	rc.translate( coord.x(), coord.y() ) ;
	rc.rotateRadians( -coord.phi() ) ; // I do not know why I need the minus sign here :-(
	return rc ;
	}

      */

      // In the following we need to compute stackX?, stackY? and
      // stackPhi0. These are 7 parameters in total. However, we
      // assume that the X and Y axis scale are well
      // calibrated, so that we only need to calibrate the angles of both axis with the global system axes.

      auto geomsvc = GeometrySvc::instance() ;
      if(m_measurements.size()>0) {
	// for every measurement we do
	// - compute the global coordinates from MSMain coordinates
	// - compute the prediction of this by using the transforms
	// - compute the residuals to the transform parameters

	// the first measurement has zero residual: this is what
	// defines the marker in the stack frame. but it also cannot
	// be right to exclude it from the measurement, can it?

	// but of course: the position of the marker in the stack
	// frame is also unknown: so these are two extra
	// parameters. we need to initialize them with something, so
	// we do that with the first measurement.

	// compute the markerposition in the stack frame from the first measurement.
	QPointF markerposstackframe ;
	{
	  const auto m0 =  m_measurements.front() ;
	  const Coordinates2D markerglobalpos = geomsvc->toGlobal( m0.main ) ;
	  const Coordinates2D stackaxis = geomsvc->stackAxisInGlobal( m0.stack ) ;
	  const double dx = markerglobalpos.x() - stackaxis.x() ;
	  const double dy = markerglobalpos.y() - stackaxis.y() ;
	  // now just rotate in the stack frame: need to get the sign of the angle right:-)
	  const double sinphi = std::sin(stackaxis.phi()) ;
	  const double cosphi = std::cos(stackaxis.phi()) ;
	  double dxprime = cosphi*dx - sinphi*dy ; 
	  double dyprime = sinphi*dx + cosphi*dy ; 
	  markerposstackframe = QPointF{dxprime,dyprime} ;
	  qDebug() << "Position of marker in stack frame: " << markerposstackframe ;
	}
	// the parameters are:
	// 0: marker pos stack frame x
	// 1: marker pos stack frame y
	// 2: m_stackX0
	// 3: m_stackY0
	// 4. thetaX: m_stackXA = cos(thetaX); m_stackYA = sin(thetaX) 
	// 5. thetaY: m_stackXB = sin(thetaY); m_stackYB = cos(thetaY) 
	// 6: m_stackPhi0
	using Vector7d = Eigen::Matrix<double,7,1> ;
	using Matrix7d = Eigen::Matrix<double,7,7> ;
	Vector7d pars ;
	pars(0) = markerposstackframe.x() ;
	pars(1) = markerposstackframe.y() ;
	pars(2) = geomsvc->stackX0() ;
	pars(3) = geomsvc->stackY0() ;
	pars(4) = std::atan2(geomsvc->stackYA(),geomsvc->stackXA()) ;
	pars(5) = std::atan2(geomsvc->stackXB(),geomsvc->stackYB()) ;
	pars(6) = geomsvc->stackPhi0() ;

	
					    
	const int maxnsteps = 5 ; 
	for( int istep=0; istep<maxnsteps; ++istep) {

	  Vector7d halfdchi2dpar   = Vector7d::Zero() ;
	  Matrix7d halfd2chi2dpar2 = Matrix7d::Zero() ;
	  const double sigma2 = 0.005*0.005 ; // sigma = 5 micron
	  
	  for( const auto& m : m_measurements) {
	    // now we do the same, but with inverse transformations, compute residual, etc
	    Coordinates2D markerglobalposref = geomsvc->toGlobal( m.main ) ;

	    const Coordinates2D stackaxisorig = geomsvc->stackAxisInGlobal( m.stack ) ;
	    const Coordinates2D stackaxis = {
	      pars(2) + std::cos( pars(4) ) * m.stack.x + std::sin( pars(5) ) * m.stack.y,
	      pars(3) + std::sin( pars(4) ) * m.stack.x + std::cos( pars(5) ) * m.stack.y,
	      pars(6) + m.stack.phi } ;
	    qDebug() << "stackaxis orig: "
		     << stackaxisorig.x() << stackaxisorig.y() <<  stackaxisorig.phi() ;
	    qDebug() << "stackaxis new : "
		     << stackaxis.x() << stackaxis.y() <<  stackaxis.phi() ;
	    
	    	    
	    const double sinphi = std::sin(stackaxis.phi()) ;
	    const double cosphi = std::cos(stackaxis.phi()) ;
	    double x = stackaxis.x() + pars(0) * cosphi + pars(1) * sinphi ;
	    double y = stackaxis.y() - pars(0) * sinphi + pars(1) * cosphi ;

	    double residualX = x - markerglobalposref.x() ;
	    double residualY = y - markerglobalposref.y() ;
	    qDebug() << "Residual: " << residualX << residualY ;
	    
	    // x and y factorise, so just do them one by one
	    Vector7d deriv ;
	    {
	      // derivatives of residual X
	      deriv(0) = cosphi ;
	      deriv(1) = sinphi ;
	      deriv(2) = 1 ;
	      deriv(3) = 0 ;
	      deriv(4) = -1.0 * std::sin( pars(4) ) * m.stack.x ;
	      deriv(5) = +1.0 * std::cos( pars(5) ) * m.stack.y ;
	      deriv(6) = pars(0) * -1.0 * sinphi + pars(1) * +1.0 * cosphi ;
	      halfdchi2dpar += residualX/sigma2 * deriv ;
	      for(int irow=0; irow<7; ++irow)
		for(int icol=0; icol<7; ++icol)
		  halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol)  / sigma2;
	    }
	    {
	      // derivatives of residual Y
	      deriv(0) = -sinphi ;
	      deriv(1) =  cosphi ;
	      deriv(2) = 0 ;
	      deriv(3) = 1 ;
	      deriv(4) = +1.0 * std::cos( pars(4) ) * m.stack.x ;
	      deriv(5) = -1.0 * std::sin( pars(5) ) * m.stack.y ;
	      deriv(6) = pars(0) * -1.0 * cosphi + pars(1) * -1.0 * sinphi ;
	      halfdchi2dpar += residualY/sigma2* deriv ;
	      for(int irow=0; irow<7; ++irow)
		for(int icol=0; icol<7; ++icol)
		  halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol)  / sigma2;
	    }
	  }
	  
	  std::cout << "1st derivative: " << halfdchi2dpar << std::endl ;
	  std::cout << "2nd derivative: " << halfd2chi2dpar2 << std::endl ;
		  
	  Vector7d delta = halfd2chi2dpar2.ldlt().solve(halfdchi2dpar) ;
	  std::cout << "Delta: " << delta << std::endl ;
	  double dchi2 = delta.dot(halfdchi2dpar) ;
	  
	  std::cout << "deltachi2 (module factor 2): " << istep << " " << delta.dot(halfdchi2dpar) << std::endl ;
	  
	  pars -= delta ;
	  if(dchi2<0.01) break ;
	  
	}
	geomsvc->setStackParameters( pars(2), std::cos( pars(4) ), std::sin( pars(5) ),
				     pars(3), std::sin( pars(4) ), std::cos( pars(5) ),
				     pars(6) ) ;
	m_measurements.clear() ;
      }
    }

    // determine the current global position of the camera in the stack frame
    void tag()
    {
      auto geomsvc = GeometrySvc::instance() ;
      QTransform fromCameraToGlobal = geomsvc->fromCameraToGlobal() ;
      QTransform fromStackToGlobal = geomsvc->fromStackToGlobal() ;
      QTransform fromCameraToStack =  fromCameraToGlobal * fromStackToGlobal.inverted();
      
      //QPointF cameraInGlobal = QPointF(0,0) * fromCameraToGlobal ;
      QPointF cameraInStack  = QPointF(0,0) * fromCameraToStack ;
      qDebug() << "CameraInStack: " << cameraInStack ;
      m_taggedCameraInStack = cameraInStack ;
    } ;

    // move the camera to the tagged position
    void track()
    {
      //
      auto geomsvc = GeometrySvc::instance() ;
      QTransform fromStackToGlobal = geomsvc->fromStackToGlobal() ;
      QPointF globalpos = m_taggedCameraInStack * fromStackToGlobal ;
      qDebug() << "In track: " << globalpos ;
      // we must find a better way to do this
      auto msmaincoordinates = geomsvc->toMSMain( globalpos ) ;
      auto mssvc = MotionSystemSvc::instance() ;
      mssvc->mainXAxis().moveTo( msmaincoordinates.x ) ;
      mssvc->mainYAxis().moveTo( msmaincoordinates.y ) ;
      // Current status: this orks for x,y displacements, but rotation
      // is exactly in opposite direction!
    } ;
    
  } ;


  MotionSystemCalibration::MotionSystemCalibration( QWidget* parent )
    : QDialog(parent)
  {
    resize(600,600) ;
    setWindowTitle("Motion System Calibration") ;
    auto taskpages = new QTabWidget{this} ;
    auto maincalibration = new MainStageCalibrationPage{} ;
    taskpages->addTab(maincalibration,"Main stage calibration") ;
    auto stackcalibration = new StackCalibrationPage{} ;
    taskpages->addTab(stackcalibration,"Stack calibration") ;
  }

}
