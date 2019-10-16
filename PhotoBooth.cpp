

// let's write some code to take a series of images with a location

#include "MotionSystemSvc.h"
#include "GeometrySvc.h"
#include "CoordinateMeasurement.h"
#include "AutoFocus.h"
#include "CameraView.h"
#include "CameraWindow.h"
#include "AutoFocus.h"
#include "TextEditStream.h"
#include "NominalMarkers.h"
#include "MovementSeries.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QVideoFrame>
#include <QVideoProbe>
#include <QSignalSpy>

#include <opencv2/opencv.hpp>

namespace FocusStacking
{
  cv::Mat computeLaplacian(const cv::Mat& image)
  {
    // I don't quite understand what this does, but let's try it
    const int kernel_size = 5;
    const int blur_size = 5;
    
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY) ;
    
    cv::Mat blurred ;
    cv::Size size(blur_size, blur_size);
    cv::GaussianBlur(gray, blurred, size, 0);
    
    cv::Mat laplace;
    cv::Laplacian(blurred, laplace, CV_64F, kernel_size, 1, 0);
    
    cv::Mat absolute;
    cv::convertScaleAbs(laplace, absolute);
    
    return absolute;
  }
  
  cv::Mat computeAbsGradient( const cv::Mat& src )
  {
    // see https://docs.opencv.org/2.4/doc/tutorials/imgproc/imgtrans/sobel_derivatives/sobel_derivatives.html

    const char* window_name = "Sobel Demo - Simple Edge Detector";
    int scale = 1;
    int delta = 0;
    int ddepth = CV_16S;
    cv::GaussianBlur( src, src, cv::Size(3,3), 0, 0, cv::BORDER_DEFAULT );
    
    /// Convert it to gray
    cv::Mat src_gray ;
    cv::cvtColor( src, src_gray, cv::COLOR_BGR2GRAY );
    
    /// Create window
    namedWindow( window_name, cv::WINDOW_AUTOSIZE );

    /// Generate grad_x and grad_y
    cv::Mat grad_x, grad_y;
    cv::Mat abs_grad_x, abs_grad_y;
    
    /// Gradient X
    Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, cv::BORDER_DEFAULT );
    //Sobel( src_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
    convertScaleAbs( grad_x, abs_grad_x );
    
    /// Gradient Y
    Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, cv::BORDER_DEFAULT );
    //Sobel( src_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
    convertScaleAbs( grad_y, abs_grad_y );

    /// Total Gradient (approximate)
    cv::Mat grad ;
    addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad );

    //imshow( window_name, grad );

    return grad ;
  }
  
  cv::Mat combine( const std::vector<cv::Mat>& images )
  {
    //std::cout << "in stack: " << images.size() << std::endl ;
    const int Nrows = images.front().rows ;
    const int Ncols = images.front().cols ;
    const cv::Size thesize{Ncols,Nrows} ;
    
    // there must be a more efficient way to do this
    std::vector<cv::Mat> laplacians ;
    //std::cout << "in stack: computing laplacians" << std::endl ;
    std::transform( images.begin(), images.end(), std::back_inserter( laplacians ),
		    [&](const cv::Mat& image) { return computeAbsGradient(image) ; } ) ;
    
    //std::cout << "in stack: after computing laplacians" << std::endl ;

    // Some people use another sharpness measure that laplace (I
    // think just the same as I use for focussing). I think that
    // laplace is closer to zero when there is an edge. So, you'd
    // need the smallest laplace at each point.
    
    // From the laplacians find at each point the sharpest
    // image. This will all become really slow because the images
    // are so big: It may be better at some point to organize the
    // data differently such that it is better in the memory
    // cache. It will anyway be shit.
    const size_t Npixels = Nrows*Ncols ;
    std::vector< std::pair<unsigned char, unsigned char> > bestindex( Npixels, std::make_pair(0,0) ) ;
    // std::cout << "Data: " << images.front().step[0] << " "
    // 	      << images.front().step[1] << " " << Nrows << " " << Ncols << std::endl ;
    // std::cout << "Data: " << laplacians.front().step[0] << " "
    // 	      << laplacians.front().step[1] << " " << Nrows << " " << Ncols << std::endl ;
    for(size_t index = 0; index<images.size(); ++index ) {
      const auto& imgdata = laplacians[index].data ; // all okay, since these are actually unsigned chars
      // now I need a zip construction, but I'll be lazy at first
      for(size_t pixel=0; pixel<Npixels; ++pixel) {
	if( imgdata[pixel] > bestindex[pixel].second ) {
	  bestindex[pixel].second = imgdata[pixel] ;
	  bestindex[pixel].first  = index ;
	}
      }
    }

    //    std::cout << "in stack: after making mask" << std::endl ;
    // I do need the result in an image again, such that we can
    // smooth with a gaussian kernel.
    
    cv::Mat indexmask( thesize, CV_8U ) ;
    // std::cout << "After constructor!" << std::endl ;
    // std::cout << "Indexmask: " << indexmask.step[0] << " "
    // 	      << indexmask.step[1] << " " << indexmask.rows << " " << indexmask.cols << std::endl ;
    for(size_t pixel=0; pixel<Npixels; ++pixel) indexmask.data[pixel] = bestindex[pixel].first ;
    // std::cout << "distance: " << std::distance( indexmask.begin<unsigned char>(), indexmask.end<unsigned char>())
    //   << " : " << Npixels
    // 	      << " ;   " << indexmask.data << std::endl ;
    //std::cout << "in stack: after copying mask" << std::endl ;
    // next step is a gaussian blur over the indices to get rid of any strange peaks
    cv::Mat smoothedindexmask ;//{thesize, CV_8U } ;
    cv::Size smoothsize{5,5} ;
    cv::GaussianBlur(indexmask, smoothedindexmask, smoothsize, 0);
    std::cout << "in stack: after smoothing" << std::endl ;
    // finally, create the output by picking the right pixel from each
    // image. every pixel is there bytes of data, so it isn't even so
    // trivial to do this right.
    cv::Mat output( thesize, images.front().type() ) ;
    for( size_t pixel=0; pixel<Npixels; ++pixel)
      for(int ibyte=0; ibyte<3; ++ibyte)
	output.data[3*pixel+ibyte] = images[ smoothedindexmask.data[pixel] ].data[3*pixel+ibyte] ;
    //std::cout << "in stack: end" << std::endl ;
    return output;
  }
}


namespace PAP
{

  class PhotoBooth : public MovementSeries
  {
  public:
    PhotoBooth( PAP::CameraWindow& parent, ViewDirection viewdir) ;
    virtual void initialize() ;
    virtual void execute() ;
    virtual void finalize() ;
  public slots: 
    void addToStack( QVideoFrame& frame ) ;
  private:
    std::vector<std::pair<Coordinate,cv::Mat> > m_images ;
    QCheckBox m_useFosusStacking{"Use focus stacking",this} ;
    float m_focusstackzmin ;
    float m_focusstackzmax ;
    const float m_focusstackdz = 0.025 ; // max 25 micron between images in the photostack
    std::vector<cv::Mat> m_stackimages ;
  } ;

  
  QWidget* createPhotoBoothPage(CameraWindow& camerasvc, ViewDirection viewdir)
  {
    return new PhotoBooth( camerasvc, viewdir ) ;
  }
  
  // helper class for page for alignment of the main jig
  PhotoBooth::PhotoBooth(PAP::CameraWindow& parent, ViewDirection viewdir)
    : MovementSeries{parent,viewdir}
  {
    buttonlayout()->addWidget( &m_useFosusStacking ) ;
  }
  
  void PhotoBooth::initialize()
  {
    // for now, just take images at the given points
    
    // the next difficult thing will be to compute the minimal number
    // of images that we need to make a nice picture of the result.

    // We would actually like to have two modes:

    // - mode A: just create the smallest rectangle to contain all points in the camera frame. That's the easiest to stich pictures

    // - mode B: smallest rotated rectangle to contain all points:
    // that creates the smallest image which is good if you want to
    // photograph only the bond pads

    // Let's start with mode A, because it is most simple. We'll use
    // global coordinates for now.
    {
      const auto& refpoints = coordinates() ;
      if( !refpoints.empty() ) {
	// this is quite tricky: we are going to generate a new set of
	// points from the given set of points. we want the movement
	// series to use the new set of points. Perhaps I need to
	// separate the two inside MovementSeries.

	// It would be nice to use a bit of C++ magic and std::minmax here.
	const auto& firstpoint = refpoints.front() ;
	double xmin{firstpoint.x()} ;
	double xmax{xmin} ;
	double ymin{firstpoint.y()} ;
	double ymax{xmin} ;
	double zmin{firstpoint.z()} ;
	double zmax{zmin} ;
	for( const auto& p : refpoints ) {
	  if(      xmin > p.x() ) xmin = p.x() ;
	  else if( xmax < p.x() ) xmax = p.x() ;
	  if(      ymin > p.y() ) xmin = p.y() ;
	  else if( ymax < p.y() ) xmax = p.y() ;
	  if(      zmin > p.z() ) xmin = p.z() ;
	  else if (zmax < p.z() ) xmax = p.z() ;
	}
	m_focusstackzmin = zmin ;
	m_focusstackzmax = zmax ;
	
	auto cv = cameraview() ;
	const double picturewidth  = cv->numPixelsX() * cv->pixelSizeX() ; 
	const double pictureheight = cv->numPixelsY() * cv->pixelSizeY() ;
	const size_t nX = (xmax-xmin) / picturewidth  + 1 ;
	const size_t nY = (ymax-ymin) / pictureheight + 1 ;
	// Now adjust the boundaries such that pictures do not overlap
	const auto LX = nX*picturewidth ;
	const auto LY = nY*pictureheight ;
	const auto X0 = xmin - 0.5 * (LX - (xmax-xmin) ) ;
	const auto Y0 = ymin - 0.5 * (LY - (ymax-ymin) ) ;

	// now define all the new points
	std::vector<Coordinate> allpoints ;
	for( size_t ix = 0; ix<nX; ++ix )
	  for( size_t iy = 0; iy<nY; ++iy ) {
	    QPointF point = {X0 + ix*LX/nX,Y0 + iy*LY/nY} ;
	    double sumZ{0}  ;
	    double sumWeight{0} ;
	    for( const auto& p : refpoints ) {
	      double dx = p.x() - point.x() ;
	      double dy = p.y() - point.y() ;
	      double dr2 = dx*dx+dy*dy ;
	      sumWeight += dr2 ;
	      sumZ += dr2 * p.z() ;
	    }
	    allpoints.emplace_back( "P" + QString::number(allpoints.size()), point.x(), point.y(), sumZ/sumWeight) ;
	  }
		
	// now make the pictures ....
	setCoordinates( allpoints ) ;

	// finally, stich them.
	
      }
    }
        
    // Here is my idea now for mode B

    // 1. define a coordinate system by the first two points (which
    // should probably be markers) 2. compute the smallest rectangle
    // that contains all points in this frame. 3. compute the
    // trajectories that you need to take to cover the entire surface.
    // 4. We may need to determine a number of extra focus points in
    // between.
    
    
  } ;

  // I need a routine to return a single sharp image from a stack of
  // images with different focus. This is called focus stacking.

  void PhotoBooth::addToStack( QVideoFrame& frame )
  {
    frame.map(QAbstractVideoBuffer::ReadOnly) ;
    if( frame.bits() && frame.pixelFormat() == QVideoFrame::Format_RGB32 ) {
      //QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
      //QImage img( frame.bits(),frame.width(),frame.height(),frame.bytesPerLine(),imageFormat);
      cv::Mat mat( frame.height(), frame.width(), CV_8UC4,
		   const_cast<uchar*>(frame.bits()), static_cast<size_t>(frame.bytesPerLine()) );
      // now clone it to take ownership of the data
      m_stackimages.emplace_back( mat.clone() ) ;
    }
    frame.unmap() ;
  }
  
  void PhotoBooth::execute()
  {
    // let's first implement the method with stacking, because that is what I need right now.

    // we should properly implement this with signals and slots, but
    // for now use signalspy because I'm lazy.
    
    // clear the stack
    m_stackimages.clear() ;
    // move from zmin to zmax, or whatever is more efficient
    auto z1 = m_focusstackzmin ;
    auto z2 = m_focusstackzmax ;

    auto autofocus = camerasvc()->autofocus() ;
    auto currentz = autofocus->currentModuleZ() ;
    if( std::abs( currentz - z2 ) < std::abs( currentz - z1 ) ) std::swap(z1,z2) ;
    
    // move first to z1. use signal spy to contine when we have arrived
    auto& focusaxis = MotionSystemSvc::instance()->focusAxis() ;
    {
      QSignalSpy spy( &focusaxis, &MotionAxis::movementStopped ) ;
      autofocus->moveFocusToModuleZ( z1 ) ;
      spy.wait( 10000 ) ;
    }
    
    // now adjust the speed such that we make one frame every 25 micron
    // we assume (?) that we have 5 frames per second: cameraview can tell us.
    const int numframespersecond = 5 ;
    const double slowspeed = m_focusstackdz * numframespersecond ;
    MotionAxisParameter* axisvelocity =  focusaxis.parameter("Velocity") ;
    const auto originalspeed = axisvelocity->getValue().value() ;
    axisvelocity->setValue() = slowspeed ;

    // connect the videoframe to the stack
    auto videoconnection = 
      connect(cameraview()->videoProbe(), SIGNAL(videoFrameProbed(QVideoFrame)),this,SLOT(addToStack(QVideoFrame)));
    
    // walk towards the other z value
    {
      QSignalSpy spy(&focusaxis,&MotionAxis::movementStopped ) ;
      autofocus->moveFocusToModuleZ( z2 ) ;
      spy.wait( 10000 ) ;
    }
    
    // if we don't have at least one image, wait for an image
    if( m_stackimages.empty() ) {
      QSignalSpy spy( cameraview()->videoProbe(), SIGNAL(videoFrameProbed(QVideoFrame)) ) ;
      spy.wait( 10000 ) ;
    }
    
    // one arrived, disconnect the video frame, reset the velocity
    QObject::disconnect(videoconnection) ;
    axisvelocity->setValue() = originalspeed ;
    
    // create a single image from the stack
    auto image = FocusStacking::combine( m_stackimages ) ;
    recordCentre() ;
    m_images.push_back( std::make_pair( coordinates()[currentcoordinate()], image ) ) ;
    m_stackimages.clear() ;
    
    // emit the signal that we are ready
    emit executeReady() ;
  }

  void PhotoBooth::finalize()
  {
    qDebug() << "Number of images to be stitched: " << m_images.size()  ;
    size_t index{0} ;
    for(const auto& im: m_images) {
      std::string filename = (QString{"image"} + QString::number(++index) + ".jpg").toStdString() ;
      cv::imwrite( filename,im.second) ;
    }
    
    return MovementSeries::finalize() ;
  }
  
}