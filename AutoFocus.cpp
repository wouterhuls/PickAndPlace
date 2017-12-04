#include "AutoFocus.h"
#include "CameraView.h"
#include "Eigen/Dense"
#include <QSignalSpy>
#include <QVideoProbe>
#include <QVideoFrame>
#include <QPixmap>
#include <QLabel>
#include <QHBoxLayout>
#include <cmath>
#include "MotionSystemSvc.h"

namespace PAP
{
  AutoFocus::AutoFocus(CameraView* camview, QWidget* parent)
    : QDialog(parent)
  {
    QObject::connect(camview->videoProbe(), SIGNAL(videoFrameProbed(QVideoFrame)),
		     this, SLOT(processFrame(QVideoFrame)));

    // QObject::connect(camview->videoProbe(), &QVideoFrame::videoFrameProbed,
    // 		     this, 

    m_focusImage = new QImage{200,200,QImage::Format_Grayscale8} ;
    //m_focusImage = new QImage{2448,2048,QImage::Format_Grayscale8} ;

    m_focusView = new QLabel{this} ;
    m_focusView->setPixmap(QPixmap::fromImage(*m_focusImage));

    m_isFocussing = false ;

    resize(400,200) ;
    auto hlayout = new QHBoxLayout{} ;
    this->setLayout( hlayout) ;
    hlayout->addWidget( m_focusView ) ;
    auto label = new QLabel{"Here we should add a graph", this} ;
    hlayout->addWidget( label ) ;
  }

  AutoFocus::~AutoFocus() {}
  
 void AutoFocus::processFrame( const QVideoFrame& frame )
  {
    // compute image contrast ? maybe not on every call!
    //qDebug() << "Frame size: " << frame.size() ;
    //qDebug() << "Pointer to frame A: " << m_frame ;
    static int numtries=0 ;
    if( ++numtries==10 ) {
      computeContrast(frame) ;
      numtries=0;
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
      rc = WHsum ;

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
    }
    m_focusMeasure = rc ;
    emit focusMeasureUpdated() ;
    return rc ;
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
      Eigen::Vector3d resid = halfdchi2dpar - halfd2chi2dpar2 * pars ;
      qDebug() << "Resid: " << resid(0) << resid(1) << resid(2) ;     
      qDebug() << "Solution: " << pars(0) << " " << pars(1) << " " << pars(2)
	       << "---> " << zref - 0.5*pars(1)/pars(2) ;
      qDebug() << "Fitting to measurements: " ;
      for( const auto& m : mvec )
	qDebug() << m.z << " " << m.I
		 << ( pars(0) + (m.z-zref)*pars(1) + (m.z-zref)*(m.z-zref)*pars(2) ) ;
		    
      bool success = pars(2) < 0 ;
      if( !success ) {
	std::stringstream str ;
	str << "Vector: " << halfdchi2dpar << std::endl
	    << "Matrix: " << halfd2chi2dpar2 ;
	qDebug() << str.str().c_str() ;
	
      }
      if( success ) z0 = zref - 0.5*pars(1)/pars(2) ;
      return success ;
    }

    FocusMeasurement takeMeasurement(AutoFocus& autofocus,
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
      
      QSignalSpy spy(&autofocus, &AutoFocus::focusMeasureUpdated) ;
      spy.wait(10000) ;
      //FIXME: the position is not updated, unfortunately. let's take the setposition.
      qDebug() << "Creating focus measurement: "
	       << zpos << " "
	       << axis.position() << " "
	       << autofocus.focusMeasure() ;
      return FocusMeasurement( zpos, autofocus.focusMeasure() ) ;
    }
  }


  void AutoFocus::startFocusSequence()
  {
    if( !m_isFocussing ) {
      m_isFocussing = true ;
      qDebug() << "In focussing routine" ;
      // perhaps we should use 'minuit' for this. we don't actually know
      // the shape of the function very well.
      const float maxstepsize = 0.025 ; // 30 micron?
      const float minstepsize = 0.001 ;
      MotionAxis& axis = MotionSystemSvc::instance()->focusAxis() ;
      // tricky: need to make sure this is up-to-date. that will turn
      // out the problem all over this routine: how do we make sure that
      // we have an up-to-date position measurement?
      //if(! axis ) { qDebug() << "Cannot find axis!" ; return ; }
      
      const double zstart = axis.position() ;
      std::vector< FocusMeasurement > measurements ;
      measurements.reserve(64) ;
      qDebug() << "Ready to take focus measurements" ;
      
      // collect sufficient measurements that we are sure that we have
      // on both sides of the maximum. first do positive, then
      // negative direction.
      size_t maxnumsteps=10 ;
      double maximum(0) ;
      bool readypos=false ;
      bool failure = false ;
      double z0 = zstart ;
      double zpos = zstart ;
      while( !readypos && !failure ) {
	auto meas = takeMeasurement(*this, axis, zpos ) ;
	if( meas.I > maximum ) {
	  maximum = meas.I ;
	  z0 = zpos ;
	} else readypos = true ;
	measurements.push_back( meas ) ;
	zpos += maxstepsize ;
	failure = measurements.size()>maxnumsteps ;
      }
      bool readyneg=false ;
      zpos = zstart - maxstepsize ;
      while( !readyneg && !failure ) {
	auto meas = takeMeasurement(*this, axis, zpos ) ;
	if( meas.I > maximum ) {
	  maximum = meas.I ;
	  z0 = zpos ;
	} else readyneg = true ;
	measurements.insert(measurements.begin(),meas) ;
	zpos -= maxstepsize ;
	failure = measurements.size()>maxnumsteps ;
      }

      //
      qDebug() << "Number of measurements after step 1: "
	       << measurements.size() ;
      qDebug() << "Best z position after firs step: "
	       << z0 ;
      	
      // 1. compute three points, around the current z-position
      //{
      //double zpositions[] = { zstart - maxstepsize, zstart, zstart + maxstepsize } ;
      //	for( auto zpos : zpositions )
      //  measurements.push_back( takeMeasurement(*m_autofocus, axis, zpos ) ) ;
      //}
      // 2. collect more measurements 

      // 2. extrapolate to the estimated minimum.
      //double z0 = zstart ;
      double deltaz0 = maxstepsize ;
      bool success = true ;
      do {
	// pruning: if we have more than 4 measurements, remove all of those
	// that are more than maxstepsize away from the current best
	// estimate.
	bool readypruning = false ;
	while( measurements.size()>4 && !readypruning ) {
	  // find the worst measurement
	  size_t iworst=0 ;
	  for(size_t i=1; i<measurements.size(); ++i)
	    if( measurements[i].I < measurements[iworst].I ) iworst=i ;
	  if( std::abs(measurements[iworst].z - z0 ) > 2*maxstepsize ) 
	    measurements.erase( measurements.begin() + iworst ) ;
	  else
	    readypruning = true ;
	}
	// now estimate the new z0 from the remaining measurements
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
	  measurements.push_back( takeMeasurement(*this, axis, z0 ) ) ;
	  std::sort( measurements.begin(), measurements.end() ) ;
	  qDebug() << "Number of focussing measurements: " << measurements.size() ;
	  qDebug() << "Delta-z: " << deltaz0 ;
	  
	  // if we have more than 4 measurements, remove all of those
	  // that are more than maxstepsize away from the current best
	  // estimate.
	  
	}
      } while( success && std::abs(deltaz0) > minstepsize ) ;
      
      // if not successful, move back to where we started
      if( !success ) {
	qWarning() << "Focussing failed!" ;
	qDebug() << "Measurements: " << measurements.size() ;
	for( const auto& m : measurements )
	  qDebug() << m.z << " " << m.I ;
	axis.moveTo( zstart ) ;
      }
      m_isFocussing = false ;
      hide() ;
    }
  }

  
  
}
