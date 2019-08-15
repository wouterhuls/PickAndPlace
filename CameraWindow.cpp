#include "CameraWindow.h"
#include "CameraView.h"
#include "MotionSystemSvc.h"

#include "AutoFocus.h"
#include "AlignPages.h"
#include "CameraImageProcessingDialog.h"
#include "MetrologyPages.h"

#include <iostream>
#include <sstream>
#include <array>
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
#include <QInputDialog>
#include <QTableWidget>
#include <QMenuBar>
#include <QProcessEnvironment>
#include "MotionSystemWidget.h"
#include "PropertySvc.h"

namespace PAP
{
  CameraWindow::CameraWindow(QWidget *parent)
    : QMainWindow(parent),
      m_moduleName{"ModuleName","NRD999"},
      m_stillImageTriggered{false}
  {
    resize(700,500);
    setWindowTitle("Velo Pick&Place") ;

    m_cameraview = new CameraView{this} ;
    m_autofocus = new AutoFocus{ m_cameraview, this } ;
    m_autofocus->move(950,600) ;
    m_autofocus->show() ;

    // create dialog for the motion system interface
    m_motionsystemdialog = new QDialog{ this } ;
    m_motionsystemdialog->resize(600,500) ;
    m_motionsystemdialog->move(950,0) ;
    m_motionsystemdialog->setWindowIcon( QIcon(":/images/VeloUpgradeLogoSmall.png") ) ;
    m_motionsystemdialog->setWindowTitle("Motion System Window") ;
    new PAP::MotionSystemWidget(m_motionsystemdialog);
    m_motionsystemdialog->show() ;
    
    //m_measurementreport = new MetrologyReportPage{*this} ;
    
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
 
    // start with a label for the name. perhaps we should make a new
    // class that does all of this.
    {
      m_moduleNameButton = new QPushButton{m_moduleName.value(),this} ;
      QFont font = m_moduleNameButton->font() ;
      font.setBold(true) ;
      font.setPointSize(32) ;
      m_moduleNameButton->setFont(font) ;
      //moduleNameButton->setFlat(true) ;
      buttonlayout->addWidget( m_moduleNameButton ) ;
      // update the label if the value changes
      connect(&m_moduleName,&MonitoredValueBase::valueChanged,[&]() {
	  m_moduleNameButton->setText( m_moduleName.value() ) ;
	}) ;
      // pop up a dialog to change the name if the label is pressed
      connect(m_moduleNameButton,&QPushButton::clicked, [&]() {
	  QString v = this->m_moduleName.value() ;
	  bool ok{false} ;
	  QString d = QInputDialog::getText(this,v,v,QLineEdit::Normal,v, &ok) ;
	  if(ok) this->m_moduleName.setValue(d) ;
	}) ;
    }
    // buttons to switch view
    {
      auto togglehlayout = new QHBoxLayout{} ;
      buttonlayout->addLayout( togglehlayout ) ;
      auto nsidebutton = new QPushButton{"N-side",this} ;
      auto csidebutton = new QPushButton{"C-side",this} ;
      nsidebutton->setCheckable(true) ;
      csidebutton->setCheckable(true) ;
      togglehlayout->setSpacing(0) ;
      togglehlayout->addWidget(nsidebutton) ;
      nsidebutton->setChecked(true) ;
      connect( csidebutton,  &QAbstractButton::clicked, [=]() {
	  csidebutton->setChecked(true ) ;
	  nsidebutton->setChecked(false) ;
	  toggleView( ViewDirection::CSideView ) ;
	  qDebug() << "C:" << csidebutton->isChecked() << nsidebutton->isChecked() ;
	} ) ;
      connect( nsidebutton,  &QAbstractButton::clicked, [=]() {
	  csidebutton->setChecked(false ) ;
	  nsidebutton->setChecked(true) ;
	  toggleView( ViewDirection::NSideView ) ;
	  //csidebutton->setChecked(!nsidebutton->isChecked()) ;
	  qDebug() << "N: " << nsidebutton->isChecked() << csidebutton->isChecked() ;
	} ) ;
      if( m_cameraview->currentViewDirection()==PAP::CSideView ) {
	csidebutton->setChecked(true ) ;
      } else {
	nsidebutton->setChecked(true ) ;
      }
      
      /*connect( nsidebutton,  &QAbstractButton::pressed, [=]() {
	csidebutton->setChecked(!nsidebutton->isChecked()) ; } ) ;*/
      
      //connect( viewtogglebutton, &QAbstractButton::toggled, this, &CameraWindow::toggleView ) ;
      togglehlayout->addWidget(csidebutton) ;
    }
    
    // auto msbutton = new QPushButton("Motion System",this) ;
    // connect( msbutton, &QPushButton::clicked, [=](){ m_motionsystemdialog->show() ; }) ;
    // buttonlayout->addWidget( msbutton ) ;
    
    // auto focusbutton = new QPushButton("Auto-Focus",this) ;
    // focusbutton->setToolTip("Start autofocus sequence") ;
    // focusbutton->setObjectName(QStringLiteral("focusButton"));
    // buttonlayout->addWidget( focusbutton ) ;

    auto markerfocusbutton = new QPushButton("Marker focus",this) ;
    markerfocusbutton->setToolTip("Move to the default focus position of the closest marker") ;
    connect( markerfocusbutton, &QPushButton::clicked, m_autofocus, &AutoFocus::applyMarkerFocus ) ;
    buttonlayout->addWidget( markerfocusbutton ) ;

    {
      auto button = new QPushButton("Move to marker",this) ;
      button->setToolTip("Move camera to one of the default markers") ;
      connect( button, &QPushButton::clicked, [&](){ this->moveToMarker(); } ) ;
      buttonlayout->addWidget( button ) ;
    }
    
    {
      auto button = new QPushButton("Move to position",this) ;
      button->setToolTip("Move camera given position in module frame") ;
      connect( button, &QPushButton::clicked, [&](){ this->moveToPositionInModuleFrame(); } ) ;
      buttonlayout->addWidget( button ) ;
    }
    
    // {
    //   auto button = new QPushButton("MeasurementReport",this) ;
    //   button->setToolTip("Pop-up meusurementreportwindow") ;
    //   connect( button, &QPushButton::clicked, [&](){ m_measurementreport->show() ; } ) ;
    //   buttonlayout->addWidget( button ) ;
    // }
    
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

    // auto viewtogglebutton = new QPushButton{"Toggle view",this} ;
    // viewtogglebutton->setToolTip("Toggle between C and N side view") ;
    // viewtogglebutton->setCheckable(true) ;
    // connect( viewtogglebutton, &QAbstractButton::toggled, this, &CameraWindow::toggleView ) ;
    // buttonlayout->addWidget(viewtogglebutton) ;

    m_showNSideTiles = new QCheckBox{"N-side markers",this} ;
    connect( m_showNSideTiles, &QCheckBox::stateChanged,m_cameraview,&CameraView::showNSideMarkers ) ;
    m_showCSideTiles = new QCheckBox{"C-side markers",this} ;
    connect( m_showCSideTiles, &QCheckBox::stateChanged,m_cameraview,&CameraView::showCSideMarkers ) ;
    buttonlayout->addWidget( m_showNSideTiles ) ;
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
    auto showGeometryButton = new QCheckBox{"Outlines",this} ;
    connect( showGeometryButton, &QCheckBox::stateChanged,m_cameraview,&CameraView::showGeometry ) ;
    showGeometryButton->setCheckState( Qt::Checked ) ;
    buttonlayout->addWidget( showGeometryButton ) ;
    
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
    
    QTabWidget* apages[2] ;
    QTabWidget* mpages[2] ;
    QTabWidget* ppages[2] ;

    for( int iview=0; iview<2; ++iview ) {
      ViewDirection view = ViewDirection(iview) ;
      
      //auto sidetaskpages = new QTabWidget{ taskpages } ;
      //taskpages->addTab(sidetaskpages,view==ViewDirection::NSideView ? "N-side" : "C-side") ;
      
      apages[view] = new QTabWidget{} ;
      apages[view]->addTab( new AlignMainJigPage{view,m_cameraview},"Align jig XY") ;
      apages[view]->addTab( makeAlignMainJigZPage(view,*this),"Align jig Z") ;
           
      ppages[view] = new QTabWidget{} ;
      for(int tile=0; tile<2; ++tile) {
	const auto ti = getTileInfo(view,TileType(tile)) ;
	ppages[view]->addTab(new AlignTilePage{m_cameraview,ti},
			     QString{"Position "} + ti.name) ;
      }
      
      mpages[view] = createSideMetrologyPage(*this,view) ;
    }

    connect(this, &CameraWindow::viewToggled, this, [=](int view)
	    {
	      taskpages->removeTab(2) ;
	      taskpages->removeTab(1) ;
	      taskpages->removeTab(0) ;
	      taskpages->addTab(apages[view],"Jig alignment") ;
	      taskpages->addTab(ppages[view],"Tile positioning") ;
	      taskpages->addTab(mpages[view],"Metrology") ;
	    } ) ;
    // need to bootstrap this
    viewToggled(m_cameraview->currentViewDirection()) ;

    createMenus() ;
   
    //connect( taskpages, &QTabWidget::tabBarClicked, this, &CameraWindow::toggleView ) ;
    
    QMetaObject::connectSlotsByName(this);
  }

  // void CameraWindow::on_quitButton_clicked()
  // {
  //   QCoreApplication::quit();
  // }

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
    if(m_cameraview->currentViewDirection() != view ) {
      if( view == int(PAP::NSideView) ) {
	m_cameraview->setViewDirection( PAP::NSideView ) ;
	m_showNSideTiles->setCheckState( Qt::Checked ) ;
	m_showCSideTiles->setCheckState( Qt::Unchecked ) ;
      } else {
	m_cameraview->setViewDirection( PAP::CSideView ) ;
	m_showNSideTiles->setCheckState( Qt::Unchecked ) ;
	m_showCSideTiles->setCheckState( Qt::Checked ) ;
      }
      emit viewToggled(view) ;
    }
  }
  
  void CameraWindow::moveToMarker()
  {
    // pop-up a dialog with all markers such that the use can choose one to move to
    auto markers = m_cameraview->visibleMarkers() ;
    bool ok;
    QString item = QInputDialog::getItem(this, tr("Marker"),
					   tr("Season:"), markers, 0, false, &ok);
    if (ok && !item.isEmpty()) {
      m_cameraview->moveCameraTo( item ) ;
    }
  }

  void CameraWindow::processFrame( const QVideoFrame& frame )
  {
    if( m_stillImageTriggered ) {
      m_stillImageTriggered = false ;
      // pop up a window asking for a filename
      auto filename = QFileDialog::getSaveFileName(this, tr("Save image"),
						   QString("/home/velouser/Documents/PickAndPlaceData/") +
						   m_moduleName.value() + "/images/untitled.jpg",
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

  /*
  void CameraWindow::createActions()
  {
    
    QMenu *fileMenu = menuBar()->addMenu(tr("&Config"));
    QToolBar *fileToolBar = addToolBar(tr("Config"));
  } ;
  */

  void CameraWindow::moveToPositionInModuleFrame()
  {
    // pop up a dialog with a table where we can fill in an x and a y value in the module frame
    QDialog dialog(this) ;
    QTableWidget table{1,2,this} ;
    table.setHorizontalHeaderLabels(QStringList{"X","Y"}) ;
    // set to the current module position
    auto point = m_cameraview->cameraCentreInModuleFrame().value() ;
    table.setItem(0,0,new QTableWidgetItem{QVariant{point.x()}.toString()} ) ;
    table.setItem(0,1,new QTableWidgetItem{QVariant{point.y()}.toString()} ) ;
    table.resize(80,20) ;
    QVBoxLayout layout ;
    layout.addWidget(&table) ;
    dialog.setLayout( &layout ) ;
    QHBoxLayout hlayout ;
    layout.addLayout(&hlayout) ;
    QPushButton cancelbutton("Cancel",&dialog) ;
    hlayout.addWidget(&cancelbutton) ;
    connect(&cancelbutton, &QPushButton::clicked, &dialog, &QDialog::reject);
    QPushButton acceptbutton("Move",&dialog) ;
    hlayout.addWidget(&acceptbutton) ;
    connect(&acceptbutton, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.adjustSize() ;
    if ( dialog.exec() == QDialog::Accepted ) {
      double x = QVariant{table.item(0,0)->text()}.toDouble() ;
      double y = QVariant{table.item(0,1)->text()}.toDouble() ;
      m_cameraview->moveCameraToPointInModule(QPointF{x,y}) ;
    }
  }

  // this belongs somewhere else, but I'm lazy
  TileInfo getTileInfo( ViewDirection view, TileType tile )
  {
    // I CANNOT STAND INEFFICIENCY. SHIT.
    const std::array<std::array<TileInfo,2>,2> tileinfos =
      { std::array<TileInfo,2>{TileInfo{"NSI","NSI_VP20_Fid1","NSI_VP22_Fid2"},
	 TileInfo{"NLO","NLO_VP10_Fid1","NLO_VP12_Fid2"}},
	std::array<TileInfo,2>{TileInfo{"CLI","CLI_VP00_Fid1","CLI_VP02_Fid2"},
	 TileInfo{"CSO","CSO_VP30_Fid1","CSO_VP32_Fid2"}} } ;
    return tileinfos[view][tile] ;
  } ;

  void CameraWindow::createMenus()
  {
    auto fileMenu = menuBar()->addMenu(tr("&File"));
    {
      auto action = new QAction(tr("&Quit"), this);
      fileMenu->addAction(action);
      action->setShortcuts(QKeySequence::Quit);
      action->setStatusTip(tr("Quit application"));
      connect(action, &QAction::triggered, []() { QCoreApplication::quit(); } ) ;
    }
    {
      auto action = new QAction(tr("&Load default config"), this);
      fileMenu->addAction(action);
      action->setStatusTip(tr("Load the default configuration file"));
      connect(action, &QAction::triggered, []()
	      {
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment() ;
		QString filename = env.value("HOME") + "/.papconfig" ;
		PAP::PropertySvc::instance()->read(filename.toLatin1()) ;
	      }) ;
    }
    {
      auto action = new QAction(tr("&Save config"), this);
      fileMenu->addAction(action);
      action->setStatusTip(tr("Save a configuration from file"));
      connect(action, &QAction::triggered, []()
	      {
		auto filename = QFileDialog::getSaveFileName(nullptr, tr("Configuration file"),
							     "",
							     tr("Config files (*.txt)"));
		PAP::PropertySvc::instance()->read(filename.toLatin1()) ;
	      } ) ;
    }
    
    
    auto viewMenu = menuBar()->addMenu(tr("&View"));
    {
      auto action = new QAction(tr("&Motion system controls"), this);
      viewMenu->addAction(action);
      action->setStatusTip(tr("Show motion window with motion system controls")) ;
      connect(action, &QAction::triggered, [=](){ m_motionsystemdialog->show() ; } ) ;
    }
    {
      auto action = new QAction(tr("&Autofocus"), this);
      viewMenu->addAction(action);
      action->setStatusTip(tr("Show autofocus window")) ;
      connect(action, &QAction::triggered, [=](){ m_autofocus->show() ; } ) ;
    }
   

    
    
  }
}
