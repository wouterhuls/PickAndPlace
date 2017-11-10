
#include "CameraView.h"
#include "GeometrySvc.h"
#include "MotionSystemSvc.h"
#include "GraphicsItems.h"

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
#include <QGraphicsItemGroup>

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
      m_chipPixelSize(0.00345),
      //m_magnification("Cam.Magnification",5.03),
      m_magnification("Cam.Magnification",4.9),
      m_rotation("Cam.Rotation",+M_PI/2),
      m_numScheduledScalings(0)
  {
    this->resize(622, 512);
    this->setMinimumSize( 622, 512 ) ;
    this->setMaximumSize( 622, 512 ) ;
    QSizePolicy sizepolicy{QSizePolicy::Fixed, QSizePolicy::Fixed} ;
    sizepolicy.setHeightForWidth(true) ;
    this->setSizePolicy(sizepolicy) ;
    //resize(2*622, 2*512) ;

    //auto layout = new QGridLayout(centralwidget) ;
    // if(false) {
    //   m_viewfinder = new QCameraViewfinder(this ) ;
    //   m_viewfinder->resize(622,512) ;
    // }
    //layout->addWidget( m_viewfinder,0,0,1,1 ) ;
    //centralwidget->setLayout( layout ) ;
    //if(true) {

    // Create the scene for this graphics view
    m_scene = new QGraphicsScene{} ;

    // Add the camera viewfinder
    m_viewfinder = new QGraphicsVideoItem{} ;
    //m_viewfinder->setSize( QSize{622,512} ) ;
    m_numPixelsX = 2448 ;
    m_numPixelsY = 2048 ;
    m_viewfinder->setSize( QSize{m_numPixelsX,m_numPixelsY} ) ;
    m_scene->addItem(m_viewfinder) ;
    // I still do not understand how to do this properly
    //resize( m_viewfinder->size().width(),  m_viewfinder->size().height()) ;
    m_nominalscale = size().width() / double(m_viewfinder->size().width()) ;
    scale(m_nominalscale,m_nominalscale) ;
    this->setScene( m_scene ) ;
    
    // Add a rectangle such that we always now where the camerawindow ends (also on my laptop)
    m_viewfinderborder = new QGraphicsRectItem{QRectF{0,0,double(m_numPixelsX),double(m_numPixelsY)}} ;
    {
      QPen pen ;
      pen.setWidth( 10 ) ;
      m_viewfinderborder->setPen(pen) ;
    }
    m_scene->addItem( m_viewfinderborder ) ;
		      
    // let's add a cross at (0,0) in the cameraview. here it would be
    // nice to have some size in 'absolute' coordinates. but for that
    // we need to know the pixel size, which I do not yet do.
    m_localOrigin = QPointF(0.5*m_numPixelsX,0.5*m_numPixelsY) ;

    // add a 'sight' of 100 micron in size
    qreal x0 = m_localOrigin.x() ;
    qreal y0 = m_localOrigin.y() ;
    auto targetsight = new SightMarker( FiducialDefinition{"Center",x0,y0}, 0.050 ) ;
    targetsight->setScale( 1/pixelSize() ) ;
    m_scene->addItem( targetsight ) ;

    //qreal dx = 0.1/pixelSizeX() ; // make a line of 100 micron
    //qreal dy = 0.1/pixelSizeY() ;
    //auto yline = new QGraphicsLineItem(x0,y0-dy,x0,y0+dy) ;
    //auto xline = new QGraphicsLineItem(x0-dx,y0,x0+dx,y0) ;
    //m_scene->addItem( xline ) ;
    //m_scene->addItem( yline ) ;

    // we'll add all the markers in the 'Module' frame. then we'll
    // deal with the transformation of the groups later.

    m_detectorgeometry = new QGraphicsItemGroup{} ;
    m_scene->addItem( m_detectorgeometry ) ;

    m_detectorgeometry->addToGroup( new Substrate() ) ;
    for( const auto& m : GeometrySvc::instance()->jigmarkers() )
      m_detectorgeometry->addToGroup( new JigMarker{m,m_detectorgeometry} ) ;

    auto m_nsidemarkers = new QGraphicsItemGroup{} ;
    m_detectorgeometry->addToGroup( m_nsidemarkers ) ;
    m_nsidemarkers->addToGroup( new Tile( GeometrySvc::instance()->velopixmarkersNSI() ) ) ;
    m_nsidemarkers->addToGroup( new Tile( GeometrySvc::instance()->velopixmarkersNLO() ) ) ;
    for( const auto& m : GeometrySvc::instance()->velopixmarkersNSide() )
      m_nsidemarkers->addToGroup( new VelopixMarker{m,m_nsidemarkers} ) ;
    
    auto m_csidemarkers = new QGraphicsItemGroup{} ;
    m_detectorgeometry->addToGroup( m_csidemarkers ) ;
    m_csidemarkers->addToGroup( new Tile( GeometrySvc::instance()->velopixmarkersCLI() ) ) ;
    m_csidemarkers->addToGroup( new Tile( GeometrySvc::instance()->velopixmarkersCSO() ) ) ;
    for( const auto& m : GeometrySvc::instance()->velopixmarkersCSide() )
      m_csidemarkers->addToGroup( new VelopixMarker{m,m_csidemarkers} ) ;
    
    m_detectorgeometry->setScale( 1/pixelSize() ) ;
    m_detectorgeometry->setPos( x0, y0 ) ;

 

    //marker->setScale(2) ;
    //marker->setScale( 0.001/pixelSize() ) ;
    //m_scene->addItem( marker ) ;
    
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
      qInfo() << "Found a DFK camera" ;
      QCameraViewfinderSettings settings ;
      const int xsize = 2448 ;
      const int ysize = 2048 ;
      settings.setResolution(xsize,ysize) ;
      settings.setMaximumFrameRate(5) ;
      settings.setMinimumFrameRate(5) ;
      settings.setPixelFormat(QVideoFrame::Format_BGR32) ;
      m_camera->setViewfinderSettings( settings ) ;
      m_camera->setViewfinder(m_viewfinder);
      m_viewfinder->setSize( m_camera->viewfinderSettings().resolution() ) ;
      qDebug() << "Resolution is now: "
	       << m_camera->viewfinderSettings().resolution()
	       << m_viewfinder->size() ;
    


    // m_camera->setViewfinder(m_viewfinder);

    // FIXME: check that this line does not break anything!
    
    
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

    } else {
      QCameraViewfinderSettings settings ;
      settings.setResolution(1280,720) ;
      settings.setPixelFormat(QVideoFrame::Format_UYVY) ;
      m_camera->setViewfinderSettings( settings ) ;
      m_viewfinder->setSize( QSize{2448, 2048} ) ;
    }
  }

  void CameraView::setViewDirection( int dir)
  {
    if( m_currentViewDirection != dir ) {
      // reverse the Y axis
      QTransform T = m_detectorgeometry->transform() ;
      T.scale(1,-1) ;
      m_detectorgeometry->setTransform( T ) ;
      m_currentViewDirection = dir > 0 ? NSideView : CSideView ;
    }
  } 
  
  void CameraView::zoomReset()
  {
    /*
    qDebug() << "Camera view size is: "
	     << size() << " "
	     << sizePolicy() ;

    //QTransform t ;
    //t.scale(m_nominalscale,m_nominalscale) ;
    //setTransform(t) ;
    qDebug() << "Position of local origin: "
	     << mapFromScene(m_localOrigin) ;
    qDebug() << "Scale: "
	     << transform().m11() 
	     << transform().m22()  ;
    centerOn( mapFromScene(m_localOrigin) ) ;//622/2,512/2) ;

    qDebug() << "Position of local origin after: "
	     << mapFromScene(m_localOrigin) ;
    */

    fitInView( m_viewfinder,Qt::KeepAspectRatio ) ;
  }

  void CameraView::zoomOut()
  {
    fitInView( sceneRect(),Qt::KeepAspectRatio ) ;
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
  
  void CameraView::scalingTime(qreal /* x */)
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
	sprintf(message,"pos=(%d,%d) --> (%f,%f) pixels --> (%f,%f) mm",x,y,
		local.x()-m_localOrigin.x(),local.y()-m_localOrigin.y(),
		(local.x()-m_localOrigin.x())*pixelSize(),
		(local.y()-m_localOrigin.y())*pixelSize()
		) ;
	
	//QMessageBox dialog(this) ;
	//dialog.setText(message) ;

	QDialog dialog(this) ;
	QVBoxLayout layout ;
	QLabel label(&dialog) ;
	label.setText(message) ;
	layout.addWidget(&label) ;

	QPushButton movebutton("Center",&dialog) ;
	layout.addWidget(&movebutton) ;
	QPushButton recordbutton("Record",&dialog) ;
	layout.addWidget(&recordbutton) ;
	QPushButton closebutton("Close",&dialog) ;
	layout.addWidget(&closebutton) ;
	connect(&movebutton, &QPushButton::clicked, &dialog, &QDialog::accept);
	connect(&recordbutton, &QPushButton::clicked, &dialog, &QDialog::accept);
	connect(&movebutton, &QPushButton::clicked, [=](){ this->moveCameraTo(local) ; } ) ;	
	connect(&recordbutton, &QPushButton::clicked, [=](){ this->record(local) ; } ) ;
	connect(&closebutton, &QPushButton::clicked, &dialog, &QDialog::reject);
	dialog.setLayout( &layout ) ;
	dialog.adjustSize() ;

	// To do this properly, use QSignalMapper?
	// I don't understand how to do this properly if I have more types of actions
	//QDialogButtonBox buttonBox(Qt::Horizontal);
	//buttonBox.addButton("Center",QDialogButtonBox::ActionRole) ;
	//buttonBox.addButton(QDialogButtonBox::Cancel) ;
	//connect(&buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	//connect(&buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	//layout.addWidget(&buttonBox) ;

	/*int ok =*/
	dialog.exec() ;
	//if( ok ) {
	//moveCameraTo( local ) ;
	//}
	//qInfo() << "CameraView: result=" << result << " " << QDialogButtonBox::ActionRole ;
      }
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
    MotionSystemSvc::instance()->mainXAxis().move(mainstagedx.x) ;
    MotionSystemSvc::instance()->mainYAxis().move(mainstagedx.y) ;
  }

  void CameraView::record( QPointF localpoint ) const
  {
    // for now, just print the information
    qInfo() << "Taking a measurement!" ;
    qInfo() << "Local coordinates: " << localpoint.x() << " " << localpoint.y() ;
    auto mscoord= MotionSystemSvc::instance()->coordinates() ;
    qInfo() << "Position of stacks: "
	    << "(" << mscoord.main.x << ","
	    << mscoord.main.y << ")"
	    << "(" << mscoord.stack.x << ","
	    << mscoord.stack.y << ","
	    << mscoord.stack.phi << ")" ;
  }
  
}
