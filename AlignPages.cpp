#include "AlignPages.h"
#include "MotionSystemSvc.h"
#include "GeometrySvc.h"
#include "CoordinateMeasurement.h"
#include "CameraView.h"

#include <cmath>
#include "Eigen/Dense"
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>


namespace PAP
{

  // helper class for page for alignment of the main jig
  MarkerRecorderWidget::MarkerRecorderWidget(const char* markername,
					     const PAP::CameraView* camview,
					     QWidget* parent)
    : QWidget(parent), m_cameraview(camview), m_status(Uninitialized)
  {
    setObjectName( markername ) ;
    auto hlayout = new QHBoxLayout{} ;
    setLayout(hlayout) ;
    auto movetomarker1button = new QPushButton{markername, this} ;
    connect(movetomarker1button,&QPushButton::clicked,[=](){ camview->moveCameraTo(markername) ; } ) ;
    hlayout->addWidget( movetomarker1button ) ;
    
    auto recordbutton = new QPushButton{"record", this} ;
    recordbutton->setCheckable(true) ;
    hlayout->addWidget( recordbutton ) ;
    connect(this, &MarkerRecorderWidget::ready, [=]() { recordbutton->setChecked(false) ; setStatus(Ready) ; } ) ;
    connect(recordbutton,&QPushButton::toggled,this,&MarkerRecorderWidget::on_recordbutton_toggled) ;
    // catch measurement updates
    connect( camview, &CameraView::recording, this, &MarkerRecorderWidget::record  ) ;
    // show a label with the status
    m_statuslabel = new QLabel{ this } ;
    setStatus( Uninitialized ) ;
    hlayout->addWidget( m_statuslabel ) ;
    
  }

  void MarkerRecorderWidget::record( const CoordinateMeasurement& m) {
    if( m_status == Active ) {
      m_measurement = m ;
      m_markerposition = m_cameraview->globalPosition( objectName() ) ;
      setStatus( Ready ) ;
      emit ready() ;
      qDebug() << "received measurement: (" << m.globalcoordinates.x << "," << m.globalcoordinates.y << ")" ;
      qDebug() << "marker position:      " << m_markerposition ;
    }
  }
  
  void MarkerRecorderWidget::setStatus(Status s) {
    m_status = s ;
    if( m_status == Ready )
      m_statuslabel->setText( "Ready" ) ;
    else if( m_status == Active )
      m_statuslabel->setText( "Recording" ) ;
    else
      m_statuslabel->setText( "Uninitialized" ) ;
  }
  
  AlignMainJigPage::AlignMainJigPage(PAP::CameraView* camview)
    : m_cameraview(camview)
  {
    auto vlayout = new QVBoxLayout{} ;
    this->setLayout(vlayout) ;
    m_marker1recorder = new MarkerRecorderWidget( "MainJigMarker1", camview ) ;
    vlayout->addWidget( m_marker1recorder ) ;
    m_marker2recorder = new MarkerRecorderWidget( "MainJigMarker2", camview ) ;
    vlayout->addWidget( m_marker2recorder ) ;
    auto calibrationbutton = new QPushButton{"Calibrate", this} ;
    connect(calibrationbutton,&QPushButton::clicked,[=](){ this->updateAlignment() ; } ) ;
    vlayout->addWidget( calibrationbutton ) ;
    /*
    auto tmptext1 = new QLabel{this} ;
    tmptext1->setText("a. place the master jig in the machine\n"
		      "b. press the 'C' or 'NC' button to identify the side\n"
		      "c. press 'zoom master jig marker 1' button to move first jig marker in field of view of camera\n"
		      "d. press the record button to record the position of the marker in the view\n"
		      "e. press 'zoom master jig marker 2' button to move the second marker in field of view\n"
		      "f. press the record button to record the position of the marker in the view\n"
		      "g. press the 'calibrate jig coordinate frame' button") ;
    tmptext1->setWordWrap(true);
    QFont font = tmptext1->font();
    font.setPointSize(2);
    font.setBold(true);
    tmptext1->setFont(font);
    vlayout->addWidget( tmptext1 ) ;
    */
  }

  void AlignMainJigPage::updateAlignment() const
  {
    // take the two measurements. then update the relevant
    // parameters (which are Geo.moduleX, moduleY and modulePhi)
    //
    // these parameter essentially make a transform and we need to
    // update that transform.
    //
    // I decided to do everything in the 'global' frame. we then simply compute dx,dy and phi, and finally update the transform:
    if( m_marker1recorder->status() == MarkerRecorderWidget::Ready &&
	m_marker2recorder->status() == MarkerRecorderWidget::Ready ) {
      std::vector< MarkerRecorderWidget* > recordings = { m_marker1recorder, m_marker2recorder } ;
      
      // Let's first print the measured and expected distance between
      // the markers. This gives an idea of how accurate we can ever
      // be.
      {
	double Lx = m_marker1recorder->markerposition().x() - m_marker2recorder->markerposition().x() ;
	double Ly = m_marker1recorder->markerposition().y() - m_marker2recorder->markerposition().y() ;
	double Lx_m  = m_marker1recorder->measurement().globalcoordinates.x -
	  m_marker2recorder->measurement().globalcoordinates.x ;
	double Ly_m  = m_marker1recorder->measurement().globalcoordinates.y -
	  m_marker2recorder->measurement().globalcoordinates.y ;
	qDebug() << "Distance in X: " << Lx << Lx_m << Lx-Lx_m ;
	qDebug() << "Distance in Y: " << Ly << Ly_m << Ly-Ly_m ;
	qDebug() << "2D distance  : " << std::sqrt(Lx*Lx+Ly*Ly) - std::sqrt(Lx_m*Lx_m+Ly_m*Ly_m) ;
      }
      
      // Now do the chi2 minimization
      Eigen::Vector3d halfdchi2dpar   ;
      Eigen::Matrix3d halfd2chi2dpar2 ;
      for( const auto& r : recordings ) {
	// the residual is '2D'. however, x and y are
	// independent. so we could as well compute them separately.
	auto markerpos = r->markerposition() ;
	{
	  // first x
	  Eigen::Vector3d deriv ;
	  deriv(0) = 1 ;
	  deriv(1) = 0 ;
	  deriv(2) = - markerpos.y() ;  // -r sin(phi) = -y
	  double residual = r->measurement().globalcoordinates.x - markerpos.x() ;
	  halfdchi2dpar += residual * deriv ;
	  for(int irow=0; irow<3; ++irow)
	    for(int icol=0; icol<3; ++icol)
	      halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
	}
	{
	  // then y
	  Eigen::Vector3d deriv ;
	  deriv(0) = 0 ;
	  deriv(1) = 1 ;
	  deriv(2) = markerpos.x() ;  // r cos(phi) = x
	  double residual = r->measurement().globalcoordinates.y - markerpos.y() ;
	  halfdchi2dpar += residual * deriv ;
	  for(int irow=0; irow<3; ++irow)
	    for(int icol=0; icol<3; ++icol)
	      halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
	}
      }
      // now solve
      Eigen::Vector3d delta = halfd2chi2dpar2.ldlt().solve(halfdchi2dpar) ;
      qDebug() << "Solution: " << delta(0) << delta(1) << delta(2) ;
      
      // now update the geometry
      GeometrySvc::instance()->applyModuleDelta(delta(0),delta(1),delta(2)) ;
      m_cameraview->updateGeometryView() ;
      
      // and don't forget to reset
      m_marker1recorder->reset() ;
      m_marker2recorder->reset() ;
    } else {
      qWarning() << "Recordings not complete: "
		 << m_marker1recorder->status() << " "
		 << m_marker2recorder->status() ;
      // FIXME: pop up a dialog to say that you haven't collected both measurements yet.
    }
  }

  // helper class for page for alignment of the tiles
  AlignTilePage::AlignTilePage(PAP::CameraView* camview,
			       const char* marker1, const char* marker2)
  {
    auto vlayout = new QVBoxLayout{} ;
    this->setLayout(vlayout) ;
    auto movetomarker1button = new QPushButton{"Move to marker 1", this} ;
    connect(movetomarker1button,&QPushButton::clicked,[=](){ camview->moveCameraTo(marker1) ; } ) ;
    vlayout->addWidget( movetomarker1button ) ;
    
    auto movetomarker2button = new QPushButton{"Move to marker 2", this} ;
    connect(movetomarker2button,&QPushButton::clicked,[=](){ camview->moveCameraTo(marker2) ; } ) ;
    vlayout->addWidget( movetomarker2button ) ;
  }
}

