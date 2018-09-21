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
#include <QPlainTextEdit>



namespace PAP
{
  // wrapper class that allows to stream to a qtextedit using stringstream
  class TextEditStream
  {
  private:
    QPlainTextEdit* m_textbox ;
  public:
    TextEditStream( QPlainTextEdit& textbox) : m_textbox(&textbox) {}
    template<typename T>
    TextEditStream& operator<<( const T& text) {
      std::stringstream os ;
      os << text ;
      m_textbox->appendPlainText( os.str().c_str() ) ;
      return *this ;
    }
  };
  
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
    
    auto recordcentrebutton = new QPushButton{"record centre", this} ;
    recordcentrebutton->setToolTip("Record the current centre of the camera view as the position of this marker.") ;
    hlayout->addWidget( recordcentrebutton ) ;
    connect(recordcentrebutton,&QPushButton::pressed,
	    [=](){ setStatus(Active) ; camview->record( camview->localOrigin()) ; } ) ;
    
    // catch measurement updates
    connect( camview, &CameraView::recording, this, &MarkerRecorderWidget::record ) ;
    
    // show a label with the status
    m_statuslabel = new QLabel{ this } ;
    setStatus( Uninitialized ) ;
    hlayout->addWidget( m_statuslabel ) ;
    
  }
  
  void MarkerRecorderWidget::record( const CoordinateMeasurement& m ) {
    if( m_status == Active ||
	m.markername == objectName() ) {
      m_markerposition = m_cameraview->globalPosition( objectName() ) ;
      //emit ready() ;
      qDebug() << "received measurement: " << objectName() << m.markername ;
      qDebug() << "global coordinates: (" << m.globalcoordinates.x() << "," << m.globalcoordinates.y() << ")" ;
      qDebug() << "marker position:      " << m_markerposition ;
      if( m_status != Uninitialized) {
	qDebug() << "Change in measurement: ("
		 << m_measurement.globalcoordinates.x() - m.globalcoordinates.x() << ","
		 << m_measurement.globalcoordinates.y() - m.globalcoordinates.y() << ")" ;
      }
      m_measurement = m ;
      setStatus( Recorded ) ;
      //auto T = GeometrySvc::instance()->fromModuleToGlobal(m_cameraview->currentViewDirection()).inverted() ;
      // qDebug() << "Measured position in module frame: "
      // 	       << T.map( QPointF{m.globalcoordinates} ) ;
      // qDebug() << "Nominal position in module frame:  "
      // 	       << T.map( QPointF{m_markerposition} ) ;
    }
  }
  
  void MarkerRecorderWidget::setStatus(Status s) {
    m_status = s ;
    if( m_status == Recorded )
      m_statuslabel->setText( "Recorded" ) ;
    else if( m_status == Active )
      m_statuslabel->setText( "Recording" ) ;
    else if( m_status == Calibrated )
      m_statuslabel->setText( "Calibrated" ) ;
    else
      m_statuslabel->setText( "Uninitialized" ) ;
  }

  namespace {

    // routine that computes dx,dy,dphi from a set of marker measurements
    Eigen::Vector3d computeAlignment( const std::vector<MarkerRecorderWidget* >& recordings,
				      Coordinates2D pivot = Coordinates2D{},
				      QPlainTextEdit* textbox=0)
    {
      // Let's first print the measured and expected distance between
      // the markers. This gives an idea of how accurate we can ever
      // be.
      std::stringstream text ;
      
      {
	const auto& m1 = recordings.front() ;
	const auto& m2 = recordings.back() ;
	double Lx = m1->markerposition().x() - m2->markerposition().x() ;
	double Ly = m1->markerposition().y() - m2->markerposition().y() ;
	double Lx_m = m1->measurement().globalcoordinates.x() - m2->measurement().globalcoordinates.x() ;
	double Ly_m = m1->measurement().globalcoordinates.y() - m2->measurement().globalcoordinates.y() ;
	text << "Distance in X: " << Lx << " " << Lx_m << " " << Lx-Lx_m << std::endl
	     << "Distance in Y: " << Ly << " " << Ly_m << " " << Ly-Ly_m << std::endl
	     << "2D distance  : " << std::sqrt(Lx*Lx+Ly*Ly) - std::sqrt(Lx_m*Lx_m+Ly_m*Ly_m) << std::endl ;
      }
      
      // Now do the chi2 minimization
      Eigen::Vector3d halfdchi2dpar   = Eigen::Vector3d::Zero() ;
      Eigen::Matrix3d halfd2chi2dpar2 = Eigen::Matrix3d::Zero() ;
      for( const auto& r : recordings ) {
	// the residual is '2D'. however, x and y are
	// independent. so we could as well compute them separately.
	auto markerpos = r->markerposition() ;
	{
	  // first x
	  Eigen::Vector3d deriv ;
	  deriv(0) = 1 ;
	  deriv(1) = 0 ;
	  deriv(2) = pivot.y() - markerpos.y() ;  // -r sin(phi) = -y
	  double residual = r->measurement().globalcoordinates.x() - markerpos.x() ;
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
	  deriv(2) = markerpos.x() - pivot.x() ;  // r cos(phi) = x
	  double residual = r->measurement().globalcoordinates.y() - markerpos.y() ;
	  halfdchi2dpar += residual * deriv ;
	  for(int irow=0; irow<3; ++irow)
	    for(int icol=0; icol<3; ++icol)
	      halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
	}
      }
      // now solve
      qDebug() << "First derivative: "
	       << halfdchi2dpar(0)
	       << halfdchi2dpar(1)
	       << halfdchi2dpar(2) ;
      qDebug() << "second derivative: "
	       << double(halfd2chi2dpar2(0,0))
	       << halfd2chi2dpar2(0,1)
	       << halfd2chi2dpar2(0,2) 
	       << halfd2chi2dpar2(1,0)
	       << halfd2chi2dpar2(1,1)
	       << halfd2chi2dpar2(1,2) 
	       << halfd2chi2dpar2(2,0)
	       << halfd2chi2dpar2(2,1)
	       << halfd2chi2dpar2(2,2) ;
	
      Eigen::Vector3d delta = halfd2chi2dpar2.ldlt().solve(halfdchi2dpar) ;
      text << "Solution: dx=" << delta(0) << " dy=" << delta(1) << " dphi=" << delta(2) << std::endl ;
      if(textbox) {
        TextEditStream{*textbox} << text.str() ;
      }
      else
	qDebug() << text.str().c_str() ;
      return delta ;
    }
  }

  
  AlignMainJigPage::AlignMainJigPage(PAP::CameraView* camview)
    : m_cameraview(camview)
  {
    auto hlayout = new QHBoxLayout{} ;
    this->setLayout(hlayout) ;
    
    auto vlayout = new QVBoxLayout{} ;
    hlayout->addLayout(vlayout) ;
    m_marker1recorder = new MarkerRecorderWidget{ "MainJigMarker1", camview } ;
    vlayout->addWidget( m_marker1recorder ) ;
    m_marker2recorder = new MarkerRecorderWidget{ "MainJigMarker2", camview } ;
    vlayout->addWidget( m_marker2recorder ) ;
    auto calibrationbutton = new QPushButton{"Calibrate", this} ;
    connect(calibrationbutton,&QPushButton::clicked,[=](){ this->updateAlignment() ; } ) ;
    vlayout->addWidget( calibrationbutton ) ;

    m_textbox = new QPlainTextEdit{this} ;
    hlayout->addWidget( m_textbox ) ;
    m_textbox->resize(300,100) ;
    m_textbox->appendPlainText("a. press the 'Marker1' button\n"
			       "b. take a recording\n"
			       "c. press the 'Marker2' button\n"
			       "d. take a recording\n"
			       "e. press the calibrate button\n"
			       "f. repeat steps a-e until you are satisfied\n") ;
    //QFont font = tmptext1->font();
    //font.setPointSize(2);
    //font.setBold(true);
    //tmptext1->setFont(font);
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
    if( m_marker1recorder->status() == MarkerRecorderWidget::Recorded &&
	m_marker2recorder->status() == MarkerRecorderWidget::Recorded ) {
      std::vector< MarkerRecorderWidget* > recordings = { m_marker1recorder, m_marker2recorder } ;
      Eigen::Vector3d delta = computeAlignment(recordings,Coordinates2D{},m_textbox) ;
      // now update the geometry
      GeometrySvc::instance()->applyModuleDelta(delta(0),delta(1),delta(2)) ;
      m_cameraview->updateGeometryView() ;
      // and don't forget to reset
      m_marker1recorder->setStatus( MarkerRecorderWidget::Calibrated ) ;
      m_marker2recorder->setStatus( MarkerRecorderWidget::Calibrated ) ;
    } else {
      qWarning() << "Recordings not complete: "
		 << m_marker1recorder->status() << " "
		 << m_marker2recorder->status() ;
      // FIXME: pop up a dialog to say that you haven't collected both measurements yet.
    }
  }

  // helper class for page for alignment of the tiles
  AlignTilePage::AlignTilePage(PAP::CameraView* camview,
			       const char* tilename,
			       const char* marker1, const char* marker2)
    : m_cameraview(camview), m_tilename(tilename)
  {
    auto hlayout = new QHBoxLayout{} ;
    this->setLayout(hlayout) ;
    
    auto vlayout = new QVBoxLayout{} ;
    hlayout->addLayout(vlayout) ;
    auto positionstackbutton = new QPushButton{"Position stack", this } ;
    connect(positionstackbutton,&QPushButton::clicked,
	    [=](){ GeometrySvc::instance()->positionStackForTile(m_tilename) ; } ) ;
    vlayout->addWidget( positionstackbutton ) ;

    m_marker1recorder = new MarkerRecorderWidget{ marker1, camview } ;
    vlayout->addWidget( m_marker1recorder ) ;
    m_marker2recorder = new MarkerRecorderWidget{ marker2, camview } ;
    vlayout->addWidget( m_marker2recorder ) ;
    
    auto calibrationbutton = new QPushButton{"Calibrate", this} ;
    connect(calibrationbutton,&QPushButton::clicked,[=](){ this->updateAlignment() ; } ) ;
    vlayout->addWidget( calibrationbutton ) ;

    m_textbox = new QPlainTextEdit{this} ;
    hlayout->addWidget( m_textbox ) ;
    m_textbox->resize(300,100) ;
    m_textbox->appendPlainText("a. press the 'position stack' button\n"
			       "b. press the button for the first marker\n"
			       "c. take a recording. YOU MAY MOVE THE MAIN STAGE BUT NOT THE STACK\n"
			       "d. do the same for the 2nd marker\n"
			       "e. press the calibrate button\n"
			       "f. press again the 'position stack' button\n"
			       "g. repeat steps b-f until you are satisfied\n") ;
    
    //auto tmptext1 = new QLabel{this} ;
    // tmptext1->setText("a. press the 'position stack' button\n"
    // 		      "b. press the button for the first marker\n"
    // 		      "c. take a recording. YOU MAY MOVE THE MAIN STAGE BUT NOT THE STACK\n"
    // 		      "d. do the same for the 2nd marker\n"
    // 		      "e. press the calibrate button\n"
    // 		      "f. press again the 'position stack' button\n"
    // 		      "g. repeat steps b-f until you are satisfied\n") ;
    // tmptext1->setWordWrap(true);
    // QFont font = tmptext1->font();
    // //font.setPointSize(2);
    // font.setBold(true);
    // tmptext1->setFont(font);
    // hlayout->addWidget( tmptext1 ) ;
    
  }
  
  void AlignTilePage::updateAlignment() const
  {
    // these parameter essentially make a transform and we need to
    // update that transform.
    //
    // I decided to do everything in the 'global' frame. we then simply compute dx,dy and phi, and finally update the transform:
    if( m_marker1recorder->status() == MarkerRecorderWidget::Recorded &&
	m_marker2recorder->status() == MarkerRecorderWidget::Recorded ) {
      std::vector< MarkerRecorderWidget* > recordings = { m_marker1recorder, m_marker2recorder } ;
      // the best thing is to compute a transform in the global frame,
      // then work from there. However, for now I'll compute with
      // respect to the known pivot point, which is the axis in the
      // global frame. We can do it more correctly later.
      
      Coordinates2D pivot = GeometrySvc::instance()->stackAxisInGlobal() ;
      Eigen::Vector3d delta = computeAlignment(recordings,pivot,m_textbox) ;
      // Fixme: this does not take the stack calibration into account
      // (apart from the position of the stack axis)
      GeometrySvc::instance()->applyStackDeltaForTile( m_tilename, delta(0), delta(1), delta(2) ) ;
      
      // this is in the global frame. all we now need to do, is
      // compute it in the stack frame (with respect to the stack
      // axis). given that I already have a transform for that, it
      // shouldn't actually be very hard.

      // now update the geometry
      //GeometrySvc::instance()->applyModuleDelta(delta(0),delta(1),delta(2)) ;

      //m_cameraview->updateStackAxisView() ;
      // Fix me: perhaps we would directly like to call the 'moveStack' thingy here
      
      // and don't forget to reset
      m_marker1recorder->setStatus( MarkerRecorderWidget::Calibrated ) ;
      m_marker2recorder->setStatus( MarkerRecorderWidget::Calibrated ) ;
    } else {
      qWarning() << "Recordings not complete: "
		 << m_marker1recorder->status() << " "
		 << m_marker2recorder->status() ;
      // FIXME: pop up a dialog to say that you haven't collected both measurements yet.
    }
  }

  
}

