#include "CameraWindow.h"
#include "CameraView.h"
#include "MotionSystemSvc.h"

#include "AutoFocus.h"
#include "AlignPages.h"
#include "CameraImageProcessingDialog.h"

#include <iostream>
#include <sstream>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVideoProbe>
#include <QSlider>
#include <QLabel>
#include <QCoreApplication>
#include <QCheckBox>
#include <QTextEdit>
#include <QCamera>
#include <QVideoFrame>
#include <QFileDialog>

namespace PAP
{
  CameraWindow::CameraWindow(QWidget *parent)
    : QMainWindow(parent),
      m_stillImageTriggered{false}
  {
    resize(900,500);
    setWindowTitle("Velo Pick&Place") ;
    
    m_cameraview = new CameraView{this} ;
    m_autofocus = new AutoFocus{ m_cameraview, this } ;
    
    // add a vertical layout. if we derive from 'mainwindow', then
    // layout must be set to the central widget.
    // Set layout in QWidget
    QWidget *window = new QWidget{this};
    auto layout = new QVBoxLayout{} ;//(widget);
    layout->setObjectName(QStringLiteral("CamWindow::layout"));
    layout->setContentsMargins(0, 0, 0, 0);

    setCentralWidget(window);
    window->setLayout(layout);

    //for now add a horizontal bar with buttons
    auto hlayout = new QHBoxLayout{} ;
    auto buttonlayout = new QVBoxLayout{} ;
    layout->addLayout( hlayout) ;
    
    buttonlayout->setObjectName(QStringLiteral("CamWindow::hlayout"));

    auto stopbutton = new QPushButton{QIcon(":/images/emergencystop.png"),"",this} ;
    stopbutton->setObjectName(QStringLiteral("stopButton"));
    stopbutton->setToolTip("Abort all motions") ;
    stopbutton->setIconSize(QSize(100,100)) ;
    buttonlayout->addWidget( stopbutton ) ;
    
    auto focusbutton = new QPushButton("Auto-Focus",this) ;
    focusbutton->setToolTip("Start autofocus sequence") ;
    focusbutton->setObjectName(QStringLiteral("focusButton"));
    buttonlayout->addWidget( focusbutton ) ;

    auto markerfocusbutton = new QPushButton("Marker focus",this) ;
    markerfocusbutton->setToolTip("Move to the default focus position of the closest marker") ;
    connect( markerfocusbutton, &QPushButton::clicked, m_autofocus, &AutoFocus::applyMarkerFocus ) ;
    buttonlayout->addWidget( markerfocusbutton ) ;
    
    if( m_cameraview->camera() ) {
      QCameraImageProcessing *imageProcessing = m_cameraview->camera()->imageProcessing();
      if (imageProcessing->isAvailable()) {
    	auto camerasettingsdialog = new CameraImageProcessingDialog( *imageProcessing,this) ;
    	auto camerasettingsbutton = new QPushButton("Settings",this) ;
    	camerasettingsbutton->setToolTip("Show camera settings dialog") ;
    	connect(camerasettingsbutton,&QPushButton::clicked,[=](){ camerasettingsdialog->show() ; } ) ;
    	buttonlayout->addWidget(camerasettingsbutton) ;
      } 
    }
    
    //auto quitbutton = new QPushButton("Quit",this) ;
    //quitbutton->setObjectName(QStringLiteral("quitButton"));
    //buttonlayout->addWidget( quitbutton ) ;
   
    auto resetzoombutton = new QPushButton("Reset zoom",this) ;
    resetzoombutton->setToolTip("Reset zoom to camera view") ;
    connect( resetzoombutton, &QPushButton::clicked,  m_cameraview, &CameraView::zoomReset ) ;
    buttonlayout->addWidget( resetzoombutton ) ;
        
    auto zoomoutbutton = new QPushButton("Zoom out",this) ;
    zoomoutbutton->setToolTip("Zoom out to full detector view") ;
    connect( zoomoutbutton, &QPushButton::clicked,  m_cameraview, &CameraView::zoomOut ) ;
    buttonlayout->addWidget( zoomoutbutton ) ;

    auto viewtogglebutton = new QPushButton{"Toggle view",this} ;
    viewtogglebutton->setToolTip("Toggle between C and N side view") ;
    viewtogglebutton->setCheckable(true) ;
    connect( viewtogglebutton, &QAbstractButton::toggled, this, &CameraWindow::toggleView ) ;
    buttonlayout->addWidget(viewtogglebutton) ;

    m_showNSideTiles = new QCheckBox{"N-side markers",this} ;
    connect( m_showNSideTiles, &QCheckBox::stateChanged,m_cameraview,&CameraView::showNSideMarkers ) ;
    m_showCSideTiles = new QCheckBox{"C-side markers",this} ;
    connect( m_showCSideTiles, &QCheckBox::stateChanged,m_cameraview,&CameraView::showCSideMarkers ) ;
    buttonlayout->addWidget( m_showNSideTiles) ;
    buttonlayout->addWidget( m_showCSideTiles ) ;
    if( m_cameraview->currentViewDirection()==PAP::CSideView ) {
      m_cameraview->showNSideMarkers( false ) ;
      m_showNSideTiles->setCheckState( Qt::Unchecked ) ;
      m_showCSideTiles->setCheckState( Qt::Checked ) ;
    } else {
      m_cameraview->showCSideMarkers( false ) ;
      m_showNSideTiles->setCheckState( Qt::Checked ) ;
      m_showCSideTiles->setCheckState( Qt::Unchecked ) ;
    }

    auto cameraresetbutton = new QPushButton{"Reset camera",this} ;
    cameraresetbutton->setToolTip("Reset the camera if the view gets stuck.") ;
    connect(cameraresetbutton , &QAbstractButton::clicked, [&]() { m_cameraview->resetCamera(); } ) ;
    buttonlayout->addWidget(cameraresetbutton) ;

    auto camerastillimagebutton = new QPushButton{"Take image",this} ;
    camerastillimagebutton->setToolTip("Take a still image and save to file") ;
    connect(camerastillimagebutton, &QAbstractButton::clicked, [&]() { m_stillImageTriggered=true; } ) ;
    buttonlayout->addWidget(camerastillimagebutton) ;
    
    connect(m_cameraview->videoProbe(), SIGNAL(videoFrameProbed(QVideoFrame)),
	    this, SLOT(processFrame(QVideoFrame)));

    /*
    auto lockwhitebalancebutton = new QPushButton{"Lock white balance",this} ;
    lockwhitebalancebutton->setToolTip("Lock the white balance of the camera view") ;
    lockwhitebalancebutton->setCheckable(true) ;
    connect(lockwhitebalancebutton, &QAbstractButton::clicked,
	    [&]() { m_cameraview->lockWhiteBalance(lockwhitebalancebutton->toggled()); } ) ;
    buttonlayout->addWidget(lockwhitebalancebutton) ;
    */
    
    //m_cameraview->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
    //m_cameraview->setGeometry(200,200,200,200);
    hlayout->addWidget( m_cameraview ) ;
    hlayout->addLayout( buttonlayout ) ;
    //m_cameraview->show() ;



    auto taskpages = new QTabWidget{this} ;
    layout->addWidget( taskpages ) ;
    //auto mainjigalignwidget = new AlignMainJigPage{m_cameraview} ;

    {
      auto nsidetaskpages = new QTabWidget{ taskpages } ;
      taskpages->addTab(nsidetaskpages,"N-side") ;
      auto mainjigalignwidget = new AlignMainJigPage{m_cameraview} ;
      nsidetaskpages->addTab(mainjigalignwidget,"Align jig") ;
      nsidetaskpages->addTab(new AlignTilePage{m_cameraview,"NSI","NSI_VP20_Fid1","NSI_VP22_Fid2"},"Align NSI") ;
      nsidetaskpages->addTab(new AlignTilePage{m_cameraview,"NLO","NLO_VP10_Fid1","NLO_VP12_Fid2"},"Align NLO") ;
    }

    {
      auto csidetaskpages = new QTabWidget{ taskpages } ;
      taskpages->addTab(csidetaskpages,"C-side") ;
      auto mainjigalignwidget = new AlignMainJigPage{m_cameraview} ;
      csidetaskpages->addTab(mainjigalignwidget,"Align jig") ;
      csidetaskpages->addTab(new AlignTilePage{m_cameraview,"CLI","CLI_VP00_Fid1","CLI_VP02_Fid2"},"Align CLI") ;
      csidetaskpages->addTab(new AlignTilePage{m_cameraview,"CSO","CSO_VP30_Fid1","CSO_VP32_Fid2"},"Align CSO") ;
    }
    connect( taskpages, &QTabWidget::tabBarClicked, this, &CameraWindow::toggleView ) ;
    
    QMetaObject::connectSlotsByName(this);
  }

  void CameraWindow::on_quitButton_clicked()
  {
    QCoreApplication::quit();
  }

  void CameraWindow::on_stopButton_clicked()
  {
    MotionSystemSvc::instance()->emergencyStop() ;
  }

  void CameraWindow::on_focusButton_clicked()
  {
    m_autofocus->show() ;
    //m_autofocus->startFocusSequence() ;
  }

  void CameraWindow::toggleView( int view )
  {
    if( view == int(PAP::NSideView) ) {
      m_cameraview->setViewDirection( PAP::NSideView ) ;
      m_showNSideTiles->setCheckState( Qt::Checked ) ;
      m_showCSideTiles->setCheckState( Qt::Unchecked ) ;
    } else {
      m_cameraview->setViewDirection( PAP::CSideView ) ;
      m_showNSideTiles->setCheckState( Qt::Unchecked ) ;
      m_showCSideTiles->setCheckState( Qt::Checked ) ;
    } 
  }

  void CameraWindow::processFrame( const QVideoFrame& frame )
  {
    if( m_stillImageTriggered ) {
      m_stillImageTriggered = false ;
      // pop up a window asking for a filename
      auto filename = QFileDialog::getSaveFileName(this, tr("Save image"),
						   "/home/velouser/Documents/PickAndPlaceData/images/untitled.jpg",
						   tr("Images (*.png *.xpm *.jpg)"));
      
      if(!filename.isEmpty()) {
      // there must be a better way than this ...
	const_cast<QVideoFrame&>(frame).map(QAbstractVideoBuffer::ReadOnly) ;
	if( frame.bits() &&
	    //(frame.pixelFormat() == QVideoFrame::Format_BGR32||
	    frame.pixelFormat() == QVideoFrame::Format_RGB32 ) {
	  QImage::Format imageFormat =
	    QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
	  QImage img( frame.bits(),
		      frame.width(),
		      frame.height(),
		      frame.bytesPerLine(),
		      imageFormat);
	  img.save(filename) ;
	}
	const_cast<QVideoFrame&>(frame).unmap() ;
      }
    }
  }
}
