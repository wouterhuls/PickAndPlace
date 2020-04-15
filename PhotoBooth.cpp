

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
#include "Eigen/Dense"
#include <opencv2/opencv.hpp>

namespace FocusStacking
{
  cv::Mat computeLaplacian(const cv::Mat& image)
  {
    // I don't quite understand what this does, but let's try it
    const int kernel_size = 31;
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

    int scale = 1;
    int delta = 0;
    int ddepth = CV_16S;
    cv::GaussianBlur( src, src, cv::Size(3,3), 0, 0, cv::BORDER_DEFAULT );
    
    /// Convert it to gray
    cv::Mat src_gray ;
    cv::cvtColor( src, src_gray, cv::COLOR_BGR2GRAY );
    
    /// Create window
    /// const char* window_name = "Sobel Demo - Simple Edge Detector";
    /// namedWindow( window_name, cv::WINDOW_AUTOSIZE );

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
		    [&](const cv::Mat& image) { return computeLaplacian(image) ; } ) ;
    
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
    cv::Size smoothsize{21,21} ;
    cv::GaussianBlur(indexmask, smoothedindexmask, smoothsize, 0);
    //std::cout << "in stack: after smoothing" << std::endl ;
    // finally, create the output by picking the right pixel from each
    // image. every pixel is there bytes of data, so it isn't even so
    // trivial to do this right.
    cv::Mat output( thesize, images.front().type() ) ;
    for( size_t pixel=0; pixel<Npixels; ++pixel)
      for(int ibyte=0; ibyte<3; ++ibyte)
	output.data[3*pixel+ibyte] = images[ smoothedindexmask.data[pixel] ].data[3*pixel+ibyte] ;

    cv::namedWindow( "indexmask", 2 );
    cv::imshow( "indexmask", indexmask);
    //std::cout << "in stack: end" << std::endl ;
    return output;
  }

  void ShowManyImages(std::string title, const std::vector<cv::Mat>& images)
  {
    if( images.empty() ) return ;

    // define the pads
    const size_t N = images.size() ;
    const size_t Nx = std::sqrt(N) ;
    size_t Ny = N/Nx  ;
    if(Ny*Nx<N) ++Ny ;
     
    // give all images the same size in x
    const auto& firstimage = images.front() ;
    const float scale = 200. / firstimage.cols ;
    const int xsize = scale * firstimage.cols ;
    const int ysize = scale * firstimage.rows ;
    std::cout << "x,ysize: "
	      << xsize << " " << ysize << std::endl ;
    
    cv::Mat DispImage = cv::Mat::zeros(cv::Size(Nx*xsize,Ny*ysize), images.front().type());
    
    size_t index{0} ;
    for( const auto& img : images ) {
      size_t row = index % Nx ;
      size_t col = index / Nx ;
      // Set the image ROI to display the current image
      // Resize the input image and copy the it to the Single Big Image
      cv::Rect ROI(row*xsize, col*ysize,xsize,ysize);
      cv::Mat temp;
      cv::resize(img,temp, cv::Size(ROI.width, ROI.height));
      temp.copyTo(DispImage(ROI));
      ++index ;
    }
    
    // Create a new window, and show the Single Big Image
    cv::namedWindow( title, 1 );
    cv::imshow( title, DispImage);
    //waitKey();
  }
}


namespace PAP
{

  class PhotoBooth : public MovementSeries
  {
    //Q_OBJECT
  public:
    PhotoBooth( PAP::CameraWindow& parent, ViewDirection viewdir) ;
    virtual void initialize() ;
    virtual void execute() ;
    virtual void finalize() ;
  public slots: 
    void addToStack( const QVideoFrame& frame ) ;
  private:
    std::vector<std::pair<Coordinate,cv::Mat> > m_images ;
    QCheckBox m_useFosusStacking{"Use focus stacking",this} ;
    float m_focusstackzmin ;
    float m_focusstackzmax ;
    const float m_focusstackdz = 0.025 ; // max 25 micron between images in the photostack
    std::vector<cv::Mat> m_stackimages ;
    //QTransform m_fromPixelToModule ;
    QTransform m_fromModuleToPixel ;
    cv::Mat m_combinedimage ;
    double m_scale{1} ; // scale factor when putting things in the combined image
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
    qDebug() << "PhotoBooth::initialize()" ;
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
            
      const auto& modulepoints = coordinates() ;
      if( !modulepoints.empty() ) {

	// this is quite tricky: we are going to generate a new set of
	// points from the given set of points. we want the movement
	// series to use the new set of points. Perhaps I need to
	// separate the two inside MovementSeries.

	// Because in the end we need pictures with the right
	// rotation, I'm going to do all of this in a 'pixel'
	// frame. We need to cache this frame, such that we can later
	// use is again (I think).
	m_fromModuleToPixel = cameraview()->fromModuleToPixel() ;
        auto m_fromPixelToModule = m_fromModuleToPixel.inverted() ;
	std::vector<QPointF> refpoints ;
	std::transform( modulepoints.begin(), modulepoints.end(),
			std::back_inserter(refpoints),
			[&]( const auto& p ) {
			  return m_fromModuleToPixel.map( QPointF{p.x(),p.y()} ) ;
			} ) ;
	
	// It would be nice to use a bit of C++ magic and std::minmax here.
	const auto& firstpoint = refpoints.front() ;
	double xmin{firstpoint.x()} ;
	double xmax{xmin} ;
	double ymin{firstpoint.y()} ;
	double ymax{ymin} ;
	for( const auto& p : refpoints ) {
	  if(      xmin > p.x() ) xmin = p.x() ;
	  else if( xmax < p.x() ) xmax = p.x() ;
	  if(      ymin > p.y() ) ymin = p.y() ;
	  else if( ymax < p.y() ) ymax = p.y() ;
	}

	
	double zmin{modulepoints.front().z()} ;
	double zmax{zmin} ;
	for( const auto& p : modulepoints ) {
	  if(      zmin > p.z() ) zmin = p.z() ;
	  else if (zmax < p.z() ) zmax = p.z() ;
	}
	m_focusstackzmin = zmin ;
	m_focusstackzmax = zmax ;

	// so, I figured that because a photo is larger in x
	// than in y, we can take fewer pictures if we bin them fixed
	// distance in y, and smooth in x. the
	// first thing we do is compute the angle: perform a straight
	// line fit of the points to x=f(y-y0),
	const double yref = firstpoint.y() ;
	Eigen::Vector2d halfdchi2dpar   = Eigen::Vector2d::Zero() ;
	Eigen::Matrix2d halfd2chi2dpar2 = Eigen::Matrix2d::Zero() ;
	for(  const auto& p : refpoints ) {
	  Eigen::Vector2d H ;
	  H(0) = 1 ;
	  H(1) = (p.y()-yref) ;
	  halfdchi2dpar += H*p.x() ;
	  halfd2chi2dpar2 += H * H.transpose();
	}
	Eigen::Vector2d pars = halfd2chi2dpar2.ldlt().solve(halfdchi2dpar) ;
	// now compute the minimum and maximum excursion from the line
	double dxmin{ firstpoint.x() - (pars(0) + (firstpoint.y()-yref)*pars(1)) } ;
	double dxmax{dxmin} ;
	for(  const auto& p : refpoints ) {
	  double dx = p.x() - (pars(0) + (p.y()-yref)*pars(1)) ;
	  if(dx < dxmin ) dxmin = dx ;
	  else if(dx>dxmax) dxmax = dx ;
	}
	
	// now compute the number of picture tiles in x and y
	auto cv = cameraview() ;
	const auto picturewidth  = cv->numPixelsX() /** cv->pixelSizeX()*/ ; 
	const auto pictureheight = cv->numPixelsY() /** cv->pixelSizeY()*/ ;
	qDebug() << "Picture size: " << picturewidth << pictureheight ;
	const size_t nY = (ymax-ymin) / pictureheight + 1 ;
	const size_t nX = (dxmax-dxmin) / picturewidth  + 1 ;
	qDebug() << "nX, nY: " << nX << nY ;
	// Now adjust the boundaries offsets in Y such that pictures do not overlap
	const double Y0  = ymin - 0.5*( (nY-1)*pictureheight - (ymax-ymin) ) ;
	const double dX0 = dxmin - 0.5*((nX-1)*picturewidth - (dxmax-dxmin) ) ;

	// now define all the new points
	
	std::vector<Coordinate> allpoints ;
	for( size_t iy = 0; iy<nY; ++iy ) {
	  const double y = Y0+iy*pictureheight ;
	  for( size_t ix = 0; ix<nX; ++ix ) {
	    const double x = pars(0) + (y-yref)*pars(1) + dX0 +ix*picturewidth ;
	    auto modulepoint = m_fromPixelToModule.map( QPointF{x,y} ) ;
	    qDebug() << "new point: " << x << y << modulepoint ;
	    // get a reasonable focus position. it may be better just
	    // to take the closest point, rather than an interpolation
	    double sumZ{0}  ;
	    double sumWeight{0} ;
	    for( const auto& p : modulepoints ) {
	      double dx = p.x() - modulepoint.x() ;
	      double dy = p.y() - modulepoint.y() ;
	      double dr2 = dx*dx+dy*dy ;
	      sumWeight += dr2 ;
	      sumZ += dr2 * p.z() ;
	    }
	    allpoints.emplace_back( "P" + QString::number(allpoints.size()), modulepoint.x(), modulepoint.y(), sumZ/sumWeight) ;
	  }
	}

	// now allocate the picture. we'll need to introduce a scale,
	// because otherwise it will never fit in memory
	const int totalysize = nY*pictureheight ;
	const int totalxsize = int(nX*picturewidth+totalysize*pars(1)) ;
	m_scale = 10000./totalxsize ;
	m_combinedimage = cv::Mat::zeros(cv::Size{int(totalxsize*m_scale),int(totalysize*m_scale)},CV_8UC4) ;

	qDebug() << "transform: " << m_fromModuleToPixel ;
	// Now need to do one more thing: Make sure that the first
	// picture appears at the correct place by modifying the
	// transform a bit. Of course it would be nicer if we could do
	// this when wefirst define the transform.
	auto origin = m_fromModuleToPixel.map( allpoints.front().position().toPointF()) ;
	qDebug() << "origin: " << origin ;
	QTransform fromPixelToMovedPixel ;
	fromPixelToMovedPixel.translate( -origin.x(), -origin.y()) ;
	m_fromModuleToPixel = m_fromModuleToPixel * fromPixelToMovedPixel ;
	
	qDebug() << "transform after: " << m_fromModuleToPixel ;
	
	qDebug() << "PhotoBooth::initialize: Number of points: " << refpoints.size() << allpoints.size() ;
	
	setCoordinates( allpoints ) ;
	
      }
    }
        
    // Here is my idea now for mode B

    // 1. define a coordinate system by the first two points (which
    // should probably be markers) 2. compute the smallest rectangle
    // that contains all points in this frame. 3. compute the
    // trajectories that you need to take to cover the entire surface.
    // 4. We may need to determine a number of extra focus points in
    // between.
    
     qDebug() << "end of PhotoBooth::initialize()" ;
  } ;

  // I need a routine to return a single sharp image from a stack of
  // images with different focus. This is called focus stacking.

  void PhotoBooth::addToStack( const QVideoFrame& frame )
  {
    const_cast<QVideoFrame&>(frame).map(QAbstractVideoBuffer::ReadOnly) ;
    if( frame.bits() && frame.pixelFormat() == QVideoFrame::Format_RGB32 ) {
      //QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
      //QImage img( frame.bits(),frame.width(),frame.height(),frame.bytesPerLine(),imageFormat);
      cv::Mat mat( frame.height(), frame.width(), CV_8UC4,
		   const_cast<uchar*>(frame.bits()), static_cast<size_t>(frame.bytesPerLine()) );
      // now clone it to take ownership of the data
      m_stackimages.emplace_back( mat.clone() ) ;
    }
    const_cast<QVideoFrame&>(frame).unmap() ;
  }
  
  void PhotoBooth::execute()
  {
    // let's first implement the method with stacking, because that is what I need right now.
    cv::Mat image ;
    m_stackimages.clear() ;
    if( m_useFosusStacking.isChecked() ) {
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
      qDebug() << "Computed slow speed: " << slowspeed ;
      
      // connect the videoframe to the stack
      auto videoconnection = 
	//connect(cameraview()->videoProbe(), SIGNAL(videoFrameProbed(QVideoFrame)),this,SLOT(addToStack(QVideoFrame)));
	connect( cameraview()->videoProbe(),
		 &QVideoProbe::videoFrameProbed,
		 this,&PhotoBooth::addToStack);
      
      // walk towards the other z value
      {
	QSignalSpy spy(&focusaxis,&MotionAxis::movementStopped ) ;
	autofocus->moveFocusToModuleZ( z2 ) ;
	spy.wait( 10000 ) ;
      }

      qDebug() << "Number of images in stack: "
	       << m_stackimages.size() ;
      
      // if we don't have at least one image, wait for an image
      if( m_stackimages.empty() ) {
	QSignalSpy spy( cameraview()->videoProbe(), SIGNAL(videoFrameProbed(QVideoFrame)) ) ;
	spy.wait( 10000 ) ;
      }
      
      // one arrived, disconnect the video frame, reset the velocity
      QObject::disconnect(videoconnection) ;
      axisvelocity->setValue() = originalspeed ;
      
      // create a single image from the stack
      FocusStacking::ShowManyImages("stack images", m_stackimages) ;
    
      image = FocusStacking::combine( m_stackimages ) ;
      //image = m_stackimages.front() ;
      
      cv::namedWindow( "stackedoutput", 3 );
      cv::imshow( "stackedoutput", image);
      m_stackimages.clear() ;
      
    } else {
      auto videoconnection = 
	connect( cameraview()->videoProbe(),&QVideoProbe::videoFrameProbed,
		 this,&PhotoBooth::addToStack);
      QSignalSpy spy( cameraview()->videoProbe(), SIGNAL(videoFrameProbed(QVideoFrame)) ) ;
      spy.wait( 10000 ) ;
      QObject::disconnect(videoconnection) ;
      image= m_stackimages.front() ;
    }
    
    recordCentre() ;
    m_images.push_back( std::make_pair( coordinates()[currentcoordinate()], image ) ) ;


    // let's immediately try to put this in the right place
    QPointF pixelcentre = m_fromModuleToPixel.map( coordinates()[currentcoordinate()].position().toPointF() ) ;
    const auto& w = image.cols ;
    const auto& h = image.rows ;
    cv::Rect ROI{int(m_scale*pixelcentre.x()),int(m_scale*pixelcentre.y()),int(m_scale*w),int(m_scale*h)} ;
    qDebug() << "pixelcentre: " << pixelcentre << ROI.x << ROI.y << m_combinedimage.cols << m_combinedimage.rows ;
    cv::Mat temp;
    cv::resize(image,temp, cv::Size(ROI.width, ROI.height));
    
    // cv::namedWindow( "original", 1 );
    // cv::imshow( "original", image);
    // cv::namedWindow( "scaled", 1 );
    // cv::imshow( "scaled", temp);
    
    temp.copyTo( m_combinedimage(ROI) ) ;

    
    // emit the signal that we are ready
    emit executeReady() ;
  }

  void PhotoBooth::finalize()
  {
    qDebug() << "Number of images to be stitched: " << m_images.size()  ;
    cv::imwrite("combinedimage.jpg",m_combinedimage) ;
    // size_t index{0} ;
    // for(const auto& im: m_images) {
    //   std::string filename = (QString{"image"} + QString::number(++index) + ".jpg").toStdString() ;
    //   cv::imwrite( filename,im.second) ;
    // }
    
    return MovementSeries::finalize() ;
  }
  
}
