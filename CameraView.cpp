
#include "CameraView.h"
#include "GeometrySvc.h"
#include "MotionSystemSvc.h"

#include <QCamera>
#include <QCameraInfo>
#include <QCameraViewfinder>
#include <QGridLayout>
#include <QCameraImageCapture>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsVideoItem>
#include <QWheelEvent>
#include <QTimeLine>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QMessageBox>
#include <QVideoProbe>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

#include <cmath>
#include <iostream>

namespace PAP
{

  CameraView::CameraView(QWidget *parent)
    : QGraphicsView(parent),
      m_camera(0),
      m_scene(0),
      m_viewfinder(0),
      m_videoProbe(0),
      m_frame(0),
      m_chipPixelSize(0.00345),
      m_magnification("Cam.Magnification",5.03),
      m_rotation("Cam.Rotation",+M_PI/2),
      m_numScheduledScalings(0)
  {
    resize(622, 512);
    //resize(2*622, 2*512) ;

    //auto layout = new QGridLayout(centralwidget) ;
    // if(false) {
    //   m_viewfinder = new QCameraViewfinder(this ) ;
    //   m_viewfinder->resize(622,512) ;
    // }
    //layout->addWidget( m_viewfinder,0,0,1,1 ) ;
    //centralwidget->setLayout( layout ) ;
    //if(true) {
    m_scene = new QGraphicsScene{} ;
    m_viewfinder = new QGraphicsVideoItem{} ;
    //m_viewfinder->setSize( QSize{622,512} ) ;
    m_numPixelsX = 2448 ;
    m_numPixelsY = 2048 ;
    m_viewfinder->setSize( QSize{m_numPixelsX,m_numPixelsY} ) ;
    double maxscale = size().width() / double(m_viewfinder->size().width()) ;
    scale(maxscale,maxscale) ;
	
    m_scene->addItem(m_viewfinder) ;
    this->setScene( m_scene ) ;

    // let's add a cross at (0,0) in the cameraview. here it would be
    // nice to have some size in 'absolute' coordinates. but for that
    // we need to know the pixel size, which I do not yet do.
    m_localOrigin = QPointF(0.5*m_numPixelsX,0.5*m_numPixelsY) ;
    qreal x0 = m_localOrigin.x() ;
    qreal y0 = m_localOrigin.y() ;
    qreal dx = 0.1/pixelSizeX() ; // make a line of 100 micron
    qreal dy = 0.1/pixelSizeY() ;
    auto yline = new QGraphicsLineItem(x0,y0-dy,x0,y0+dy) ;
    auto xline = new QGraphicsLineItem(x0-dx,y0,x0+dx,y0) ;
    m_scene->addItem( xline ) ;
    m_scene->addItem( yline ) ;
    
    //m_cursor = new QGraphicsTextItem("0, 0", 0, this); //Fixed at 0, 0

    //m_view = new QGraphicsView{m_scene,this} ;
    //m_view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    //}

    // I am not sure what this does
    //this->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    
    // list the cameras
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, cameras) {
        qDebug() << "camera description: "
                 << cameraInfo.description() ;
        qDebug() << "camera name: "
                 << cameraInfo.deviceName() ;
        //if (cameraInfo.deviceName() == "mycamera")
        //    camera = new QCamera(cameraInfo);
    }
    
    setCamera(QCameraInfo::defaultCamera());

    show() ;
  }
  
  
  CameraView::~CameraView()
  {
    delete m_camera ;
  }
  
  void CameraView::setCamera(const QCameraInfo &cameraInfo)
  {
    qInfo() << "CamereView::setCamera" ;
    bool isDFKcamera = cameraInfo.description().contains("DFK") ;
    
    delete m_camera;
    m_camera = new QCamera(cameraInfo);
    
    // let's do something special for our gstreamer camera:
    if( isDFKcamera ) {
      qDebug() << "Found a DFK camera" ;
      QCameraViewfinderSettings settings ;
      const int xsize = 2448 ;
      const int ysize = 2048 ;
      settings.setResolution(xsize,ysize) ;
      settings.setMaximumFrameRate(5) ;
      settings.setMinimumFrameRate(5) ;
      settings.setPixelFormat(QVideoFrame::Format_BGR32) ;
      m_camera->setViewfinderSettings( settings ) ;
      m_camera->setViewfinder(m_viewfinder);
    } else {
      QCameraViewfinderSettings settings ;
      settings.setResolution(1280,720) ;
      settings.setPixelFormat(QVideoFrame::Format_UYVY) ;
      m_camera->setViewfinderSettings( settings ) ;    
    }


    // m_camera->setViewfinder(m_viewfinder);

    // FIXME: check that this line does not break anything!
    m_viewfinder->setSize( m_camera->viewfinderSettings().resolution() ) ;
    qDebug() << "Resolution is now: "
	     << m_camera->viewfinderSettings().resolution()
	     << m_viewfinder->size() ;

    
    // also add a videoprobe such that we can in parallel analyze the
    // image, e.g. for focusssing.
    m_camera->setCaptureMode(QCamera::CaptureVideo);
    m_videoProbe = new QVideoProbe(this);
    m_videoProbe->setSource(m_camera) ;
    // to analyze images we connect a function to the videoprobe signal
    connect(m_videoProbe, SIGNAL(videoFrameProbed(QVideoFrame)),
	    this, SLOT(processFrame(QVideoFrame)));
    
    m_camera->start();
    qDebug() << "CameraView E" ;
    
  }
  
  void CameraView::wheelEvent ( QWheelEvent * event )
  {
    // copied from https://wiki.qt.io/Smooth_Zoom_In_QGraphicsView
      int numDegrees = event->delta() / 8;
      int numSteps = numDegrees / 15; // see QWheelEvent documentation
      m_numScheduledScalings += numSteps;
      if (m_numScheduledScalings * numSteps < 0) // if user moved the wheel in another direction, we reset previously scheduled scalings
	m_numScheduledScalings = numSteps;
    
      QTimeLine *anim = new QTimeLine(350, this);
      anim->setUpdateInterval(20);
      
      connect(anim, SIGNAL (valueChanged(qreal)), SLOT (scalingTime(qreal)));
      connect(anim, SIGNAL (finished()), SLOT (animFinished()));
      anim->start();
  }
  
  void CameraView::scalingTime(qreal x)
  {
    qreal factor = 1.0 + qreal(m_numScheduledScalings) / 300.0;
    scale(factor, factor);
  }

  void CameraView::animFinished()
  {
    if (m_numScheduledScalings > 0)
      --m_numScheduledScalings;
    else
      ++m_numScheduledScalings;
    sender()->~QObject();
  }

  void CameraView::mousePressEvent( QMouseEvent* event)
  {
    // may need to cast to QGraphicsSceneMouseEvent

    // I want the position inside the scene, not inside this
    // graphicsview. how do we do that?
    
    
    // let's pop up a dialog with the cursos position
    if(event->button() == Qt::RightButton)
      {
	char message[256] ;
	int x = event->pos().x() ;
	int y = event->pos().y() ;
	QPointF local = mapToScene( x,y ) ;
	sprintf(message,"pos=(%d,%d)-->(%f,%f)",x,y,local.x()-m_localOrigin.x(),local.y()-m_localOrigin.y()) ;
	
	//QMessageBox dialog(this) ;
	//dialog.setText(message) ;

	QDialog dialog(this) ;
	QVBoxLayout layout ;
	QLabel label(&dialog) ;
	label.setText(message) ;
	layout.addWidget(&label) ;

	QPushButton movebutton("Center",&dialog) ;
	layout.addWidget(&movebutton) ;
	QPushButton closebutton("Close",&dialog) ;
	layout.addWidget(&closebutton) ;
	connect(&movebutton, &QPushButton::clicked, &dialog, &QDialog::accept);
	connect(&closebutton, &QPushButton::clicked, &dialog, &QDialog::reject);
	dialog.setLayout( &layout ) ;
	dialog.adjustSize() ;

	// I don't understand how to do this properly if I have more types of actions
	//QDialogButtonBox buttonBox(Qt::Horizontal);
	//buttonBox.addButton("Center",QDialogButtonBox::ActionRole) ;
	//buttonBox.addButton(QDialogButtonBox::Cancel) ;
	//connect(&buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	//connect(&buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	//layout.addWidget(&buttonBox) ;

	int ok = dialog.exec() ;
	if( ok ) {
	  moveCameraTo( local ) ;
	}
	//qInfo() << "CameraView: result=" << result << " " << QDialogButtonBox::ActionRole ;
      }
  }

  void CameraView::processFrame( const QVideoFrame& frame )
  {
    // compute image contrast ? maybe not on every call!
    qDebug() << "Frame size: " << frame.size() ;
    m_frame = const_cast< QVideoFrame* >(&frame) ;
    static int numtries=10 ;
    if( --numtries>0 ) {
      computeContrast(frame) ;
      
    }
  }

  double CameraView::computeContrast( const QVideoFrame& frame ) const
  {
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
    const_cast<QVideoFrame&>(frame).map(QAbstractVideoBuffer::ReadOnly) ;

    // My laptop camera uses "UYVY", which means that UVY for two
    // adjacent pixels is stored with common U and V values. The Y
    // value is for brightness, which is all we need here. Of course,
    // as long as the camera anyway doesn't work, it makes little
    // sense to adapt the code for this.
    qDebug() << "VideoFrame: "
	     << frame.size() << " "
	     << frame.width() << " "
	     << frame.height() << " "
	     << frame.bytesPerLine() << " "
	     << frame.mappedBytes() << " "
	     << frame.pixelFormat() ;
    double rc = 0 ;
    if( frame.bits() && frame.pixelFormat() ==QVideoFrame::Format_BGR32) {
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
      
      const int centralPixelX = m_numPixelsX/2 ;
      const int centralPixelY = m_numPixelsY/2 ;
      const int numPixelX = 100 ; // 30x30 microns.
      const int numPixelY = 100 ;
      const int firstPixelX = centralPixelX - numPixelX/2 ;
      const int lastPixelX  = centralPixelX + numPixelX/2 ;
      const int firstPixelY = centralPixelY - numPixelY/2 ;
      const int lastPixelY  = centralPixelY + numPixelY/2 ;
      
      // fill histogram with intensity values
      double histogram[256] ;
      for(int i=0; i<256; ++i) histogram[i]=0 ;
      const unsigned int* bgr32 = reinterpret_cast<const unsigned int*>(frame.bits()) ;
      // for now we'll just use entropy. try fancier things later.
      for(int ybin = firstPixelY ; ybin<lastPixelY ; ++ybin) {
	int xbin0 = ybin*frame.height() ;
	for(int xbin = firstPixelX ; xbin<lastPixelX ; ++xbin) {
	  unsigned int bbggrrff = bgr32[ xbin0+xbin ] ;
	  unsigned char B  = (bbggrrff >> 24 ) & 0xff ;
	  unsigned char R  = (bbggrrff >> 16 ) & 0xff ;
	  unsigned char G  = (bbggrrff >> 8 )  & 0xff ;
	  unsigned char F  = bbggrrff & 0xff ;
	  if( ybin==firstPixelY && xbin==firstPixelX)
	    std::cout << "B,G,R: " << int(B) << " " << int(R) << " " << int(G) << " " << int(F) << std::endl ;
	  // many ways to turn this into an intensity. 
	  //https://stackoverflow.com/questions/687261/converting-rgb-to-grayscale-intensity
	  // https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color
	  // onw that runs very quickly is:
	  unsigned char Y = (R+R+R+B+G+G+G+G)>>3 ;
	  histogram[Y] += 1 ;
	}
      }
      // compute entropy
      double norm = numPixelX*numPixelY ;
      double entropy = 0 ;
      for(int i=0; i<256; ++i) entropy -= histogram[i] * std::log2(histogram[i]) ;
      //qDebug() << "Entropy = "
      //<< entropy ;
      // unmap in order to free the memory
      const_cast<QVideoFrame&>(frame).unmap() ;
      rc = entropy ;
    }
    return rc ;
  }
  
  void CameraView::moveCameraTo( QPointF localpoint ) const
  {
    // first compute the local change in microns
    double localdx = ( localpoint.x() - m_localOrigin.x() ) * pixelSizeX() ;
    double localdy = ( localpoint.y() - m_localOrigin.y() ) * pixelSizeY() ;
    // now translate that into a change in global coordinates, taking
    // into account that the camera view may be rotated
    const double cosphi = std::cos( m_rotation )  ;
    const double sinphi = std::sin( m_rotation ) ;
    PAP::Coordinates2D globaldx{
      localdx*cosphi + localdy*sinphi, -localdx*sinphi + localdy*cosphi} ;
    // now translate that into a change in motor positions using the geosvc
    auto mainstagedx = GeometrySvc::instance()->toMSMainDelta( globaldx ) ;
    qInfo() << "Moving camera: "
	    << "(" << localdx << "," << localdy << ") --> ("
	    << mainstagedx.x << "," << mainstagedx.y << ")" ;
    // finally, move the motors!
    MotionSystemSvc::instance()->axis("MainX")->move(mainstagedx.x) ;
    MotionSystemSvc::instance()->axis("MainY")->move(mainstagedx.y) ;
  }
}
