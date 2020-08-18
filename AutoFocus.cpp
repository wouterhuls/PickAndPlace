#include "AutoFocus.h"
#include "CameraView.h"
#include "Eigen/Dense"
#include <QSignalSpy>
#include <QVideoProbe>
#include <QVideoFrame>
#include <QPixmap>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QScatterSeries>

#include <cmath>
#include "MotionSystemSvc.h"
#include "GeometrySvc.h"
#include "PropertySvc.h"
#include "NamedValueInputWidget.h"

namespace PAP
{
  struct AutoFocusSettingsWidget : public QDialog
  {
    NamedValue<double> zmin{"AutoFocus.ZMin",23.0} ;  // Minimal z for focus search
    NamedValue<double> zmax{"AutoFocus.ZMax",24.5} ;  // Maximal z for focus search
    NamedValue<double> movespeed{"AutoFocus.MoveVelocity",0.4} ;
    NamedValue<double> fastspeed{"AutoFocus.FastSearchVelocity",0.1} ;
    NamedValue<double> slowspeed{"AutoFocus.SlowSearchVelocity",0.02} ;

    AutoFocusSettingsWidget( QWidget *parent )
      : QDialog{parent}
    {
      this->setWindowTitle( "AutoFocusSettings" ) ;
      //auto layout = new QGridLayout{} ;
      auto layout = new QVBoxLayout{} ;
      layout->setContentsMargins(0, 0, 0, 0);
      this->setLayout(layout) ;
      
      layout->addWidget( new NamedValueInputWidget<double>{zmin,18.0,25.0,2} ) ;
      layout->addWidget( new NamedValueInputWidget<double>{zmax,18.0,26.0,2} ) ;
      layout->addWidget( new NamedValueInputWidget<double>{slowspeed,0,0.4,2} ) ;
      layout->addWidget( new NamedValueInputWidget<double>{fastspeed,0,0.4,2} ) ;
      layout->addWidget( new NamedValueInputWidget<double>{movespeed,0,0.4,2} ) ;

    }
  } ;

  namespace {
    QString turnjigMarkerFocusSuffix( int jigversion, int viewdir )
    {
      QString rc ;
      rc.append( jigversion == GeometrySvc::TurnJigVersion::VersionA ? ".JigA" : ".JigB" ) ;
      rc.append( viewdir == ViewDirection::NSideView ? ".NSide" : ".CSide" ) ;
      return rc ;
    }
  }
  
  AutoFocus::AutoFocus(CameraView* camview, QWidget* parent)
    : QDialog(parent), m_cameraView(camview),
      m_settings{ new AutoFocusSettingsWidget{this} },
      m_zaxis{&MotionSystemSvc::instance()->focusAxis()}
  {
    qRegisterMetaType<PAP::FocusMeasurement>() ;
    
    connect(camview->videoProbe(), SIGNAL(videoFrameProbed(QVideoFrame)),
	    this, SLOT(processFrame(QVideoFrame)));
    connect( m_zaxis, &MotionAxis::movementStopped,
	     [=]{this->m_focusTriggered = true ; } ) ;
    connect(&(m_zaxis->position()),&NamedValueBase::valueChanged,[&]{this->m_focusTriggered = true ; }) ;
    connect( this, &AutoFocus::focus, [&](){ /*if(!isFocussing())*/ this->startNearFocusSequence() ; } ) ;

    // QObject::connect(camview->videoProbe(), &QVideoFrame::videoFrameProbed,
    // 		     this, 

    m_focusImage = new QImage{200,200,QImage::Format_Grayscale8} ;
    //m_focusImage = new QImage{2448,2048,QImage::Format_Grayscale8} ;

    m_focusView = new QLabel{this} ;
    m_focusView->setPixmap(QPixmap::fromImage(*m_focusImage));
    
    this->resize(600,200) ;
    this->move(50,500) ;

    auto vlayout = new QVBoxLayout{} ;
    this->setLayout( vlayout) ;
   
    auto hlayout = new QHBoxLayout{} ;
    hlayout->addWidget( m_focusView ) ;
    vlayout->addLayout( hlayout ) ;

    m_focusmeasurements = new QtCharts::QScatterSeries() ;
    m_focusmeasurements->append( -1,1 ) ;
    m_focusmeasurements->append(  0,0 ) ;
    m_focusmeasurements->append(  1,1 ) ;
    
    m_focuschart = new QtCharts::QChart() ;
    m_focuschart->addSeries( m_focusmeasurements ) ;
    m_focuschart->createDefaultAxes();
    m_chartview = new QtCharts::QChartView(m_focuschart);
    m_chartview->setRenderHint(QPainter::Antialiasing);
    hlayout->addWidget( m_chartview ) ;
    
    //auto label = new QLabel{"Here we should add a graph", this} ;
    //hlayout->addWidget( label ) ;
    
    // Fix me, to implement here:
    // - every time the z-axis stops, you want to take a focusmeasurement and put it in the chart
    // - we need buttons for moving the axis, but that can be done later
    // - we need a button to clear the set of measurements
    // - we need a button to start the autofocus session. the other button just opens the focus window
    // - we need a button to hide the focus window

    auto buttonlayout = new QHBoxLayout{} ;
    vlayout->addLayout( buttonlayout ) ;

    //auto startfocusbutton = new QPushButton{"AutoFocus", this} ;
    //connect( startfocusbutton,  &QPushButton::clicked, [=](){ this->startFocusSequence() ; } ) ;
    //buttonlayout->addWidget( startfocusbutton ) ;
    
    auto startfocussearchbutton = new QPushButton{"Search", this} ;
    connect( startfocussearchbutton,  &QPushButton::clicked, [=](){ this->startFastFocusSequenceSimple() ; } ) ;
    buttonlayout->addWidget( startfocussearchbutton ) ;

    {
      auto button = new QPushButton{"NearSearch", this} ;
      //connect( button,  &QPushButton::clicked, [=](){ this->startNearFocusSequence() ; } ) ;
      connect( button,  &QPushButton::clicked, [=](){
	  const double focus = currentFocus() ;
	  this->startFocusSequence(focus-0.05,focus+0.05) ; } ) ;
      buttonlayout->addWidget( button ) ;
    }
    
    auto clearbutton = new QPushButton{"Clear", this} ;
    connect( clearbutton,  &QPushButton::clicked, [=](){ this->m_focusmeasurements->clear() ; } ) ;
    buttonlayout->addWidget( clearbutton ) ;

    auto storebutton = new QPushButton{"Store", this} ;
    connect( storebutton,  &QPushButton::clicked, [&](){ this->storeMarkerFocus() ; } ) ;
    buttonlayout->addWidget( storebutton ) ;

    auto settingsbutton = new QPushButton(QIcon(":/images/settings.png"),"",this) ;
    connect( settingsbutton, &QPushButton::clicked, [&](){ m_settings->show() ; } ) ;
    buttonlayout->addWidget(settingsbutton);
    
    auto hidebutton = new QPushButton{"Hide", this} ;
    connect( hidebutton,  &QPushButton::clicked, [=](){ this->hide() ; } ) ;
    buttonlayout->addWidget( hidebutton ) ;

    

    // fill markerfocuspoint map
    // double m_zpositionNSideJigMarker1 = 24.675 ; // 
    // double m_zpositionNSideJigMarker2 = 24.465 ; // 
    // double m_zpositionCSideJigMarker1 = 23.243 ; // 
    // double m_zpositionCSideJigMarker2 = 23.223 ; // 
    // double m_zpositionVelopixMarker   = 23.610 ;
    
    const QString prefix = "Focus." ;
    const auto& geosvc = GeometrySvc::instance() ;
    for( const auto& m : geosvc->jigmarkers() )
      for( int iside=0; iside<2; ++iside )
	for(int iversion=0; iversion<2; ++iversion ) {
	  QString name = m.name + turnjigMarkerFocusSuffix( iversion, iside ) ;
	  m_markerfocuspoints.insert( { name, NamedDouble{prefix + name, 24.} } ) ;
	}
    for( const auto& m : geosvc->velopixmarkersNSide() )
      m_markerfocuspoints.insert( {m.name,NamedDouble{prefix + m.name,24.} } ) ;
    for( const auto& m : geosvc->velopixmarkersCSide() )
      m_markerfocuspoints.insert( {m.name,NamedDouble{prefix + m.name,24.} } ) ;
    for( const auto& m : geosvc->mcpointsNSide() )
      m_markerfocuspoints.insert( {m.name,NamedDouble{prefix + m.name,24.} } ) ;
    for( const auto& m : geosvc->mcpointsCSide() )
      m_markerfocuspoints.insert( {m.name,NamedDouble{prefix + m.name,24.} } ) ;

    // auto currentviewdirection = m_cameraView->currentViewDirection();
    // for(int i=0; i<2; ++i) {
    //   QString prefix = i==0 ? "Focus.NSide." : "Focus.CSide." ;
    //   m_cameraView->setViewDirection( i==0 ? ViewDirection::NSideView :  ViewDirection::CSideView) ;
    //   auto markers = m_cameraView->visibleMarkers() ;
    //   for( const auto& m : markers ) {
    // 	auto it = m_markerfocuspoints[i].insert( {m,NamedDouble{prefix + m,24.0} } ) ;
    // 	PropertySvc::instance()->add( it.first->second ) ;
    //   }
    // }
    // m_cameraView->setViewDirection( currentviewdirection ) ;
    // for(int i=0; i<2; ++i)
    for(auto& it: m_markerfocuspoints)
      PropertySvc::instance()->add( it.second ) ;
  }

  AutoFocus::~AutoFocus() {}

  void AutoFocus::moveFocusTo( double focus ) const
  {
    m_zaxis->moveTo( focus ) ;
  }

  double AutoFocus::focusFromZ( double z ) const
  {
    return GeometrySvc::instance()->moduleZ( m_cameraView->currentViewDirection(), 0 ) - z ;
  }

  double AutoFocus::zFromFocus( double focus ) const
  {
    return GeometrySvc::instance()->moduleZ( m_cameraView->currentViewDirection(), focus ) ;
  }

  double AutoFocus::currentFocus() const
  {
    return m_zaxis->position().value() ;
  } ;
  
  void AutoFocus::moveFocusToModuleZ( double z ) const
  {
    qDebug() << "AutoFocus::moveFocusToModuleZ: " << z << focusFromZ( z ) << zFromFocus(focusFromZ( z )) ;
    moveFocusTo( focusFromZ( z ) ) ;
  }

  void AutoFocus::storeMarkerFocus()
  {
    storeMarkerFocus( m_cameraView->closestMarkerName() ) ;
  }

  void AutoFocus::storeMarkerFocus(const QString& name)
  {
    storeMarkerFocus(name, m_zaxis->position() ) ;
  }

  void AutoFocus::storeMarkerFocus(const QString& markername, double focus)
  {
    QString name = markername ;
    if( markername.contains( "MainJigMarker") )
      name.append(turnjigMarkerFocusSuffix( GeometrySvc::instance()->turnJigVersion(), m_cameraView->currentViewDirection() )) ;
    auto it = m_markerfocuspoints.find( name ) ;
    if( it != m_markerfocuspoints.end() ) it->second.setValue( focus ) ;
  }
  
  void AutoFocus::applyMarkerFocus(const QString& markername) const
  {
    QString name = markername ;
    if( markername.contains( "MainJigMarker") )
      name.append(turnjigMarkerFocusSuffix( GeometrySvc::instance()->turnJigVersion(), m_cameraView->currentViewDirection() )) ;
    auto it = m_markerfocuspoints.find( name ) ;
    if( it != m_markerfocuspoints.end() )
      moveFocusTo( it->second.value()  ) ;
  }

  void AutoFocus::applyMarkerFocus() const
  {
    auto closestmarker = m_cameraView->closestMarkerName() ;
    applyMarkerFocus( closestmarker) ;
  }
  
  void AutoFocus::processFrame( const QVideoFrame& frame )
  {
    // argument needs to be const, because VideoFrameProbed has a const signal!
    if( m_focusTriggered ) {
      computeContrast(frame) ;
      m_focusTriggered = false ;
    }
  }

  double AutoFocus::computeContrast( const QVideoFrame& frame )
  {
    /*qDebug()
      << "Pointer to frame B: "
      << m_frame << " "
      << &frame ; */
    // right now we'll 'just' compute the contrast in a 100x100 pixel
    // size area around the center.
    //
    // one standard measure is 'constrast per pixel'
    // * 8 copmute the difference between a pixel and it's 8 neighours
    // * all up all of those in squares
    //
    
    // I am not sure what is a fast way to do this, but we should
    // definitely use as few points as we can. it probably also makes
    // sense to first copy the data such that it fits in the cache.

    // I thikn that I need to use the colours in "HSV" mode. The V componess is the brightness, which is probablaby good for computing a contract. I cna also convert to grayscale.

    // rather than 'contrast per pixel', we can also use 'entropy'
    // E = - sum p * log2(p)
    // where the sum runs over all pixels and p is again intensity.

    // first call the 'map' to copy the contents to accessible memory
    //qDebug() << "Before calling QVideoFrame::map" ;
    const_cast<QVideoFrame&>(frame).map(QAbstractVideoBuffer::ReadOnly) ;
    //qDebug() << "After calling QVideoFrame::map" ;
    // My laptop camera uses "UYVY", which means that UVY for two
    // adjacent pixels is stored with common U and V values. The Y
    // value is for brightness, which is all we need here. Of course,
    // as long as the camera anyway doesn't work, it makes little
    // sense to adapt the code for this.
    // qDebug() << "VideoFrame: "
    // 	     << frame.size() << " "
    // 	     << frame.width() << " "
    // 	     << frame.height() << " "
    // 	     << frame.bytesPerLine() << " "
    // 	     << frame.mappedBytes() << " "
    // 	     << frame.pixelFormat() ;
    double rc = 0 ;
    if( frame.bits() &&
	//(frame.pixelFormat() == QVideoFrame::Format_BGR32||
	frame.pixelFormat() == QVideoFrame::Format_RGB32 ) {
      /*
      // the following should also allow to change the format
      QImage::Format imageFormat =
      QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
      QImage img( frame.bits(),
      frame.width(),
      frame.height(),
      frame.bytesPerLine(),
      imageFormat);
      */
      
      QImage::Format imageFormat =
	QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
      QImage img( frame.bits(),
		  frame.width(),
		  frame.height(),
		  frame.bytesPerLine(),
		  imageFormat);
      
      
      const int centralPixelX = frame.width()/2 ;
      const int centralPixelY = frame.height()/2 ;
      const int numPixelX = m_focusImage->width() ; //200 ; // 60x60 microns. ?
      const int numPixelY = m_focusImage->height() ;//200 ;
      const int firstPixelX = centralPixelX - numPixelX/2 ;
      const int lastPixelX  = firstPixelX + numPixelX ;
      const int firstPixelY = centralPixelY - numPixelY/2 ;
      const int lastPixelY  = firstPixelY + numPixelY ;

      // according to this article:
      // https://www.google.nl/url?sa=t&rct=j&q=&esrc=s&source=web&cd=4&ved=0ahUKEwj_2q7N9K7XAhWDyaQKHQVEBS0QFghCMAM&url=http%3A%2F%2Fciteseerx.ist.psu.edu%2Fviewdoc%2Fdownload%3Fdoi%3D10.1.1.660.5197%26rep%3Drep1%26type%3Dpdf&usg=AOvVaw1ID-dlE4Ji72E9knlQdzsr
      // just the (normalized) variance is the best focussing criterion. It is also very easy to compute ...

       // fill histogram with intensity values
      double histogram[256] ;
      for(int i=0; i<256; ++i) histogram[i]=0 ;
      unsigned char grid[numPixelX*numPixelY] ;
      const unsigned int* bgr32 = reinterpret_cast<const unsigned int*>(frame.bits()) ;
      unsigned char* imagebits = reinterpret_cast<unsigned char*>(m_focusImage->bits()) ;
      // for now we'll just use entropy. try fancier things later.
      for(int ybin = firstPixelY ; ybin<lastPixelY ; ++ybin) {
	int xbin0 = ybin*frame.width() ;
	for(int xbin = firstPixelX ; xbin<lastPixelX ; ++xbin) {
	  unsigned int bbggrrff = bgr32[ xbin0+xbin ] ;
	  //unsigned char F  = (bbggrrff >> 24 ) & 0xff ;
	  unsigned char R  = (bbggrrff >> 16 ) & 0xff ;
	  unsigned char G  = (bbggrrff >> 8 )  & 0xff ;
	  unsigned char B  = bbggrrff & 0xff ;
	  //if( ybin==firstPixelY && xbin==firstPixelX)
	  //std::cout << "B,G,R: " << int(B) << " " << int(R) << " " << int(G) << " " << int(F) << std::endl ;
	  // many ways to turn this into an intensity. 
	  //https://stackoverflow.com/questions/687261/converting-rgb-to-grayscale-intensity
	  // https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color
	  // onw that runs very quickly is:
	  unsigned char Y = (R+R+R+B+G+G+G+G)>>3 ;
	  
	  histogram[Y] += 1 ;
	  grid[ xbin-firstPixelX + numPixelX * (ybin-firstPixelY)] = Y ;
	  imagebits[ xbin-firstPixelX + numPixelX * (ybin-firstPixelY) ] = Y ;
	}
      }
      const double norm = numPixelX*numPixelY ;
      // compute Boddeke's measure (gradient just in x direction, skipping bins (not sure why))
      double BODsum(0) ;
      double normBODsum(0) ;
      for(int ybin=0; ybin<numPixelY; ++ybin)
	for(int xbin=0; xbin<numPixelX-2; ++xbin) {
	  int bin = ybin*numPixelX + xbin ;
	  double tmp = grid[ bin + 2] - grid[ bin ] ;
	  BODsum += tmp*tmp ;
	  // same but now normalize to a variance that you estimate from how far the midpoint is off:
	  double res = 0.5*(grid[ bin + 2] + grid[ bin ]) - grid[bin+1] ;
	  normBODsum += tmp*tmp / (1.0 + res*res) ;
	}
      BODsum /= norm ;
      normBODsum /= norm ;
      // compute my measure (gradient in xy direction)
      // we should be able to write this a bit quicker
      double WHsum(0) ;
      for(int ybin=0; ybin<numPixelY-1; ++ybin) {
	short n11 = grid[ ybin*numPixelX ] ;
	short n12 = grid[ (ybin+1)*numPixelX ] ;
	for(int xbin=0; xbin<numPixelX-1; ++xbin) {
	  short n21 = grid[ ybin*numPixelX + xbin + 1] ;
	  short n22 = grid[ (ybin+1)*numPixelX + xbin + 1] ;
	  //float dx = (float(n11) + float(n12) - float(n21) - float(n22)) ;
	  //float dy = (float(n11) + float(n21) - float(n12) - float(n22)) ;
	  //WHsum += dx*dx+dy*dy ;
	  short a = n11 - n22 ;
	  short b = n12 - n21 ;
	  WHsum += a*a + b*b ;
	  n11 = n21 ;
	  n12 = n22 ;
	}
      }
      WHsum /= norm ;
      
      // compute the variance. for best precision, first compute the mean ...
      double sumX(0) ;
      for(int i=0; i<256; ++i) sumX += histogram[i] * i ;
      double mu = sumX / norm ;
      double var(0) ;
      for(int i=0; i<256; ++i) var += histogram[i] * ( (i-mu)*(i-mu) ) ;
      var = var / norm ;
           
      // compute entropy
      double entropy = 0 ;
      for(int i=0; i<256; ++i)
	if( histogram[i]>0 )
	  entropy -= histogram[i]/norm * std::log2(histogram[i]/norm) ;
      //qDebug() << "Entropy = "
      //<< entropy ;
      // unmap in order to free the memory
      const_cast<QVideoFrame&>(frame).unmap() ;
      rc = entropy ;
      rc = var/mu ;
      rc = WHsum/mu ;

      /*
      qDebug() << "Pos, mean, variance, entropy: "
	       << MotionSystemSvc::instance()->focusAxis().position() 
	       << mu << " " << var << " " << std::sqrt(var) << " "
	       << var/mu << " " << entropy  << " "
	       << BODsum << " "
	       << normBODsum << " "
	       << WHsum ;
      */
      // what works is: sqrt(var), entropy and WHsum!
      
      
      m_focusView->setPixmap(QPixmap::fromImage(*m_focusImage));
      //m_focusView->setPixmap(QPixmap::fromImage(img));

      FocusMeasurement focusmeasurement ;
      focusmeasurement.I = rc ;
      focusmeasurement.z    = m_zaxis->position().value() ;
      focusmeasurement.timestamp = m_zaxis->position().timestamp() ;
      focusmeasurement.zdir = m_zaxis->direction() ;
      emit focusMeasurement( focusmeasurement ) ;
      if( m_focusmeasurements->count()/10==0 )
	qDebug() << "Number of focus measurements: "
		 << m_focusmeasurements->count() ;
      m_focusmeasurements->append( focusmeasurement.z, rc ) ;

      m_focuschart->removeSeries( m_focusmeasurements ) ;
      m_focuschart->addSeries( m_focusmeasurements ) ;
      
      m_focuschart->createDefaultAxes();
      m_chartview->repaint() ;
    }
    
    emit focusMeasureUpdated() ;
    return rc ;
  }

  namespace {
    bool testseries( const std::vector< FocusMeasurement >& mvecorig, const double zvelocity)
    {
      // this routine tests how many measurements are missing. we
      // don't really know the frequency, but we can compare the
      // average interval, to the maximum interval. If the ratio is
      // too odd, then something is wrong.

      if( mvecorig.size()>2 ) {
	// for some reason the first measurement is always odd. so let's
	// leave that out. it is probably beause we start taking data
	// before the axis moves.
	const std::vector< FocusMeasurement > mvec( mvecorig.begin()+1, mvecorig.end() ) ;
      	const auto z0 = mvec.front().z ;
	const auto z1 = mvec.back().z ;
	const auto t0 = mvec.front().timestamp ;
	const auto t1 = mvec.back().timestamp;
	const double dtinseconds = 1e-3*t0.msecsTo(t1) ;
	// check that the velocity makes sense
	qDebug() << "AutoFocus testseries: Velocity = " << zvelocity << " " << std::abs((z1-z0)/dtinseconds) ;
	if( std::abs(std::abs((z1-z0)/dtinseconds)/zvelocity -1 ) > 0.2 ) {
	  qWarning() << "AutoFocus testseries: Bad velocity: "
		     << t0 << t1 << dtinseconds << z1 << z0 ;
	  for( const auto& m: mvec)
	    qDebug() << m.timestamp << m.z ;
	  return false ;
	}
	
	double maxinterval{0} ;
	for( size_t i=0; i<mvec.size()-1; ++i) {
	  const double thisdtinseconds = 1e-3*std::abs(mvec[i].timestamp.msecsTo( mvec[i+1].timestamp )) ;
	  if( thisdtinseconds > maxinterval ) maxinterval = thisdtinseconds ;
	}
	const double averageinterval = dtinseconds / mvec.size() ; 
	qDebug() << "Average/max interval: "
		 << averageinterval << " " << maxinterval ;
	if( maxinterval > 2* averageinterval ) {
	  qWarning() << "Maximum interval too large: " ;
	  for( const auto& m: mvec)
	    qDebug() << m.timestamp ;
	  return false ;
	}
      } else {
	qDebug() << "AutoFocus testseries: Not enough measurement: " << mvecorig.size() ;
	return false ;
      }
      return true ;
    }
    
    
    bool estimatez0( const std::vector< FocusMeasurement >& mvec, double& z0 )
    {
      // return true if success
      
      // let's do this as if we are fitting parameters of a
      // parabola. (we should really use Minuit, but will keep that
      // for later.) we want the fit to be linear, so don't try
      // anything smart: just choose
      //    I = [0] + [1]*(z-zref) + [2]*(z-zref)^2
      // initial values are all zero
      // double chi2(0) ;
      bool success = false ;
      if( mvec.size() >2 ) {
	Eigen::Vector3d halfdchi2dpar{Eigen::Vector3d::Zero()}   ;
	Eigen::Matrix3d halfd2chi2dpar2{Eigen::Matrix3d::Zero()} ;
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
	Eigen::Vector3d resid = halfdchi2dpar - halfd2chi2dpar2 * pars ;
	qDebug() << "Resid: " << resid(0) << resid(1) << resid(2) ;     
	qDebug() << "Solution: " << pars(0) << " " << pars(1) << " " << pars(2)
		 << "---> " << zref - 0.5*pars(1)/pars(2) ;
	qDebug() << "Fitting to measurements: " ;
	for( const auto& m : mvec )
	  qDebug() << m.z << " " << m.I
		   << ( pars(0) + (m.z-zref)*pars(1) + (m.z-zref)*(m.z-zref)*pars(2) ) ;
	
	success = pars(2) < 0 ;
	if( !success ) {
	  std::stringstream str ;
	  str << "Vector: " << halfdchi2dpar << std::endl
	      << "Matrix: " << halfd2chi2dpar2 ;
	  qDebug() << str.str().c_str() ;
	}
	if( success ) z0 = zref - 0.5*pars(1)/pars(2) ;
      }
      return success ;
    }
    
    std::pair<int,FocusMeasurement>
    analyseFocusSeries( const std::vector<FocusMeasurement>& measurements,
			double threshold = 20)
    {
      // assume that they are sorted?
      std::pair<int,FocusMeasurement> rc ;
      rc.first = FocusSeriesType::None ;
      if( measurements.empty() ) return rc ;
      rc.second = measurements.front() ;
      for(unsigned int i=1; i<measurements.size(); ++i) {
	if( measurements[i].I > measurements[i-1].I + threshold )
	  rc.first |= FocusSeriesType::Rising ;
	if( measurements[i].I < measurements[i-1].I - threshold )
	  rc.first |= FocusSeriesType::Falling ;
	if( measurements[i].I > rc.second.I )
	  rc.second = measurements[i] ;
      }
      return rc ;
    }

    std::vector<FocusMeasurement> selectnearfocusmeasurements( const std::vector<FocusMeasurement>& measurements,
							       unsigned char N,
							       size_t& imax)
    {
      if( measurements.size() < N ) return measurements ;
      
      // selects the measurements with the best focus and N
      // measurements left and right. assumes the measurements are
      // ordered.
      imax=0 ;
      for(size_t i = 0; i<measurements.size(); ++i)
	if( measurements[imax].I < measurements[i].I) imax=i ;
      auto i1 = std::max(int(0),int(imax)-int(N)) ;
      auto i2 = std::min(measurements.size(),imax+N+1) ;
      qDebug() << "selectnearfocusmeasurements: "
	       << "i1=" << i1 << " imax=" << imax << " i2=" << i2 ;

      return std::vector<FocusMeasurement>{measurements.begin() + i1,
	  measurements.begin() + i2} ;
    }
    
  }
  
  void AutoFocus::analyseFastFocus( FocusMeasurement result )
  {
    // this routine needs to determine if we are ready, and if we are,
    // stop the axis. let's walk a certain minimum distance past the
    // maximum (e.g. 50 micron)

    // somehow we need to characterise the series:
    // 1. only increasing
    // 2. only decreasing
    // 3. maximum found
    // 4. neither rising nor decreasing
    // In case 4 we don't know what to do. In case 1 or 2 we need to
    // search in the other direction. In case 3 we are done.
    m_fastfocusmeasurements.push_back( result ) ;
    auto analysis = analyseFocusSeries( m_fastfocusmeasurements, 5 ) ;
    m_bestfocus = analysis.second ;
    m_focusseriestype = FocusSeriesType(analysis.first) ;
    qDebug() << "analyseFastFocus: "
	     << m_focusseriestype
	     << m_bestfocus.z
	     << result.z ;
    if( m_focusseriestype & FocusSeriesType::Falling &&
	std::abs( m_bestfocus.z - result.z ) > 0.05 ) {
      m_zaxis->stop() ;
    }
  }

  void AutoFocus::analyseSlowFocus( FocusMeasurement result )
  {
    if( (m_fastfocusmeasurements.size()%10) == 0 )
      qDebug() << "AutoFocus::analyseSlowFocus: " << m_fastfocusmeasurements.size() << result.z << result.I;
    if( m_fastfocusmeasurements.empty() ||
	std::abs(result.z - m_fastfocusmeasurements.back().z) >0.0001 )
      m_fastfocusmeasurements.push_back( result ) ;
    //qDebug() << "End of AutoFocus::analyseSlowFocus: " ;
  }

  void AutoFocus::startNearFocusSequence()
  {
    qDebug() << "AutoFocus::startNearFocusSequence" << m_zaxis ;
    if( !isFocussing() ) {
      m_status = IsFocussing ;
      auto axisvelocity =  m_zaxis->parameter("Velocity") ;
      const auto originalspeed = axisvelocity->getValue().value() ;
      double zstart = m_zaxis->position().value() ;
      qDebug() << "zstart = " << zstart << originalspeed ;
      // move to 50 micron below it. perform a slow search
      {
	m_zaxis->moveTo(zstart-0.06) ;
	QCoreApplication::processEvents() ;
	QSignalSpy spy( m_zaxis,&MotionAxis::movementStopped) ;
	spy.wait( 10000 ) ;
	qDebug() << "Curent z: " <<  m_zaxis->position().value() ;
      }
      {
	m_zaxis->moveTo(zstart-0.05) ;
	QCoreApplication::processEvents() ;
	QSignalSpy spy( m_zaxis,&MotionAxis::movementStopped) ;
	spy.wait( 10000 ) ;
	qDebug() << "Curent z: " <<  m_zaxis->position().value() ;
      }
      // perform a slow search
      m_focusmeasurements->clear() ;
      m_fastfocusmeasurements.clear() ;
      qDebug() << "Connecting signal to collect measurements" ;
      auto connection = QObject::connect(this,&AutoFocus::focusMeasurement,this,&AutoFocus::analyseSlowFocus) ;
      qDebug() << "Setting velocity" ;
      axisvelocity->setValue() = m_settings->slowspeed.value() ;
      qDebug() << "Moving slowly to new z position: " ;
      {
	m_zaxis->moveTo(zstart+0.05) ;
	QSignalSpy spy( m_zaxis,&MotionAxis::movementStopped) ;
	spy.wait( 10000 ) ;
      }
      qDebug() << "Disconnecting signal to collect measurements" ;
      QObject::disconnect(connection) ;
      if( m_fastfocusmeasurements.empty() ) {
	qDebug() << "No measurements. Issue focusfailed signal" ;
	m_status = FocusFailed ;
	emit focusfailed() ;
	
      } else {
	// for now just print the series
	for_each( m_fastfocusmeasurements.begin(),
		  m_fastfocusmeasurements.end(),
		  []( const FocusMeasurement& m ) { qDebug() << m.z << " " << m.I ; } ) ;
	// why does every measurement appear two times?
	// perform a parabola fit two measurements close to the maximum?
	size_t imax{0} ;
	std::vector<FocusMeasurement> selection =
	  selectnearfocusmeasurements(m_fastfocusmeasurements,2,imax) ;
	double z0 = m_fastfocusmeasurements[imax].z;
	bool success = estimatez0(selection,z0) ;
	qDebug() << "near focus success/z0: " << success << " " << z0 ;
	//if(!success) z0 = m_fastfocusmeasurements[imax].z ;
	
	// now we need to arrive from below
	{
	  m_fastfocusmeasurements.clear() ;
	  auto connection = QObject::connect(this,&AutoFocus::focusMeasurement,this,&AutoFocus::analyseSlowFocus) ;
	  m_zaxis->moveTo(z0-0.05) ;
	  QSignalSpy spy( m_zaxis,&MotionAxis::movementStopped) ;
	  spy.wait( 10000 ) ;
	  std::vector<FocusMeasurement> selection =
	    selectnearfocusmeasurements(m_fastfocusmeasurements,2,imax) ;
	  double z0tmp = m_fastfocusmeasurements[imax].z;
	  /*bool success = */ estimatez0(selection,z0tmp) ;
	  qDebug() << "up/down measures of z0: "
		   << z0 << " " << z0tmp << " " << z0 - z0tmp ;
	  disconnect(connection) ;
	}
	axisvelocity->setValue() = originalspeed ;
	m_zaxis->moveTo(z0) ;
	m_status = IsFocussed ;
	emit focussed() ;
      }
    }
    qDebug() << "End of AutoFocus::startNearFocusSequence" ;
  }

  void AutoFocus::startFastFocusSequenceSimple()
  {
    startFocusSequence( m_settings->zmin, m_settings->zmax) ;
  }

  void AutoFocus::takeSeries( double zmin, double zmax, double velocity )
  {

    // First make sure that we are no longer moving
    if( m_zaxis->isMoving() ) {
      m_zaxis->stop() ;
      QSignalSpy spy( m_zaxis, &MotionAxis::movementStopped) ;
      spy.wait( 60000 ) ;
    }
    
    // Now move to zmin, if we are not yet there.
    auto axisvelocity =  m_zaxis->parameter("Velocity") ;
    axisvelocity->setValue() = m_settings->movespeed.value() ;
    const double positiontolerance = 0.001 ;
    if( std::abs(m_zaxis->position() - zmin) > positiontolerance ) {
      m_zaxis->moveTo(zmin) ;
      QSignalSpy spy( m_zaxis,&MotionAxis::movementStopped) ;
      spy.wait( 60000 ) ;
    }
    
    axisvelocity->setValue() = velocity ;
    
    // clear the focusmeasurements and start taking data.
    m_focusmeasurements->clear() ;
    m_fastfocusmeasurements.clear() ;
    connect(this,&AutoFocus::focusMeasurement,this,&AutoFocus::analyseSlowFocus) ;
    m_zaxis->moveTo(zmax) ;
    QSignalSpy spy( m_zaxis,&MotionAxis::movementStopped) ;
    // factor 5 safety margin?
    const double timeout = 5000*std::abs(zmax-zmin)/velocity ;
    qDebug() << "AutoFocus::takeSeries: timeout for  search: " << timeout  ;
    spy.wait( timeout ) ;
    // disconnect and reset velocity
    disconnect(this,&AutoFocus::focusMeasurement,this,&AutoFocus::analyseSlowFocus) ;
    axisvelocity->setValue() = m_settings->movespeed.value() ;
  }
  
  void AutoFocus::startFocusSequence(double zmin, double zmax)
  {
    // FIXME: we still need to return a failure in case we fail the number of tries
    const int maxtries = 3 ;
    if( !isFocussing() ) {
      bool success = false ;
      m_status = IsFocussing ;
      
      // cache the speed
      auto axisvelocity =  m_zaxis->parameter("Velocity") ;
      const auto originalspeed = axisvelocity->getValue().value() ;

      // Take the fast series, until it is right
      double z1 = std::min(zmin,zmax) ;
      double z2 = std::max(zmin,zmax) ;
      if( z2 - z1 > 0.11 ) {
	qDebug() << "Starting fast search loop" ;
	int ntries{0} ;
	do {
	  qDebug() << "Fast search iteration: " << ntries ;
	  takeSeries(z1,z2,m_settings->fastspeed.value()) ;
	} while(!testseries( m_fastfocusmeasurements, axisvelocity->getValue().value().toDouble() ) &&
		++ntries<=maxtries ) ;
	
	// find the maximum
	m_bestfocus = m_fastfocusmeasurements.front() ;
	for( const auto& m : m_fastfocusmeasurements )
	  if( m.I > m_bestfocus.I ) m_bestfocus = m;
	qDebug() << "Fast search done: " << m_bestfocus.z ;
	// move to 50 micron below it. why do we have an asymmetric
	// range. because we are so much biased?
	z1 = std::max(z1,m_bestfocus.z-0.05) ;
	z2 = std::min(m_bestfocus.z+0.10,z2) ;
	
      } else {
	qDebug() << "No need to do fast search as we are close enough" ;
      }
      
      // now perform a slow search
      int ntries = 0 ;
      do {
	qDebug() << "FastSlow search iteration: " << ntries ;
	takeSeries(z1,z2,m_settings->slowspeed.value()) ;
      } while(!testseries( m_fastfocusmeasurements, axisvelocity->getValue().value().toDouble() ) &&
	      ++ntries<=maxtries ) ;
      
      // for now just print the series
      // for_each( m_fastfocusmeasurements.begin(),
      // 		m_fastfocusmeasurements.end(),
      // 		[]( const FocusMeasurement& m ) { qDebug() << m.z << " " << m.I ; } ) ;
      // why does every measurement appear two times?
      // perform a parabola fit two measurements close to the maximum?
      if(!m_fastfocusmeasurements.empty()) {
	int imax=0 ;
	for(size_t i = 0; i<m_fastfocusmeasurements.size(); ++i)
	  if( m_fastfocusmeasurements[imax].I < m_fastfocusmeasurements[i].I) imax=int(i) ;
	auto i1 = std::max(0,imax-2) ;
	auto i2 = std::min(int(m_fastfocusmeasurements.size()),imax+3) ;
	qDebug() << "Focus selection: " << m_fastfocusmeasurements.size() << imax << i1 << i2 ;
	std::vector<FocusMeasurement> selection(m_fastfocusmeasurements.begin() + i1,
						m_fastfocusmeasurements.begin() + i2) ;
	double z0 = m_fastfocusmeasurements[imax].z ;
	success = estimatez0(selection,z0) ;
	qDebug() << "Three esimataes of z0: "
		 << m_bestfocus.z	 << " " << m_fastfocusmeasurements[imax].z
		 << z0 ;
	if(!success || z0<zmin || z0 > zmax)
	  z0 = m_fastfocusmeasurements[imax].z ;

	axisvelocity->setValue() = m_settings->movespeed.value() ;
	// now we need to arrive from below
	{
	  m_zaxis->moveTo(z0-0.05) ;
	  QSignalSpy spy( m_zaxis,&MotionAxis::movementStopped) ;
	  spy.wait( 10000 ) ;
	}
	m_zaxis->moveTo(z0) ;
	QSignalSpy spy( m_zaxis,&MotionAxis::movementStopped) ;
	spy.wait( 10000 ) ;
	axisvelocity->setValue() = originalspeed ;
      } else {
	success = false ;
      }

      m_status = success ? IsFocussed : FocusFailed ;
      emit success ? focussed() : focusfailed() ;
    }
  }
  
}
