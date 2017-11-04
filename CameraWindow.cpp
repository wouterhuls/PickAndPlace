#include "CameraWindow.h"
#include "CameraView.h"
#include "MotionSystemSvc.h"
#include "Eigen/Dense"
#include <iostream>
#include <sstream>
#include <QSignalSpy>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVideoProbe>
#include <QSlider>


namespace PAP
{
  CameraWindow::CameraWindow(QWidget *parent)
    : QMainWindow(parent),
      m_isFocussing(false)
  {
    resize(700,700);
    m_cameraview = new CameraView{} ;
    
    // add a vertical layout. if we derive from 'mainwindow', then
    // layout must be set to the central widget.
    // Set layout in QWidget
    QWidget *window = new QWidget{this};
    auto layout = new QVBoxLayout{} ;//(widget);
    layout->setObjectName(QStringLiteral("CamWindow::layout"));
    layout->setContentsMargins(0, 0, 0, 0);

    setCentralWidget(window);
    window->setLayout(layout);

    //for now add a horizontal bar with buttons
    auto hlayout = new QHBoxLayout{} ;
    layout->addLayout( hlayout) ;
    hlayout->setObjectName(QStringLiteral("CamWindow::hlayout"));
    auto focusbutton = new QPushButton("Focus",this) ;
    focusbutton->setObjectName(QStringLiteral("focusButton"));
    hlayout->addWidget( focusbutton ) ;

    auto quitbutton = new QPushButton("Quit",this) ;
    quitbutton->setObjectName(QStringLiteral("quitButton"));
    hlayout->addWidget( quitbutton ) ;
   
    auto resetzoombutton = new QPushButton("Reset zoom",this) ;
    connect( resetzoombutton, &QPushButton::clicked,  m_cameraview, &CameraView::zoomReset ) ;
    hlayout->addWidget( resetzoombutton ) ;
    
    auto zoomoutbutton = new QPushButton("Zoom out",this) ;
    connect( zoomoutbutton, &QPushButton::clicked,  m_cameraview, &CameraView::zoomOut ) ;
    hlayout->addWidget( zoomoutbutton ) ;

    /*
    auto viewtogglebutton = new QSlider{ Qt::Horizontal, this } ;
    viewtogglebutton->setRange(0,1) ;
    viewtogglebutton->setValue(0) ;
    viewtogglebutton->resize(100,20) ;
    connect( viewtogglebutton, &QAbstractSlider::sliderMoved,  m_cameraview, &CameraView::setViewDirection ) ;
    
    */

    auto viewtogglebutton = new QPushButton{"N/C-side",this} ;
    viewtogglebutton->setCheckable(true) ;
    connect( viewtogglebutton, &QAbstractButton::toggled,  m_cameraview, &CameraView::setViewDirection ) ;
    hlayout->addWidget(viewtogglebutton) ;
    
    //m_cameraview->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
    //m_cameraview->setGeometry(200,200,200,200);
    layout->addWidget( m_cameraview ) ;
    //m_cameraview->show() ;

    QMetaObject::connectSlotsByName(this);
  }

  void CameraWindow::on_quitButton_clicked()
  {
    QCoreApplication::quit();
  }
  
  namespace {
    struct FocusMeasurement
    {
      FocusMeasurement( double _z, double _I) : z(_z),I(_I) {}
      double z ;
      double I ;
      bool operator<(const FocusMeasurement& rhs) const { return z < rhs.z ; }
    } ;
    
    bool estimatez0( const std::vector< FocusMeasurement >& mvec,
		       double& z0 )
    {
      // return true if success
      
      // let's do this as if we are fitting parameters of a
      // parabola. (we should really use Minuit, but will keep that
      // for later.) we want the fit to be linear, so don't try
      // anything smart: just choose
      //    I = [0] + [1]*(z-zref) + [2]*(z-zref)^2
      // initial values are all zero
      // double chi2(0) ;
      Eigen::Vector3d halfdchi2dpar   ;
      Eigen::Matrix3d halfd2chi2dpar2 ;
      const double zref = mvec.front().z ;
      for( const auto& m : mvec ) {
	double dz = m.z - zref ;
	Eigen::Vector3d deriv ;
	deriv(0) = 1 ;
	deriv(1) = dz ;
	deriv(2) = dz*dz ;
	halfdchi2dpar += m.I*deriv ;
	for(int irow=0; irow<3; ++irow)
	  for(int icol=0; icol<3; ++icol)
	    halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
      }
      // now solve
      Eigen::Vector3d pars = halfd2chi2dpar2.ldlt().solve(halfdchi2dpar);
      qDebug() << "Solution: " << pars(0) << " " << pars(1) << " " << pars(2)  ;
      bool success = pars(2) > 0 ;
      if( !success ) {
	std::stringstream str ;
	str << "Vector: " << halfdchi2dpar << std::endl
	    << "Matrix: " << halfd2chi2dpar2 ;
	qDebug() << str.str().c_str() ;
	
      }
      if( success ) z0 = zref - 0.5*pars(1)/pars(2) ;
      return success ;
    }

    FocusMeasurement takeMeasurement(CameraView& camview,
				     MotionAxis& axis, double zpos )
    {
      // move the camera to position zpos
      if( true ||  axis.hasMotorOn() ) {
	axis.moveTo( zpos ) ;
	// wait until it is no longer moving
	if( axis.isMoving() ) {
	  qDebug() << "Waiting for focussing axis to move to correct place." ;
	  // FIXME: it would be nicer to wait for a signal here, but we
	  // could also just wait
	  //QThread::msleep(10) ;
	  //   http://doc.qt.io/qt-5/qsignalspy.html#wait
	  // the problem with this one is that it starts a separate event loop. if it waits for-ever in this 
	  QSignalSpy spy( &axis, SIGNAL(movementStopped())) ;
	  spy.wait( 1000 ) ;
	}
      } else {
	qDebug() << "Cannot focus because Motor is not on" ;
      }
      // make the measurement
      qDebug() << "Will now call computeContrast" ;
      
      QSignalSpy spy(&camview, &CameraView::focusMeasureUpdated) ;
      spy.wait(10000) ;
      //FIXME: the position is not updated, unfortunately. let's take the setposition.
      qDebug() << "CReating focus measurement: "
	       << zpos << " "
	       << axis.position() << " "
	       << camview.focusMeasure() ;
      return FocusMeasurement( zpos, camview.focusMeasure() ) ;
    }
  }
  
  void CameraWindow::on_focusButton_clicked()
  {
    if( !m_isFocussing ) {
      m_isFocussing = true ;
      qDebug() << "In focussing routine" ;
      // perhaps we should use 'minuit' for this. we don't actually know
      // the shape of the function very well.
      const float maxstepsize = 0.10 ; // 30 micron?
      const float minstepsize = 0.01 ;
      MotionAxis* axis = MotionSystemSvc::instance()->axis("Focus") ;
      // tricky: need to make sure this is up-to-date. that will turn
      // out the problem all over this routine: how do we make sure that
      // we have an up-to-date position measurement?
      if(! axis ) { qDebug() << "Cannot find axis!" ; return ; }
      
      const double zstart = axis->position() ;
      std::vector< FocusMeasurement > measurements ;
      measurements.reserve(64) ;
      qDebug() << "Ready to take focus measurements" ;
      // 1. compute three points, around the current z-position
      {
	double zpositions[] = { zstart - maxstepsize, zstart, zstart + maxstepsize } ;
	for( auto zpos : zpositions )
	  measurements.push_back( takeMeasurement(*m_cameraview, *axis, zpos ) ) ;
      }
      // 2. extrapolate to the estimated minimum. we actually want to enclose the minimum ...
      double z0 = zstart ;
      double deltaz0 = maxstepsize ;
      bool success = true ;
      do {
	double z0prev = z0 ;
	success = estimatez0( measurements, z0 ) ;
	if( success ) {
	  // maximize the step size
	  if(     z0 > measurements.back().z + maxstepsize )
	    z0 = measurements.back().z + maxstepsize ;
	  else if( z0 < measurements.front().z - maxstepsize )
	    z0 = measurements.front().z - maxstepsize ;
	  deltaz0 = z0 - z0prev ;
	  // move to the new position and take a new focus measurement
	  measurements.push_back( takeMeasurement(*m_cameraview, *axis, z0 ) ) ;
	  std::sort( measurements.begin(), measurements.end() ) ;
	  qDebug() << "Number of focussing measurements: " << measurements.size() ;
	}
      } while( success && std::abs(deltaz0) > minstepsize ) ;
      
      // if not successful, move back to where we started
      if( !success ) {
	qWarning() << "Focussing failed!" ;
	qDebug() << "Measurements: " << measurements.size() ;
	for( const auto& m : measurements )
	  qDebug() << m.z << " " << m.I ;
	axis->moveTo( zstart ) ;
      }
      m_isFocussing = false ;
    }
  }

  
}
