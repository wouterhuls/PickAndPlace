#include "AlignPages.h"
#include "MotionSystemSvc.h"
#include "GeometrySvc.h"
#include "CoordinateMeasurement.h"
#include "CameraView.h"
#include "CameraWindow.h"
#include "AutoFocus.h"
#include "TextEditStream.h"
#include "NominalMarkers.h"
#include "GraphicsItems.h"

#include <cmath>
#include "Eigen/Dense"
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QSignalSpy>
#include <QVideoProbe>

#include <opencv2/opencv.hpp>

namespace PAP
{
 
  // helper class for page for alignment of the main jig
  MarkerRecorderWidget::MarkerRecorderWidget(const QString& markername,
					     const PAP::CameraView* camview,
					     QWidget* parent)
    : QWidget(parent), m_cameraview(camview), m_status(Uninitialized)
  {
    setObjectName( markername ) ;
    auto hlayout = new QHBoxLayout{} ;
    setLayout(hlayout) ;
    auto movetomarker1button = new QPushButton{markername, this} ;
    connect(movetomarker1button,&QPushButton::clicked,[=](){ camview->moveCameraTo(markername, true) ; } ) ;
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
      // store the markerfocus for later use ?
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
	text << "Marker 1: " << m1->measurement().globalcoordinates.x() << "," << m1->measurement().globalcoordinates.y()  << std::endl
	     << "Marker 2: " << m2->measurement().globalcoordinates.x() << "," << m2->measurement().globalcoordinates.y() << std::endl
	     << "Distance in X: " << Lx << " " << Lx_m << " " << Lx-Lx_m << std::endl
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

  
  AlignMainJigPage::AlignMainJigPage(ViewDirection dir, PAP::CameraView* camview)
    : m_viewdirection(dir), m_cameraview(camview)
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
    auto autofindbutton = new QPushButton{"Find", this} ;
    connect(autofindbutton,&QPushButton::clicked,[=](){ m_triggerMarkerFinder=true ; } ) ;
    vlayout->addWidget( autofindbutton ) ;

    m_textbox = new QPlainTextEdit{this} ;
    hlayout->addWidget( m_textbox ) ;
    m_textbox->resize(300,100) ;
    m_textbox->appendPlainText("a. press the 'Marker1' button\n"
			       "b. take a recording\n"
			       "c. press the 'Marker2' button\n"
			       "d. take a recording\n"
			       "e. press the calibrate button\n"
			       "f. repeat steps a-e until you are satisfied\n") ;

    connect(camview->videoProbe(), SIGNAL(videoFrameProbed(QVideoFrame)),
	    this, SLOT(findMarker(QVideoFrame)));
     
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
      Eigen::Vector3d deltaXYPhi = computeAlignment(recordings,Coordinates2D{},m_textbox) ;
      // now update the geometry
      GeometrySvc::instance()->applyModuleDelta(m_viewdirection,deltaXYPhi(0),deltaXYPhi(1),deltaXYPhi(2)) ;
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

  void AlignMainJigPage::findMarker( const QVideoFrame& frame )
  {
    if( m_triggerMarkerFinder ) {
      m_triggerMarkerFinder = false ;
      // first call the 'map' to copy the contents to accessible memory
      const_cast<QVideoFrame&>(frame).map(QAbstractVideoBuffer::ReadOnly) ;
      if( frame.bits() && frame.pixelFormat() == QVideoFrame::Format_RGB32 ) {
	//QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
	//QImage img( frame.bits(),frame.width(),frame.height(),frame.bytesPerLine(),imageFormat);
	cv::Mat mat( frame.height(), frame.width(), CV_8UC4,
		     const_cast<uchar*>(frame.bits()), static_cast<size_t>(frame.bytesPerLine()) );
	// Officially, we still need to convert this to another format,
	// because the byte ordering is wrong. That may be not very
	// important for our application.
	// (see https://asmaloney.com/2013/11/code/converting-between-cvmat-and-qimage-or-qpixmap/)
	//cv::Mat  matNoAlpha;
	//cv::cvtColor( mat, matNoAlpha, cv::COLOR_BGRA2BGR ); // drop the all-white alpha channel
	// See https://docs.opencv.org/2.4/doc/tutorials/imgproc/imgtrans/hough_circle/hough_circle.html
	
	// Very soon we'll wamnt to crop the image, because I think that will save time once we are going to do a Hough transform
	// Setup a rectangle to define your region of interest
	//cv::Rect myROI(10, 10, 100, 100);
	
	// Crop the full image to that image contained by the rectangle myROI
	// Note that this doesn't copy the data
	// cv::Mat croppedImage = image(myROI);
	
	cv::Mat  src_gray ;
	/// Convert it to gray
	cv::cvtColor( mat, src_gray, cv::COLOR_BGRA2GRAY );
	
	/// Reduce the noise so we avoid false circle detection
	//cv::GaussianBlur( src_gray, src_gray, Size(9, 9), 2, 2 );
	
	std::vector<cv::Vec3f> circles;
	
	/// Apply the Hough Transform to find the circles
	const auto min_dist = 50; //src_gray.rows/8,
	const auto dp = 1 ; //: The inverse ratio of resolution
	const auto param_1 = 150; // Upper threshold for the internal Canny edge detector
	const auto param_2 = 10; // Threshold for center detection.
	const auto min_radius = 10; // Minimum radio to be detected. If unknown, put zero as default.
	const auto max_radius = 20; // Maximum radius to be detected. If unknown, put zero as default
	cv::HoughCircles( src_gray, circles, cv::HOUGH_GRADIENT, dp,min_dist,param_1,param_2,min_radius,max_radius);
	
	qDebug() << "Number of circles: " << circles.size() ;
	// now identify the correct circles. lot's more hardcoded numbers that we can just take from the geometry.
	std::vector<int> selectedcircles;
	bool success=false ;
	double x{0},y{0},phi{0} ;
	for( size_t i = 0; i < circles.size() && !success; ++i ) {
	  const auto ix = circles[i][0]  ;
	  const auto iy = circles[i][1]  ;
	  // find 1 circles that is at a distance of 100 from this one
	  for( size_t j = 0; j < i && !success; ++j ) {
	    const auto jx = circles[j][0]  ;
	    const auto jy = circles[j][1]  ;
	    const auto dx = ix - jx ; 
	    const auto dy = iy - jy ;
	    const double r = std::sqrt(dx*dx + dy*dy) ;
	    if( 95 < r && r < 105 ) {
	      selectedcircles.emplace_back(i) ;
	      selectedcircles.emplace_back(j) ;
	      // bingo. find the other two
	      const auto ijx = (ix+jx)/2 ;
	      const auto ijy = (iy+jy)/2 ;
	      size_t n{0} ;
	      auto sumx = ix+jx ;
	      auto sumy = iy+jy ;
	      for( size_t k = 0; k < circles.size(); ++k )
		if(k != i && k!=j ) {
		  const auto kx = circles[k][0]  ;
		  const auto ky = circles[k][1]  ;
		  const auto dx = kx - ijx ; 
		  const auto dy = ky - ijy ;
		  const double r = std::sqrt(dx*dx + dy*dy) ;
		  if( 45 < r && r < 55 ) {
		    sumx += kx ;
		    sumy += ky ;
		    ++n ;
		    selectedcircles.emplace_back(k) ;
		  }
		}
	      if( n == 2 ) {
		x = sumx/4 ;
		y = sumy/4 ;
		phi = std::atan2(dy,dx) ;
		success = true ;
		qDebug() << "Marker position is: " << x << "," << y ;
		//circle( src, cv::Point{int(x),int(y)}, 10, cv::Scalar(255,0,255), 3, 8, 0 );
		double sumr2{0} ;
		 for(int k=0; k<4; ++k) {
		   int l = selectedcircles[k] ;
		   sumr2 += std::pow(x - circles[l][0],2) + std::pow(y - circles[l][1],2) ;
		 }
		 qDebug() << "Average distance: " << std::sqrt(sumr2/4) ;
		 qDebug() << "Selected circles: "
			  << selectedcircles.size() << " : "
			  << selectedcircles ;
		 
		 // I wonder if we better do a chi2 fit, given that we know all dimensions
	      }
	    }
	  }
	}
	if(success) {
	  // create a graphics item to add to the geometry and draw it
	  if(!m_measuredmarker) {
	    m_measuredmarker = new MeasuredJigMarker("MeasuredMarker",m_cameraview->globalgeometry()) ;
	    //m_measuredmarker = new StackAxisMarker() ;
	    //new SightMarker( FiducialDefinition{"Measurement",0,0}, 10 ) ;
	    m_cameraview->globalgeometry()->addToGroup( m_measuredmarker ) ;
	    m_measuredmarker->setZValue(1) ;
	    m_measuredmarker->show() ;
	    m_cameraview->globalgeometry()->show() ;
	    m_cameraview->show() ;
	  }
	  
	  // now translate the pixel coordinates and angles to
	  // coordinates and angles in the global frame
	  // first create the transform in the pixel frame. See Cameraview::updateGeometryView for the inverse.
	  QTransform pixeltomarker;
	  pixeltomarker.translate(x,y) ;
	  pixeltomarker.rotateRadians(phi) ;
	  const auto geomsvc = GeometrySvc::instance() ;
	  // Now watch the order!
	  const QTransform T1 = geomsvc->fromCameraToGlobal() ;
	  const QTransform T2 = m_cameraview->fromCameraToPixel() ;
	  m_measuredmarker->setTransform( pixeltomarker * T2.inverted() * T1 ) ;
	  qDebug() << "Global coordinates: "
		   << m_measuredmarker->transform().m31()
		   << m_measuredmarker->transform().m32() ;
	  // let's auto move the camera
	  m_cameraview->moveCameraTo( QPointF{x,y}, CameraView::AbsolutePosition ) ;
	  
	}
      }
    }
  }
  
  // helper class for page for alignment of the tiles
  AlignTilePage::AlignTilePage(PAP::CameraView* camview, const TileInfo& tileinfo)
    : m_cameraview(camview), m_tilename(tileinfo.name)
  {
    auto hlayout = new QHBoxLayout{} ;
    this->setLayout(hlayout) ;
    
    auto vlayout = new QVBoxLayout{} ;
    hlayout->addLayout(vlayout) ;
    auto positionstackbutton = new QPushButton{"Position stack", this } ;
    connect(positionstackbutton,&QPushButton::clicked,
	    [=](){ GeometrySvc::instance()->positionStackForTile(m_tilename) ; } ) ;
    vlayout->addWidget( positionstackbutton ) ;

    m_marker1recorder = new MarkerRecorderWidget{ tileinfo.marker1, camview } ;
    vlayout->addWidget( m_marker1recorder ) ;
    m_marker2recorder = new MarkerRecorderWidget{ tileinfo.marker2, camview } ;
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


  /************************************/
  
  class AlignMainJigZPage : public QWidget
  {
  private:
    enum Status { Inactive, Active, Calibrated } ;
    ViewDirection m_viewdirection ;
    CameraWindow* m_camerasvc{0} ;
    Status m_status{Inactive} ;
    //const std::vector<MSMainCoordinates> refcoordinates{ {-100,-10},{-100,120},{80,120},{80,-10} } ;
    const std::vector<FiducialDefinition> m_refcoordinates ;
    std::vector<MSCoordinates> m_measurements ;
    std::vector<QMetaObject::Connection> m_conns ;
    QTableWidget* m_measurementtable{0} ;
    QPlainTextEdit* m_textbox{0} ;
  public:
    AlignMainJigZPage(ViewDirection view, CameraWindow& camerasvc) ;
  private slots:
    void disconnectsignals() ;
    void connectsignals() ;
    void move() ;
    void focus() ;
    void measure() ;
    void calibrate() ;
  } ;


  AlignMainJigZPage::AlignMainJigZPage(ViewDirection view,CameraWindow& camerasvc)
    : QWidget{&camerasvc},m_viewdirection{view},m_camerasvc(&camerasvc),
      m_refcoordinates{view==ViewDirection::NSideView ? PAP::Markers::jigSurfaceNSide() : PAP::Markers::jigSurfaceCSide()}
  {
    // just two buttons: start and stop + a label to tell that it is
    // ready, running, done or something like that.
    auto vlayout = new QVBoxLayout{} ;
    setLayout(vlayout) ;
    auto buttonlayout = new QHBoxLayout{} ;
    vlayout->addLayout(buttonlayout) ;
    auto startbutton = new QPushButton{"Start",this} ;
    connect(startbutton,&QPushButton::clicked,[&]() {
	//
	m_measurements.clear() ;
	m_status = Active ;
	// connect the signals
	this->connectsignals() ;
	// start the movement
	move() ;
      });
    buttonlayout->addWidget(startbutton) ;
    auto stopbutton = new QPushButton{"Abort",this} ;
    connect(stopbutton,&QPushButton::clicked,[&]() {
	// connect the signals
	this->disconnectsignals() ;
	// start the movement
	m_status = Inactive ;
      });
    buttonlayout->addWidget(stopbutton) ;
    {
      // temporary
      auto button = new QPushButton{"Focus",this} ;
      connect(button,&QPushButton::clicked,[&]() {
	  m_status = Active ;
	  this->focus() ;
	  m_status = Inactive ;
	  //m_camerasvc->autofocus()->startNearFocusSequence() ;
	});
      buttonlayout->addWidget(button) ;
    }
    // add a table with the results of the measurements
    auto hlayout = new QHBoxLayout{} ;
    vlayout->addLayout(hlayout) ;
    int nrow = m_refcoordinates.size() ;
    m_measurementtable = new QTableWidget{nrow,5,this} ;
    m_measurementtable->setHorizontalHeaderLabels(QStringList{"Main X","Main Y","Nominal Z","Focus Z","Residual"}) ;
    QTableWidgetItem prototype ;
    prototype.setBackground( QBrush{ QColor{Qt::gray} } ) ;
    prototype.setFlags( Qt::ItemIsSelectable ) ;
    const size_t N = m_refcoordinates.size();
    m_measurementtable->setRowCount( N ) ;
    for(size_t row=0; row<N; ++row ) {
      for(size_t col=0; col<5; ++col)
	if( !m_measurementtable->item( row,col) )
	  m_measurementtable->setItem(row,col,new QTableWidgetItem{prototype} ) ;
      const auto& r = m_refcoordinates[row] ;
      m_measurementtable->item(row,0)->setText( QString::number( r.x, 'g', 5 ) ) ;
      m_measurementtable->item(row,1)->setText( QString::number( r.y, 'g', 5 ) ) ;
      m_measurementtable->item(row,2)->setText( QString::number( r.z, 'g', 5 ) ) ;
    }
    hlayout->addWidget(m_measurementtable) ;
    
    m_textbox = new QPlainTextEdit{this} ;
    hlayout->addWidget( m_textbox ) ;
    m_textbox->resize(300,150) ;
    // connect( &(MotionSystemSvc::instance()->mainXAxis()), &MotionAxis::movementStopped,this, &AlignMainJigZPage::focus) ;
    // connect( &(MotionSystemSvc::instance()->mainYAxis()), &MotionAxis::movementStopped,this, &AlignMainJigZPage::focus) ;
    
  }

  void AlignMainJigZPage::disconnectsignals()
  {
    for( auto& c : m_conns ) QObject::disconnect(c) ;
    m_conns.clear() ;
  }
    
  void AlignMainJigZPage::connectsignals()
  {
    //m_conns.emplace_back( connect( MotionSystemSvc::instance(), &MotionSystemSvc::mainStageMoved, [&]() { this->focus() ; } ) ) ;
    Qt::ConnectionType type = Qt::QueuedConnection;
    m_conns.emplace_back( connect( &(MotionSystemSvc::instance()->mainXAxis()), &MotionAxis::movementStopped, this, [=]() { this->focus() ; },
				   type) ) ;
    m_conns.emplace_back( connect( &(MotionSystemSvc::instance()->mainYAxis()), &MotionAxis::movementStopped, this, [=]() { this->focus() ; },
				   type ) ) ;

    //m_conns.emplace_back( connect( &(MotionSystemSvc::instance()->mainXAxis()), &MotionAxis::movementStopped,this, &AlignMainJigZPage::focus) ) ;
    //m_conns.emplace_back( connect( &(MotionSystemSvc::instance()->mainYAxis()), &MotionAxis::movementStopped,this, &AlignMainJigZPage::focus)  ) ;

    
    m_conns.emplace_back( connect( m_camerasvc->autofocus(), &AutoFocus::focussed, this, [=]() { this->measure() ; },type ) ) ;
    m_conns.emplace_back( connect( m_camerasvc->autofocus(), &AutoFocus::focusfailed, this, [=]() {
	  this->disconnectsignals() ;
	  m_status = Inactive ;
	},type ) ) ;
  }
    
  void AlignMainJigZPage::move() {
    // move to next reference z position
    auto n = m_measurements.size() ;
    if( n<m_refcoordinates.size() ) {
      // need to translate from module coordinates to MS coordinates
      const auto& ref = m_refcoordinates[n] ;
      const auto geomsvc = GeometrySvc::instance() ;
      const QTransform fromModuleToGlobal = geomsvc->fromModuleToGlobal( m_viewdirection ) ;
      QPointF globalpos = fromModuleToGlobal.map( QPointF{ref.x,ref.y} ) ;
      qDebug() << "Module coordinates: " << QPointF{ref.x,ref.y} ;
      qDebug() << "Global coordinates: " << globalpos ;
      auto mscoordinates = geomsvc->toMSMain( globalpos ) ;
      MotionSystemSvc::instance()->mainXAxis().moveTo(mscoordinates.x) ;
      MotionSystemSvc::instance()->mainYAxis().moveTo(mscoordinates.y) ;
    }
  }
  
  void AlignMainJigZPage::focus() {
    qDebug() << "AlignMainJigZPage::focus: "
	     << MotionSystemSvc::instance()->mainXAxis().isMoving()
	     << MotionSystemSvc::instance()->mainYAxis().isMoving() ;
    if( m_status == Active &&
	!MotionSystemSvc::instance()->mainXAxis().isMoving() &&
	!MotionSystemSvc::instance()->mainYAxis().isMoving() ) {
      //emit m_camerasvc->autofocus()->focus() ;
      m_camerasvc->autofocus()->startFocusSequence(12.2,12.6) ;
      //MotionSystemSvc::instance()->focusAxis().move(0.2) ;
      //emit m_camerasvc->autofocus()->focussed() ;
    } ;
  }
  
  void AlignMainJigZPage::measure() {
    qDebug() << "AlignMainJigZPage::measure()" ;
    if( m_status == Status::Active ) {
      m_measurements.emplace_back(MotionSystemSvc::instance()->coordinates()) ;
      qDebug() << "Z-axis seen in 'measure': "
	       << m_measurements.back().focus
	       << MotionSystemSvc::instance()->focusAxis().position().value() ;
      int row = m_measurements.size()-1 ;
      const auto& m = m_measurements[row] ;
      m_measurementtable->item(row,3)->setBackground( QBrush{ QColor{Qt::green} } ) ;
      m_measurementtable->item(row,0)->setText( QString::number( m.main.x, 'g', 5 ) ) ;
      m_measurementtable->item(row,1)->setText( QString::number( m.main.y, 'g', 5 ) ) ;
      m_measurementtable->item(row,3)->setText( QString::number( m.focus, 'g', 5 ) ) ;
      if( m_measurements.size()==m_refcoordinates.size() ) {
	disconnectsignals() ;
	calibrate() ;
	// move the focus back to where the module is. for now, choose one of the markers.
	if(m_status ==  Calibrated)
	  m_camerasvc->autofocus()->moveFocusToModuleZ(0.0) ;
      } else {
	move() ;
      }
    }
  }

  void AlignMainJigZPage::calibrate() {
    qDebug() << "AlignMainJigZPage::calibrate()"
	     << m_measurements.size() << " " << m_refcoordinates.size() ;
    std::stringstream os ;
    os << "Measurements (main.x, main.y, focus):" << std::endl ;
    for( const auto& m : m_measurements ) 
      os << m.main.x << "," << m.main.y << "," << m.focus << std::endl ;
    qDebug() << os.str().c_str() ;
    // for now really simple:
    //   z = z0 + aX * x + aY * y
    Eigen::Vector3d halfdchi2dpar   = Eigen::Vector3d::Zero() ;
    Eigen::Matrix3d halfd2chi2dpar2 = Eigen::Matrix3d::Zero() ;
    for(unsigned int i=0; i<m_measurements.size(); ++i) {
      const auto& m   = m_measurements[i] ;
      const auto& ref =  m_refcoordinates[i] ;
      Eigen::Vector3d deriv ;
      deriv(0) = 1 ;
      deriv(1) = m.main.x ;
      deriv(2) = m.main.y ;
      double residual = ref.z + m.focus ; // take into account the factor -1 for focus
      halfdchi2dpar += residual * deriv ;
      for(int irow=0; irow<3; ++irow)
	for(int icol=0; icol<3; ++icol)
	  halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
    }
    Eigen::Vector3d delta = halfd2chi2dpar2.ldlt().solve(halfdchi2dpar) ;
    os << "Solution: " << delta(0) << ", " << delta(1) << ", " << delta(2) << std::endl ;
    qDebug() << os.str().c_str() ;
    // fill the column with residuals
    for(unsigned int i=0; i<m_measurements.size(); ++i) {
      const auto& m   = m_measurements[i] ;
      const auto& ref =  m_refcoordinates[i] ;
      double residual =  ref.z + m.focus - (delta(0) + delta(1)*m.main.x + delta(2)*m.main.y) ; 
      m_measurementtable->item(i,4)->setText( QString::number( residual, 'g', 5 ) ) ;
    }
    // sanity check
    if( std::abs(delta(1))<0.01 && std::abs(delta(2))<0.01 ) {
      // now we need to think waht we want the measurements to mean ...
      // ... which xy position?
      // ... what plane, or view?
      GeometrySvc::instance()->applyModuleZCalibration(m_viewdirection,delta(0),delta(1),delta(2)) ;
      m_status = Calibrated ;
      m_measurements.clear() ;
    } else {
      os << "Calibration failed!" << std::endl ;
    }
    qDebug() << os.str().c_str() ;
    m_textbox->appendPlainText( os.str().c_str() ) ;
  }
  
  QWidget* makeAlignMainJigZPage(ViewDirection view, CameraWindow& camerasvc) {
    return new AlignMainJigZPage(view,camerasvc) ;
  }
}

