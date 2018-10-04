#include "MetrologyPages.h"
#include "GeometrySvc.h"
#include <QStandardItemModel>
#include <QTableView>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "CameraWindow.h"
#include "CameraView.h"
#include "AutoFocus.h"
#include "MetrologyReport.h"
#include "GraphicsItems.h"
#include <iostream>

namespace PAP
{
  // this needs to get a proper place
  namespace {
    const double microchannelsurfaceZ = 0.25 ;
    const double gluelayerthickness  = 0.1 ;
    const double ascithickness       = 0.2 ;
    const double sensorthickness       = 0.2 ;
    const double bumpbondthickness     = 0.02 ; 
    const double asicZ = microchannelsurfaceZ + gluelayerthickness + ascithickness ;
    const double sensorZ = microchannelsurfaceZ + gluelayerthickness + ascithickness + sensorthickness;
  }
  
  class MetrologyReportPage ;

  // Measure the tilemarkers and sensor markers for one side

  class MarkerMetrologyPage : public QWidget
  {
  private:
    //SideMetrologyReport* m_report ;
    QTableWidget* m_markertable{0} ;
    int m_activerow{-1} ;
    CameraWindow* m_camerasvc{0} ;
  protected:
    std::vector<ReportCoordinate> m_measurements ;
    void createTable() ;
  public:
    MarkerMetrologyPage(CameraWindow& camerasvc) ;
    void updateTableRow( int row, const ReportCoordinate& coord ) ;
    void activateRow( int row ) ;
    void record(const CoordinateMeasurement& measurement) ;
  } ;
  
  MarkerMetrologyPage::MarkerMetrologyPage(CameraWindow& camerasvc)
    : QWidget{&camerasvc}, m_camerasvc{&camerasvc}
  {
    //this->resize(500,300);
    auto layout =  new QVBoxLayout{} ;
    this->setLayout(layout) ;
    
    auto tmptext1 = new QLabel{this} ;
    tmptext1->setText("Page to measure the coordinates of the tile/sensor markers for one side of the module.") ;
    tmptext1->setWordWrap(true);
    layout->addWidget( tmptext1 ) ;
    m_markertable = new QTableWidget{1,4,this} ;
    m_markertable->setHorizontalHeaderLabels(QStringList{"Marker","X","Y","Z"}) ;
    layout->addWidget(m_markertable) ;
    connect(m_markertable,&QTableWidget::cellClicked,[&](int row, int /*column*/) { this->activateRow( row ) ; }) ;
    connect(m_camerasvc->cameraview(),&CameraView::recording,this,&MarkerMetrologyPage::record) ;
  }
  
  void MarkerMetrologyPage::createTable()
  {
    int N = int(m_measurements.size()) ;
    if(N>0) {
      m_markertable->setRowCount(N) ;
      QTableWidgetItem prototype ;
      prototype.setBackground( QBrush{ QColor{Qt::gray} } ) ;
      prototype.setFlags( Qt::ItemIsSelectable ) ;
      int row{0} ;
      for( const auto& m : m_measurements ) {
	for(int icol=0; icol<4; ++icol )  
	  m_markertable->setItem(row,icol,new QTableWidgetItem{prototype} ) ;
	m_markertable->item(row,0)->setText( m.m_name ) ;
	updateTableRow( row++, m ) ;
      }
    }
  }
  
  void MarkerMetrologyPage::updateTableRow( int row, const ReportCoordinate& coord ) {
    m_markertable->item(row,1)->setText( QString::number( coord.m_x, 'g', 5 ) ) ;
    m_markertable->item(row,2)->setText( QString::number( coord.m_y, 'g', 5 ) ) ;
    m_markertable->item(row,3)->setText( QString::number( coord.m_z, 'g', 5 ) ) ;
  }
  
  void MarkerMetrologyPage::activateRow( int row ) {
    // uncolor the current row
    if( row != m_activerow ) {
      if( m_activerow>=0 ) {
	m_markertable->item(m_activerow,0)->
	  setBackground( QBrush{ QColor{m_measurements[m_activerow].m_status == ReportCoordinate::Ready ? Qt::green : Qt::gray} } ) ;
      }
      m_activerow = row ;
      m_markertable->item(row,0)->setBackground( QBrush{ QColor{Qt::yellow} } ) ;
      // now move to the marker position as currently set in the measurement
      auto& m = m_measurements[row] ;
      m_camerasvc->cameraview()->moveCameraToPointInModule( QPointF{m.m_x,m.m_y} ) ;
      if( m.m_z != 0 ) {
	// move the camera to the current focus point, if any
	//m_camerasvc->autofocus()->moveFocusToModuleZ( m.m_z ) ;
	// call the autofocus!
	// m_camerasvc->autofocus()->startNearFocusSequence() ;
      }	
    }
  }
  
  void MarkerMetrologyPage::record(const CoordinateMeasurement& measurement)
  {
    if( m_activerow >=0 && m_activerow < int(m_measurements.size()) ) {
      auto& reportcoordinate = m_measurements[m_activerow] ;
      const auto viewdir = m_camerasvc->cameraview()->currentViewDirection() ;
      const QTransform fromGlobalToModule = GeometrySvc::instance()->fromModuleToGlobal(viewdir).inverted() ;
      const auto modulecoordinates = fromGlobalToModule.map( measurement.globalcoordinates ) ;
      reportcoordinate.m_x = modulecoordinates.x() ;
      reportcoordinate.m_y = modulecoordinates.y() ;
      reportcoordinate.m_z = m_camerasvc->autofocus()->zFromFocus( measurement.mscoordinates.focus ) ;
      reportcoordinate.m_status = ReportCoordinate::Ready ;
      updateTableRow(m_activerow,reportcoordinate) ;
    }
  }

  //****************************************************************************//
  
  class TileMetrologyPage : public MarkerMetrologyPage
  {
  public: 
    TileMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
      : MarkerMetrologyPage(camerasvc)
    {
      // get the geometry service to retrieve predefined positions for all measured points
      const auto geosvc = GeometrySvc::instance() ;
      // we will first do just the tile measurements. there are far
      // too many markers here. make a subselection.
      auto allvelopixmarkers = geosvc->velopixmarkers(viewdir) ;
      
      for( const auto& def: allvelopixmarkers )
	if( def.name.contains("2_Fid2") || def.name.contains("0_Fid1") )
	  m_measurements.emplace_back( def, asicZ ) ;
	else if( def.name.contains("Sensor") )
	  m_measurements.emplace_back( def, - (asicZ +bumpbondthickness) ) ;
      createTable() ;
    }
  } ;
    
  QWidget* createTileMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
  {
    return new TileMetrologyPage{camerasvc,viewdir} ;
  }

  namespace {
    void collectReferenceMarkers(const QGraphicsItem& item,
				 std::vector<const PAP::ReferenceMarker*>& markers)
    {
      auto m = dynamic_cast<const PAP::ReferenceMarker*>(&item) ;
      if( m ) {
	markers.push_back(m) ;
      } else {
	for( const auto& dau : item.childItems() ) 
	  collectReferenceMarkers(*dau,markers) ;
      }
    }
  }

  //****************************************************************************//
  
  class SensorSurfaceMetrologyPage : public MarkerMetrologyPage
  {
  public:
    SensorSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
      : MarkerMetrologyPage(camerasvc)
    {
      // now I need to think, because I created these 'reference'
      // position only inside the displayed geometry. there is no
      // concept of a tile anywhere else. perhaps we can extract them
      // from the full list, then filter on view by marker name?
      std::vector<const PAP::ReferenceMarker*> markers ;
      const auto& camview = camerasvc.cameraview() ;
      collectReferenceMarkers( *(viewdir == ViewDirection::NSideView ? camview->nsidemarkers() : camview->csidemarkers()),
			       markers) ;
      for( const auto& m: markers )
	if(m->toolTip().contains("Sensor"))
	   m_measurements.emplace_back( m->toolTip(), m->pos().x(), m->pos().y(), sensorZ ) ;
      createTable() ;
    }
  public:
  } ;

    QWidget* createSensorSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
  {
    return new SensorSurfaceMetrologyPage{camerasvc,viewdir} ;
  }

  //****************************************************************************//

  class SubstrateSurfaceMetrologyPage : public MarkerMetrologyPage
  {
  public:
    SubstrateSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
      : MarkerMetrologyPage(camerasvc)
    {
      const auto geosvc = GeometrySvc::instance() ;
      // we will first do just the tile measurements. there are far
      // too many markers here. make a subselection.
      auto markers = geosvc->substratemarkers(viewdir) ;
      for( const auto& def: markers )
	m_measurements.emplace_back( def, microchannelsurfaceZ ) ;
      createTable() ;
    }
  public:
  } ;

    QWidget* createSubstrateSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
  {
    return new SubstrateSurfaceMetrologyPage{camerasvc,viewdir} ;
  }


  
  
  // class MetrologyReportPage : public QDialog
  // {
  // public:
  //   Q_OBJECT
  // private:
  //   //CameraWindow* m_parent ;
  //   QStandardItem m_modulename ;
  //   TileMetrologyPage* m_reports[2] ; // for both sides
  // public:
  // MetrologyReportPage( CameraWindow& parent )
  //   : QDialog{&parent}, m_modulename{QString{"Unknown"}}
  //   {
  //     this->resize(500,500);
  //     this->move(50,500) ;
  //     auto layout =  new QVBoxLayout{} ;
  //     this->setLayout(layout) ;
  //     layout->setContentsMargins(0, 0, 0, 0);

  //     auto tmptext1 = new QLabel{this} ;
  //     tmptext1->setText("Add field to enter module name") ;
  //     tmptext1->setWordWrap(true);
  //     layout->addWidget( tmptext1 ) ;
      
  //     // need a text field where we can change the name
  //     /*{
  // 	auto namemodel = new QStandardItemModel{1,1,this} ;
  // 	namemodel->setItem(0,0,&m_modulename) ;
  // 	auto nameview = new QTableView{this} ;
  // 	nameview->SetModel(namemodel) ;
  // 	nameview->setHeaderData(0,Qt::Horizontal,QString{"Module name"}) ;
  // 	layout->addWidget( nameview ) ;
  //     }
  //     */
      
  //     // add pages for N side and C side
  //     auto taskpages = new QTabWidget{this} ;
  //     layout->addWidget( taskpages ) ;
  //     for(int i=0; i<2; ++i) {
  // 	m_reports[i] = new TileMetrologyPage{parent,ViewDirection(i)} ;
  // 	taskpages->addTab( m_reports[i], i==ViewDirection::NSideView?"N-side":"C-side") ;
  //     }

  //     //layout->addWidget( new TileMetrologyPage{ViewDirection(0),this} ) ;
  //     //connect( taskpages, &QTabWidget::tabBarClicked, this, &CameraWindow::toggleView ) ;
  //   }
  // } ;

 

  /* class MetrologyReportWindow : public QMainWindow */
  /* { */
    
  /*   Q_OBJECT */
  /* public: */
  /*   MetrologyReportWindow(QWidget *parent = 0) */
  /*     : QMainWindow(parent) */
  /*   { */
  /*     resize(500,500); */
  /*     setWindowTitle("Measurement report") ; */
      
      
  /*   } ; */
  /* } ; */
}
