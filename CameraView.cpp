
#include "CameraView.h"
#include "GeometrySvc.h"
#include "MotionSystemSvc.h"
#include "GraphicsItems.h"
#include "CoordinateMeasurement.h"
#include "AutoFocus.h"
#include "VideoRecorder.h"

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
      //m_magnification("Cam.Magnification",4.865),
      m_magnification("Cam.Magnification",4.865),
      //m_magnification("Cam.Magnification",2.159),
      m_rotation("Cam.Rotation",+M_PI/2),
      m_currentViewDirection(NSideView),
      m_numScheduledScalings{0},
      m_videorecorder{ new VideoRecorder{this} }
  {
    qRegisterMetaType<PAP::CoordinateMeasurement>() ;
    
    this->resize(622, 512);
    this->setMinimumSize( 622, 512 ) ;
    //this->setMaximumSize( 622, 512 ) ;
    QSizePolicy sizepolicy{}; //QSizePolicy::Fixed, QSizePolicy::Fixed} ;
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
    m_viewfinder->setToolTip("Camera view") ;
    //m_viewfinder->setSize( QSize{622,512} ) ;
    m_numPixelsX = 2448 ;
    m_numPixelsY = 2048 ;
    m_viewfinder->setSize( QSize{m_numPixelsX,m_numPixelsY} ) ;
    //m_viewfinder->setScale( pixelSize() ) ;
    
    m_scene->addItem(m_viewfinder) ;
    // I still do not understand how to do this properly
    //resize( m_viewfinder->size().width(),  m_viewfinder->size().height()) ;
    m_nominalscale = size().width() / double(m_viewfinder->size().width()) ;
    scale(m_nominalscale,m_nominalscale) ;

    this->setScene( m_scene ) ;

    // Add a rectangle such that we always now where the camerawindow ends (also on my laptop)
    m_viewfinderborder = new QGraphicsRectItem{QRectF{-200,-200,double(m_numPixelsX+400),double(m_numPixelsY+400)}} ;
    m_viewfinderborder->setToolTip("Camera view border") ;
    //m_viewfinderborder->setScale( pixelSize() ) ;
    {
      QPen pen ;
      pen.setWidth( 200 ) ;
      pen.setColor( QColor{1,109,15} ) ;//Qt::red ) ;
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
    auto geosvc = GeometrySvc::instance() ;
    for( const auto& m : geosvc->jigmarkers() )
      m_detectorgeometry->addToGroup( new JigMarker{m,m_detectorgeometry} ) ;

    m_nsidemarkers = new QGraphicsItemGroup{} ;
    m_detectorgeometry->addToGroup( m_nsidemarkers ) ;
    m_nsidemarkers->addToGroup( new Tile( geosvc->velopixmarkersNSI(),m_nsidemarkers) ) ;
    m_nsidemarkers->addToGroup( new Tile( geosvc->velopixmarkersNLO(),m_nsidemarkers ) ) ;
    for( const auto& m : geosvc->velopixmarkersNSide() )
      m_nsidemarkers->addToGroup( new VelopixMarker{m,m_nsidemarkers} ) ;
    for( const auto& m : geosvc->mcpointsNSide() )
      m_nsidemarkers->addToGroup( new SubstrateMarker{m,m_nsidemarkers} ) ;
    
    m_csidemarkers = new QGraphicsItemGroup{} ;
    m_detectorgeometry->addToGroup( m_csidemarkers ) ;
    m_csidemarkers->addToGroup( new Tile{geosvc->velopixmarkersCLI(),m_csidemarkers}) ;
    m_csidemarkers->addToGroup( new Tile{geosvc->velopixmarkersCSO(),m_csidemarkers} ) ;
    for( const auto& m : geosvc->velopixmarkersCSide() )
      m_csidemarkers->addToGroup( new VelopixMarker{m,m_csidemarkers} ) ;
    for( const auto& m : geosvc->mcpointsCSide() )
      m_csidemarkers->addToGroup( new SubstrateMarker{m,m_csidemarkers} ) ;
 
    auto beamline = new SightMarker( FiducialDefinition{"Beamline",0,0}, 2.0 ) ;
    m_detectorgeometry->addToGroup( beamline ) ;
    this->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    m_globalgeometry = new QGraphicsItemGroup{} ;
    m_scene->addItem( m_globalgeometry ) ;

    // Add the coordinates of the pointer, somewhere
    m_textbox = new QGraphicsItemGroup{} ;
    m_markertext = new QGraphicsSimpleTextItem{"markername"} ;
    m_markertext->setBrush( QBrush{QColor{255,69,0} } ) ;
    m_positiontext = new QGraphicsSimpleTextItem{"position"} ;
    m_positiontext->setBrush( QBrush{QColor{255,69,0} } ) ;
    m_markertext->setPos(0,-15) ;
    m_textbox->addToGroup( m_markertext ) ;
    m_textbox->addToGroup( m_positiontext ) ;
    m_textbox->setFlag(QGraphicsItem::ItemIgnoresTransformations,true) ;
    m_scene->addItem( m_textbox ) ;

    // Only do this after all items are created.
    updateGeometryView() ;

    //m_markertext->setFlag(QGraphicsItem::ItemIgnoresTransformations,true) ;

    //m_detectorgeometry->setScale( 1/pixelSize() ) ;
    //m_detectorgeometry->setPos( x0, y0 ) ;

    // connect the signal for movements of the main stage
    connect(MotionSystemSvc::instance(),&MotionSystemSvc::mainStageMoved,
	    this,&CameraView::updateGeometryView) ;
    // connect the signal for movement also to updating the camera position
    connect(&(MotionSystemSvc::instance()->focusAxis().position()),&MonitoredValueBase::valueChanged,
	    this,&CameraView::updatePositionText) ;

    m_stackaxis = new StackAxisMarker() ;
    m_globalgeometry->addToGroup( m_stackaxis ) ;
    connect(MotionSystemSvc::instance(),&MotionSystemSvc::stackStageMoved,
	    this,&CameraView::updateStackAxisView) ;
    updateStackAxisView() ;

    // add functionality to switch turn jig version.
    connect(&geosvc->turnJigVersion(),&NamedInteger::valueChanged,
	    this,&CameraView::updateTurnJigMarkers) ;
    
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
    positionTextBox() ;

    show() ;

    m_autofocus = new AutoFocus{ this, this } ;
  }
  
  
  CameraView::~CameraView()
  {
    delete m_camera ;
  }
  
  void CameraView::setCamera(const QCameraInfo &cameraInfo)
  {
    // we should really read this documentation:
    //     http://doc.qt.io/qt-5/cameraoverview.html
    // among others it explains how to adjust the camera
    // settings. perhaps we can make a nice popup for that.
    
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
      settings.setMaximumFrameRate(5) ; // accepted rates: {1,5,15,25} [Hz]
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
    //connect(m_videoProbe, SIGNAL(videoFrameProbed(QVideoFrame)),
    //	    this, SLOT(processFrame(QVideoFrame)));
    
    m_camera->start();

    } else {
      QCameraViewfinderSettings settings ;
      settings.setResolution(1280,720) ;
      settings.setPixelFormat(QVideoFrame::Format_UYVY) ;
      m_camera->setViewfinderSettings( settings ) ;
      m_viewfinder->setSize( QSize{2448, 2048} ) ;
    }

    
    QCameraImageProcessing *imageProcessing = m_camera->imageProcessing();
    if (imageProcessing->isAvailable()) {
      imageProcessing->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceFluorescent);
    } else {
      qWarning() << "Image processing not available!" ;
    }

    m_videorecorder->setCamera( m_camera ) ;
  }

  void CameraView::updateGeometryView()
  {
    // OK, I figured out something very confusing: QTransforms work
    // from right to left. So, if you want to apply a transformation
    // followed by a rotation, you need to use "T * R". This is
    // different from transforms in ROOT. If you are used to dealing
    // with column vectors, it is also not very logical.
    /*
    qDebug() << "Let's test something: " ;
    QTransform translation ;
    translation.translate(100,200) ;
    QTransform rotation ;
    rotation.rotate(30) ;
    QTransform scaler ;
    scaler.scale(100,-100) ;
    qDebug() << " R * T: " << rotation*translation ;
    qDebug() << " T * R: " << translation*rotation;
    qDebug() << " S * T: " << scaler*translation ;
    qDebug() << " T * S: " << translation*scaler;
    qDebug() << " S * (T * R): " << scaler*(translation*rotation) ;
    qDebug() << " (T*R) * S: " << (translation*rotation)*scaler;
    */
    
    //qDebug() << "CameraView::updateGeometryView()" ;
    const auto geomsvc = GeometrySvc::instance() ;
    // Now watch the order!
    const QTransform T1 = geomsvc->fromCameraToGlobal() ;
    const QTransform T2 = geomsvc->fromModuleToGlobal( m_currentViewDirection ) ;
    const QTransform T1inv = T1.inverted() ;
    const QTransform fromModuleToCamera = T2 * T1inv ;
    // I am totally confused that this works:-)
    m_detectorgeometry->setTransform( fromModuleToCamera * fromCameraToPixel() ) ;

    //
    m_markertext->setText(closestMarkerName()) ;

    // update camera coordinates (such that we can display them elsewhere)
    const QTransform fromCameraToModule = fromModuleToCamera.inverted() ;
    //QTransform fromModuleToGlobal = geomsvc->fromModuleToGlobal(m_currentViewDirection) ;
    //QTransform fromCameraToModule = geomsvc->fromCameraToGlobal() * fromModuleToGlobal.inverted() ;
    auto modulepoint = fromCameraToModule.map( QPointF{0,0} ) ;
    m_cameraCentreInModuleFrame.setValue( modulepoint ) ;
    updatePositionText() ;

    // update the transform for the global frame
    m_globalgeometry->setTransform( T1inv * fromCameraToPixel() ) ;
  }
  
  void CameraView::updateStackAxisView()
  {
    m_stackaxis->setTransform( GeometrySvc::instance()->fromStackToGlobal() ) ;
  }

  void CameraView::resetCamera()
  {
    m_camera->stop() ;
    m_camera->start() ;
  }

  // void CameraView::lockWhiteBalance( bool lock )
  // {
  //   /*
  //   if(lock) m_camera->lock(QCamera::LockWhiteBalance) ;
  //   else     m_camera->unlock(QCamera::LockWhiteBalance) ;
  //   */
  // }

  void CameraView::setViewDirection( ViewDirection dir )
  {
    if( true || m_currentViewDirection != dir ) {
      // reverse the Y axis
      // QTransform T = m_detectorgeometry->transform() ;
      // T.scale(1,-1) ;
      // m_detectorgeometry->setTransform( T ) ;
      m_currentViewDirection = dir ;
      updateGeometryView() ;
    }
  }

  void CameraView::updateTurnJigMarkers()
  {
    // Get the new marker positions
    auto jigmarkers = GeometrySvc::instance()->jigmarkers() ;
    // Go through the entire list to find the right ones
    for( const auto& newmarker : GeometrySvc::instance()->jigmarkers() ) {
      for( const auto& item : m_detectorgeometry->childItems() ) {
	auto jigmarker = dynamic_cast<JigMarker*>( item ) ;
	if( jigmarker && jigmarker->name() == newmarker.name ) {
	  jigmarker->setPos( newmarker.x, newmarker.y ) ;
	  qDebug() << "Found jig marker: " << newmarker.name ;
	}
      }
    }
  }
  
  void CameraView::zoomReset()
  {
    fitInView( m_viewfinder,Qt::KeepAspectRatio ) ;
    positionTextBox() ;
  }

  void CameraView::zoomOut()
  {
    fitInView( sceneRect(),Qt::KeepAspectRatio ) ;
    positionTextBox() ;
  }
  
  void CameraView::positionTextBox()
  {
    QPointF p = mapToScene(0.1*width(),0.9*height()) ;
    m_textbox->setPos(p) ;
  }

  void CameraView::updatePositionText()
  {
    float x = m_cameraCentreInModuleFrame.value().x() ;
    float y = m_cameraCentreInModuleFrame.value().y() ;
    float z = GeometrySvc::instance()->moduleZ(currentViewDirection()) ;
    char textje[256] ;
    sprintf(textje,"camera : (%7.3f,%7.3f,%7.3f)", x,y,z) ;
    m_positiontext->setText(textje) ;
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
    positionTextBox() ;
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
	int x = event->pos().x() ;
	int y = event->pos().y() ;
	QPointF local = mapToScene( x,y ) ;

	char message[256] ;
	// sprintf(message,
	// 	"position (x,y) to center [pixels]: (%d,%d)\n"
	// 	"                [mm]: (%.4f,%.4f)",//x,y,
	// 	int(local.x()-m_localOrigin.x()),
	// 	int(local.y()-m_localOrigin.y()),
	// 	(local.x()-m_localOrigin.x())*pixelSize(),
	// 	(local.y()-m_localOrigin.y())*pixelSize()
	// 	) ;
	QTransform fromModuleToGlobal = GeometrySvc::instance()->fromModuleToGlobal(m_currentViewDirection) ;
	QTransform fromPixelToGlobal =
	  fromCameraToPixel().inverted() * GeometrySvc::instance()->fromCameraToGlobal() ;
	QTransform fromLocalToModule = fromPixelToGlobal * fromModuleToGlobal.inverted() ;
	auto modulepoint = fromLocalToModule.map( local ) ;
	auto globalpoint = fromPixelToGlobal.map( local ) ;
	auto mscoordinates = GeometrySvc::instance()->toMSMain( globalpoint ) ;
	sprintf(message,
		"position wrt camera centre [mm]: (%.4f,%.4f)\n"
		"position in module frame [mm]:   (%.4f,%.4f)\n"
		"position in global frame [mm]:   (%.4f,%.4f)\n"
		"MS main coordinates [mm]:        ((%.4f,%.4f)",
		(local.x()-m_localOrigin.x())*pixelSize(),
		(local.y()-m_localOrigin.y())*pixelSize(),
		modulepoint.x(),modulepoint.y(),
		globalpoint.x(),globalpoint.y(),
		mscoordinates.x,mscoordinates.y
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
	connect(&movebutton, &QPushButton::clicked, [=](){ this->moveCameraTo(local,RelativePosition) ; } ) ;	
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
  
  QTransform CameraView::fromCameraToPixel() const
  {
    // Also here, take into account that we apply operators from left to right, rather than vice versa:
    QTransform T ;
    T.translate( m_localOrigin.x(), m_localOrigin.y() ) ;
    QTransform S ;
    S.scale( 1.0/pixelSizeX(), -1.0/pixelSizeY() ) ;
    return  S * T;
    // The following solution may be a bit faster:
    // QTransform T ;
    // T.scale( 1.0/pixelSizeX(), -1.0/pixelSizeY() ) ;
    // T.translate( m_localOrigin.x()*pixelSizeX(), -m_localOrigin.y()*pixelSizeY() ) ;
    // return T ;
  }

  QTransform CameraView::fromModuleToPixel() const
  {
    return m_detectorgeometry->transform() ;
  }

  
  void CameraView::moveCameraTo( QPointF localpoint, MovementType mode ) const
  {
    //QTransform T = m_detectorgeometry->transform().inverted() ;
    QTransform T = fromCameraToPixel().inverted() * GeometrySvc::instance()->fromCameraToGlobal() ;
    auto globalpoint = T.map( localpoint ) ;
    qDebug() << "Moving to local point: " << localpoint ;
    qDebug() << "Moving to global point: " << globalpoint ;

    // After a long debugging session, it turns out that there is no
    // problem in the transformations. We really just need to split in
    // relative movements and absolute movements. Whether we then use
    // 'move' or 'moveTo' is not so important.
    
    if(mode==RelativePosition) {
      auto globalorigin = T.map( m_localOrigin ) ;
      PAP::Coordinates2D globaldx{
	globalpoint.x() - globalorigin.x(), globalpoint.y() - globalorigin.y() } ;
      // qDebug() << "Local point: " << localpoint ;
      //qDebug() << "Global x, y: "
      //		 << globalpoint.x() << " " << globalpoint.y() ;
      //qDebug() << "Global dx, dy: "
      //		 << globaldx.x() << " " << globaldx.y() ;
      auto mainstagedx = GeometrySvc::instance()->toMSMainDelta( globaldx ) ;
      qDebug() << "Desired change in motor position: "
	       << mainstagedx.x << " " << mainstagedx.y ;
      
      // finally, move the motors!
      MotionSystemSvc::instance()->mainXAxis().move(mainstagedx.x) ;
      MotionSystemSvc::instance()->mainYAxis().move(mainstagedx.y) ;
    } else {
      auto mscoordinates = GeometrySvc::instance()->toMSMain( globalpoint ) ;
      // {
      // 	auto globalorigin = T.map( m_localOrigin ) ;
      // 	auto mainstagex0 = GeometrySvc::instance()->toMSMain( globalorigin ) ;
      // 	auto globaloriginprime = GeometrySvc::instance()->toGlobal( mainstagex0 ) ;
      // 	auto mainstagex0prime = GeometrySvc::instance()->toMSMain( globaloriginprime ) ;
	  
      // 	qDebug() << "Local origin: " << m_localOrigin ;
      // 	qDebug() << "Global origin: " << globalorigin ;
      // 	qDebug() << "Parameters corresponding to origin: "
      // 		 << mainstagex0.x << mainstagex0.y ;
      // 	qDebug() << "Back to global origin: " << globaloriginprime ;
      // 	qDebug() << "Back to local origin: " << T.inverted().map( globaloriginprime ) ;
      // 	qDebug() << "And back to parameters: "
      // 		 << mainstagex0prime.x << mainstagex0prime.y ;
      // }
      MotionSystemSvc::instance()->mainXAxis().moveTo(mscoordinates.x) ;
      MotionSystemSvc::instance()->mainYAxis().moveTo(mscoordinates.y) ;
    }
  }

  namespace {
    const QGraphicsItem* finditemByToolTipText( const QGraphicsItem& parent, const QString& name )
    {
      const QGraphicsItem* rc(0) ;
      if(parent.toolTip() == name ) rc = &parent ;
      else {
	for( const auto& dau : parent.childItems() ) {
	  rc = finditemByToolTipText(*dau,name) ;
	  if(rc) break ;
	}
      }
      return rc ;
    }

    void collectVisibleMarkers(const QGraphicsItem& item,
			       std::vector<const PAP::Marker*>& markers)
    {
      auto m = dynamic_cast<const PAP::Marker*>(&item) ;
      if( m ) {
	if(m->isVisible()) markers.push_back(m) ;
      } else {
	for( const auto& dau : item.childItems() ) 
	  collectVisibleMarkers(*dau,markers) ;
      }
    }
  }

  QStringList CameraView::visibleMarkers() const
  {
    std::vector<const PAP::Marker*> markers ;
    collectVisibleMarkers(*m_detectorgeometry,markers) ;
    QStringList markernames ;
    markernames.reserve( markers.size() ) ;
    std::transform( markers.begin(),
		    markers.end(),
		    std::back_inserter(markernames),
		    [] ( const PAP::Marker* m ) {
		      return m->name() ; } ) ;
    return markernames ;
  }
  
  void CameraView::moveCameraTo( const QString& markername, bool useDefaultFocus ) const
  {
    const QGraphicsItem* marker = finditemByToolTipText(*m_detectorgeometry, markername) ;
    if(marker) {
      // get the coordinates in the scene
      QPointF localcoord = m_detectorgeometry->mapToScene( marker->pos() ) ;
      moveCameraTo( localcoord, AbsolutePosition ) ;
      if( useDefaultFocus ) autofocus()->applyMarkerFocus( markername ) ;
    } else {
      qWarning() << "Cannot find graphics item: " << markername ;
    }
  }

  QPointF CameraView::globalPosition( const QString& markername ) const
  {
    QPointF rc ;
    const QGraphicsItem* marker = finditemByToolTipText(*m_detectorgeometry, markername) ;
    if(marker) {
      // get the global coordinates
      QTransform T = GeometrySvc::instance()->fromModuleToGlobal(m_currentViewDirection) ;
      rc = T.map( marker->pos() ) ;
    }
    return rc ;
  }

  const PAP::Marker* CameraView::closestMarker(const QPointF& localpoint ) const
  {
    const PAP::Marker* rc(0) ;
    std::vector<const PAP::Marker*> markers ;
    collectVisibleMarkers(*m_detectorgeometry,markers) ;
    double dist2=0 ;
    for( const auto& m : markers ) {
      const QPointF mpos = m_detectorgeometry->mapToScene( m->pos() ) ;
      const double dx = mpos.x()-localpoint.x() ;
      const double dy = mpos.y()-localpoint.y() ;
      double thisdist2 = dx*dx+dy*dy ;
      if( !rc || thisdist2 < dist2 ) {
	dist2 = thisdist2 ;
	rc = m ;
      }
    }
    return rc ;
  }

  QString CameraView::closestMarkerName() const
  {
    const PAP::Marker* m = closestMarker() ;
    return m ? m->name() : QString{} ;
  }

  QPointF CameraView::globalCoordinates( QPointF localpoint ) const
  {
    QTransform T = fromCameraToPixel().inverted() * GeometrySvc::instance()->fromCameraToGlobal() ;
    //GeometrySvc::instance()->fromCameraToGlobal().inverted() * fromCameraToPixel() ;
    return T.map( localpoint ) ;
  }
  
  CoordinateMeasurement CameraView::coordinateMeasurement( QPointF localpoint ) const
  {
    qInfo() << "Taking a measurement!" ;
    qInfo() << "Local coordinates: " << localpoint.x() << " " << localpoint.y() ;
    auto mscoord= MotionSystemSvc::instance()->coordinates() ;
    qInfo() << "Position of stacks: "
	    << "(" << mscoord.main.x << ","
	    << mscoord.main.y << ")"
	    << "(" << mscoord.stack.x << ","
	    << mscoord.stack.y << ","
	    << mscoord.stack.phi << ")" ;
    CoordinateMeasurement measurement ;
    measurement.mscoordinates = mscoord ;
    measurement.globalcoordinates = globalCoordinates( localpoint ) ;
    // Let's also add the name of the marker
    measurement.markername = closestMarkerName() ;
    // let's fill the module coordinates
    const auto viewdir = currentViewDirection() ;
    const auto& geosvc = GeometrySvc::instance() ;
    const QTransform fromGlobalToModule = geosvc->fromModuleToGlobal(viewdir).inverted() ;
    const auto modulecoordinatesXY = fromGlobalToModule.map( measurement.globalcoordinates ) ;
    measurement.modulecoordinates.setX( modulecoordinatesXY.x() ) ;
    measurement.modulecoordinates.setY( modulecoordinatesXY.y() ) ;
    measurement.modulecoordinates.setZ( geosvc->moduleZ( viewdir, mscoord.focus ) ) ;
    
    return measurement ;
  }
  
  void CameraView::record( QPointF localpoint ) const
  {
    emit recording(coordinateMeasurement(localpoint)) ;
  }
  
}
