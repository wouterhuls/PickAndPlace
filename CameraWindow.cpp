#include "CameraWindow.h"
#include "CameraView.h"
#include "MotionSystemSvc.h"
#include "AutoFocus.h"
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
    resize(900,800);
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
    layout->addLayout( hlayout) ;
    hlayout->setObjectName(QStringLiteral("CamWindow::hlayout"));
    auto focusbutton = new QPushButton("Focus",this) ;
    focusbutton->setObjectName(QStringLiteral("focusButton"));
    hlayout->addWidget( focusbutton ) ;

    auto quitbutton = new QPushButton("Quit",this) ;
    quitbutton->setObjectName(QStringLiteral("quitButton"));
    hlayout->addWidget( quitbutton ) ;
   
    auto resetzoombutton = new QPushButton("Reset zoom",this) ;
    connect( resetzoombutton, &QPushButton::clicked,  m_cameraview, &CameraView::zoomReset ) ;
    hlayout->addWidget( resetzoombutton ) ;
    
    auto zoomoutbutton = new QPushButton("Zoom out",this) ;
    connect( zoomoutbutton, &QPushButton::clicked,  m_cameraview, &CameraView::zoomOut ) ;
    hlayout->addWidget( zoomoutbutton ) ;

    auto viewtogglebutton = new QPushButton{"Toggle view",this} ;
    viewtogglebutton->setCheckable(true) ;
    connect( viewtogglebutton, &QAbstractButton::toggled, this, &CameraWindow::toggleView ) ;
    hlayout->addWidget(viewtogglebutton) ;

    m_showNSideTiles = new QCheckBox{"N-side markers",this} ;
    connect( m_showNSideTiles, &QCheckBox::stateChanged,m_cameraview,&CameraView::showNSideMarkers ) ;
    m_showCSideTiles = new QCheckBox{"C-side markers",this} ;
    connect( m_showCSideTiles, &QCheckBox::stateChanged,m_cameraview,&CameraView::showCSideMarkers ) ;
    hlayout->addWidget( m_showNSideTiles) ;
    hlayout->addWidget( m_showCSideTiles ) ;
    if( m_cameraview->currentViewDirection()==PAP::NSideView ) {
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
    layout->addWidget( m_cameraview ) ;
    //m_cameraview->show() ;

    m_autofocus = new AutoFocus{ m_cameraview, this } ;

    auto taskpages = new QTabWidget{this} ;
    layout->addWidget( taskpages ) ;

    auto page1widget = new QWidget{} ;
    taskpages->addTab(page1widget,"Calibrate N-side") ;
    {
      auto vlayout = new QVBoxLayout{} ;
      page1widget->setLayout(vlayout) ;
      auto tmptext1 = new QLabel{page1widget} ;
      tmptext1->setText("a. place the master jig in the machine\n"
			"b. press the 'C' or 'NC' button to identify the side\n"
			"c. press 'zoom master jig marker 1' button to move first jig marker in field of view of camera\n"
			"d. press the record button to record the position of the marker in the view\n"
			"e. press 'zoom master jig marker 2' button to move the second marker in field of view\n"
			"f. press the record button to record the position of the marker in the view\n"
			"g. press the 'calibrate jig coordinate frame' button") ;
      tmptext1->setWordWrap(true);
      vlayout->addWidget( tmptext1 ) ;
      
      auto movetomarker1button = new QPushButton{"Move to marker 1", this} ;
      connect(movetomarker1button,&QPushButton::clicked,[=](){ m_cameraview->moveCameraTo("MainJigMarker1") ; } ) ;
      vlayout->addWidget( movetomarker1button ) ;
      
      auto movetomarker2button = new QPushButton{"Move to marker 2", this} ;
      connect(movetomarker2button,&QPushButton::clicked,[=](){ m_cameraview->moveCameraTo("MainJigMarker2") ; } ) ;
      vlayout->addWidget( movetomarker2button ) ;
      
    }
    
    auto page2widget = new QWidget{} ;
    taskpages->addTab(page2widget,"Position NSI") ;
    {
      auto vlayout = new QVBoxLayout{} ;
      page2widget->setLayout(vlayout) ;
      auto movetomarker1button = new QPushButton{"Move to marker 1", this} ;
      connect(movetomarker1button,&QPushButton::clicked,[=](){ m_cameraview->moveCameraTo("NSI_VP20_Fid1") ; } ) ;
      vlayout->addWidget( movetomarker1button ) ;
      
      auto movetomarker2button = new QPushButton{"Move to marker 2", this} ;
      connect(movetomarker2button,&QPushButton::clicked,[=](){ m_cameraview->moveCameraTo("NSI_VP22_Fid2") ; } ) ;
      vlayout->addWidget( movetomarker2button ) ;
    }

    auto page3widget = new QWidget{} ;
    taskpages->addTab(page3widget,"Position tile 2") ;

    auto page4widget = new QWidget{} ;
    taskpages->addTab(page4widget,"Calibrate C-side") ;

    auto page5widget = new QWidget{} ;
    taskpages->addTab(page5widget,"Position tile 3") ;

    auto page6widget = new QWidget{} ;
    taskpages->addTab(page6widget,"Position tile 4") ;
    
    QMetaObject::connectSlotsByName(this);
  }

  void CameraWindow::on_quitButton_clicked()
  {
    QCoreApplication::quit();
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
