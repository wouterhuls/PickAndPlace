#include "CameraWindow.h"
#include "CameraView.h"
#include "MotionSystemSvc.h"

#include "AutoFocus.h"
#include "AlignPages.h"

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


namespace PAP
{
  CameraWindow::CameraWindow(QWidget *parent)
    : QMainWindow(parent)
  {
    resize(900,500);
    m_cameraview = new CameraView{} ;
    
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
    
    auto focusbutton = new QPushButton("Focus",this) ;
    focusbutton->setToolTip("Start autofocus sequence") ;
    focusbutton->setObjectName(QStringLiteral("focusButton"));
    buttonlayout->addWidget( focusbutton ) ;

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
    
    //m_cameraview->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
    //m_cameraview->setGeometry(200,200,200,200);
    hlayout->addWidget( m_cameraview ) ;
    hlayout->addLayout( buttonlayout ) ;
    //m_cameraview->show() ;

    m_autofocus = new AutoFocus{ m_cameraview, this } ;

    auto taskpages = new QTabWidget{this} ;
    layout->addWidget( taskpages ) ;

    {
      auto nsidetaskpages = new QTabWidget{ taskpages } ;
      taskpages->addTab(nsidetaskpages,"N-side") ;
      
      auto page1widget = new AlignMainJigPage{m_cameraview} ;
      nsidetaskpages->addTab(page1widget,"Align jig") ;
      nsidetaskpages->addTab(new AlignTilePage{m_cameraview,"NSI_VP20_Fid1","NSI_VP22_Fid2"},"Align NSI") ;
      nsidetaskpages->addTab(new AlignTilePage{m_cameraview,"NLO_VP30_Fid1","NLO_VP32_Fid2"},"Align NLO") ;
    }

    {
      auto csidetaskpages = new QTabWidget{ taskpages } ;
      taskpages->addTab(csidetaskpages,"C-side") ;
      
      auto page1widget = new AlignMainJigPage{m_cameraview} ;
      csidetaskpages->addTab(page1widget,"Align jig") ;
      csidetaskpages->addTab(new AlignTilePage{m_cameraview,"CLI_VP00_Fid1","CLI_VP02_Fid2"},"Align CLI") ;
      csidetaskpages->addTab(new AlignTilePage{m_cameraview,"CSO_VP10_Fid1","CSO_VP12_Fid2"},"Align CSO") ;
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
    m_autofocus->startFocusSequence() ;
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
  
}
