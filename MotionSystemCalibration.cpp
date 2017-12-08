#include <QTabWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include "GeometrySvc.h"
#include "MotionSystemCalibration.h"
#include "CoordinateMeasurement.h"
#include "MotionSystemSvc.h"
#include "Eigen/Dense"

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
      
      auto addbutton = new QPushButton{ "Add", this } ;
      connect( addbutton, &QPushButton::clicked, [=](){ this->add() ; } ) ;
      hlayout->addWidget( addbutton ) ;

      auto testbutton = new QPushButton{ "Test", this } ;
      connect( testbutton, &QPushButton::clicked, [=](){ this->test() ; } ) ;
      hlayout->addWidget( testbutton ) ;
      
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
	Eigen::Vector3d halfdchi2dparX   ;
	Eigen::Vector3d halfdchi2dparY   ;
	Eigen::Matrix3d halfd2chi2dpar2 ;
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
	//clear() ;
	// add some text to the display box:
	char textje[256] ;
	sprintf( textje, "parX: %f %f %f\n", m_parX(0),m_parX(1),m_parX(2) ) ;
	m_measurementsdisplaybox->append(textje) ;
	sprintf( textje, "parY: %f %f %f\n", m_parY(0),m_parY(1),m_parY(2) ) ;
	m_measurementsdisplaybox->append(textje) ;
	
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
  public:
    StackCalibrationPage( QWidget* parent=0 )
      : QWidget( parent )
    {
      auto hlayout = new QHBoxLayout{} ;
      this->setLayout(hlayout) ;

      auto tmptext1 = new QLabel{this} ;
      tmptext1->setText("The idea is that by taking a number of different measurements at different positions,"
			"we should be able to calibrate the small stage such that we can follow a marker with the main stage"
			"while arbitrarily moving the marker with the stack") ;
      tmptext1->setWordWrap(true);
      hlayout->addWidget( tmptext1 ) ;

    }
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
