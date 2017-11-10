#include "AutoFocus.h"
#include "CameraView.h"
#include <QVideoProbe>
#include <QVideoFrame>
#include <QPixmap>
#include <QLabel>
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
	  unsigned char F  = (bbggrrff >> 24 ) & 0xff ;
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

      qDebug() << "Pos, mean, variance, entropy: "
	       << MotionSystemSvc::instance()->focusAxis().position() 
	       << mu << " " << var << " " << std::sqrt(var) << " "
	       << var/mu << " " << entropy  << " "
	       << BODsum << " "
	       << normBODsum << " "
	       << WHsum ;

      // what works is: sqrt(var), entropy and WHsum!
      
      
    m_focusView->setPixmap(QPixmap::fromImage(*m_focusImage));
    //m_focusView->setPixmap(QPixmap::fromImage(img));
    }
    m_focusMeasure = rc ;
    emit focusMeasureUpdated() ;
    return rc ;
  }
}
