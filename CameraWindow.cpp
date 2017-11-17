#include "CameraWindow.h"
#include "CameraView.h"
#include "MotionSystemSvc.h"
#include "GeometrySvc.h"
#include "AutoFocus.h"
#include "CoordinateMeasurement.h"
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

#include <cmath>
#include "Eigen/Dense"


namespace PAP
{
  // helper class for page for alignment of the main jig

  MarkerRecorderWidget::MarkerRecorderWidget(const char* markername,
					     const PAP::CameraView* camview,
					     QWidget* parent)
    : QWidget(parent), m_status(Uninitialized)
  {
    auto hlayout = new QHBoxLayout{} ;
    setLayout(hlayout) ;
    auto movetomarker1button = new QPushButton{markername, this} ;
    connect(movetomarker1button,&QPushButton::clicked,[=](){ camview->moveCameraTo(markername) ; } ) ;
    hlayout->addWidget( movetomarker1button ) ;
    
    auto recordbutton = new QPushButton{"record", this} ;
    recordbutton->setCheckable(true) ;
    hlayout->addWidget( recordbutton ) ;
    connect(this, &MarkerRecorderWidget::ready, [=]() { recordbutton->setChecked(false) ; setStatus(Ready) ; } ) ;
    connect(recordbutton,&QPushButton::toggled,this,&MarkerRecorderWidget::on_recordbutton_toggled) ;
    // catch measurement updates
    connect( camview, &CameraView::recording, this, &MarkerRecorderWidget::record  ) ;
    m_markerposition = camview->globalPosition( markername ) ;
    // show a label with the status
    m_statuslabel = new QLabel{ this } ;
    setStatus( Uninitialized ) ;
    hlayout->addWidget( m_statuslabel ) ;
    
  }

  void MarkerRecorderWidget::record( CoordinateMeasurement m) {
    if( m_status == Active ) {
      qDebug() << "received measurement: (" << m.globalcoordinates.x << "," << m.globalcoordinates.y << ")" ;
      qDebug() << "marker position:      " << m_markerposition ;
      m_measurement = m ;
      setStatus( Ready ) ;
      emit ready() ;
    }
  }
  
  void MarkerRecorderWidget::setStatus(Status s) {
    m_status = s ;
    if( m_status == Ready )
      m_statuslabel->setText( "Ready" ) ;
    else if( m_status == Active )
      m_statuslabel->setText( "Recording" ) ;
    else
      m_statuslabel->setText( "Uninitialized" ) ;
  }
  
  
  class AlignMainJigPage : public QWidget
  {
  private:
    MarkerRecorderWidget* m_marker1recorder ;
    MarkerRecorderWidget* m_marker2recorder ;
  public:
    AlignMainJigPage(PAP::CameraView* camview)
    {
      auto vlayout = new QVBoxLayout{} ;
      this->setLayout(vlayout) ;
      m_marker1recorder = new MarkerRecorderWidget( "MainJigMarker1", camview ) ;
      vlayout->addWidget( m_marker1recorder ) ;
      m_marker2recorder = new MarkerRecorderWidget( "MainJigMarker2", camview ) ;
      vlayout->addWidget( m_marker2recorder ) ;
      auto calibrationbutton = new QPushButton{"Calibrate", this} ;
      connect(calibrationbutton,&QPushButton::clicked,[=](){ this->updateAlignment() ; } ) ;
      vlayout->addWidget( calibrationbutton ) ;
      auto tmptext1 = new QLabel{this} ;
      tmptext1->setText("a. place the master jig in the machine\n"
			"b. press the 'C' or 'NC' button to identify the side\n"
			"c. press 'zoom master jig marker 1' button to move first jig marker in field of view of camera\n"
			"d. press the record button to record the position of the marker in the view\n"
			"e. press 'zoom master jig marker 2' button to move the second marker in field of view\n"
			"f. press the record button to record the position of the marker in the view\n"
			"g. press the 'calibrate jig coordinate frame' button") ;
      tmptext1->setWordWrap(true);
      vlayout->addWidget( tmptext1 ) ;
    }

  public slots:
    void updateAlignment() const {
      // take the two measurements. then update the relevant
      // parameters (which are Geo.moduleX, moduleY and modulePhi)
      //
      // these parameter essentially make a transform and we need to
      // update that transform.
      //
      // I decided to do everything in the 'global' frame. we then simply compute dx,dy and phi, and finally update the transform:
      if( m_marker1recorder->status() == MarkerRecorderWidget::Ready &&
	  m_marker2recorder->status() == MarkerRecorderWidget::Ready ) {
	std::vector< MarkerRecorderWidget* > recordings = { m_marker1recorder, m_marker2recorder } ;
	Eigen::Vector3d halfdchi2dpar   ;
	Eigen::Matrix3d halfd2chi2dpar2 ;
	for( const auto& r : recordings ) {
	  // the residual is '2D'. however, x and y are
	  // independent. so we could as well compute them separately.
	  auto markerpos = r->markerposition() ;
	  {
	    // first x
	    Eigen::Vector3d deriv ;
	    deriv(0) = 1 ;
	    deriv(1) = 0 ;
	    deriv(2) = - markerpos.y() ;  // -r sin(phi) = -y
	    double residual = r->measurement().globalcoordinates.x - markerpos.x() ;
	    halfdchi2dpar += residual * deriv ;
	    for(int irow=0; irow<3; ++irow)
	      for(int icol=0; icol<3; ++icol)
		halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
	  }
	  {
	    // then y
	    Eigen::Vector3d deriv ;
	    deriv(0) = 0 ;
	    deriv(1) = 1 ;
	    deriv(2) = markerpos.x() ;  // r cos(phi) = x
	    double residual = r->measurement().globalcoordinates.y - markerpos.y() ;
	    halfdchi2dpar += residual * deriv ;
	    for(int irow=0; irow<3; ++irow)
	      for(int icol=0; icol<3; ++icol)
		halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
	  }
	}
	// now solve
	Eigen::Vector3d delta = halfd2chi2dpar2.ldlt().solve(halfdchi2dpar) ;
	qDebug() << "Solution: " << delta(0) << delta(1) << delta(2) ;
	
	// now update the geometry
	GeometrySvc::instance()->applyModuleDelta(delta(0),delta(1),delta(2)) ;
		
	// and don't forget to reset
	m_marker1recorder->reset() ;
	m_marker2recorder->reset() ;
      } else {
	qWarning() << "Recordings not complete: "
		   << m_marker1recorder->status() << " "
		   << m_marker2recorder->status() ;
	// FIXME: pop up a dialog to say that you haven't collected both measurements yet.
      }
    }
  } ;
  // helper class for page for alignment of the tiles
  class AlignTilePage : public QWidget
  {
  public:
    AlignTilePage(PAP::CameraView* camview,
		  const char* marker1, const char* marker2)
    {
      auto vlayout = new QVBoxLayout{} ;
      this->setLayout(vlayout) ;
      auto movetomarker1button = new QPushButton{"Move to marker 1", this} ;
      connect(movetomarker1button,&QPushButton::clicked,[=](){ camview->moveCameraTo(marker1) ; } ) ;
      vlayout->addWidget( movetomarker1button ) ;
      
      auto movetomarker2button = new QPushButton{"Move to marker 2", this} ;
      connect(movetomarker2button,&QPushButton::clicked,[=](){ camview->moveCameraTo(marker2) ; } ) ;
      vlayout->addWidget( movetomarker2button ) ;
    }
  } ;
  
}


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
    layout->addWidget( m_cameraview ) ;
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
