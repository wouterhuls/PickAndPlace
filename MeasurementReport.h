#include "GeometrySvc.h"
#include <QStandardItemModel>
#include <QTableView>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "CameraWindow.h"
#include "CameraView.h"
#include "AutoFocus.h"
#include <iostream>

namespace PAP
{
  class MeasurementReportPage ;
  
  // FIXME: we already have a class
  // 'CoordinateMeasurement'. cannot we merge those? or at least make
  // the names a bit more descriptive?
  class ReportCoordinate
  {
  public:
    enum Status { Uninitialized, Ready } ;
  public:
    QString m_name ;
    // These coordinates in the GLOBAL LHCb frame? That's most logical
    // but perhaps not the most convenient?
    double m_x ;
    double m_y ;
    double m_z ;
    double m_focusMeasure ;
    Status m_status ;
  public:
  ReportCoordinate( const FiducialDefinition& def, double z )
      : m_name{def.name}, m_x{def.x}, m_y{def.y}, m_z{z}, m_focusMeasure{0}, m_status{Uninitialized} {}
  } ;

  class SideMeasurementReportPage : public QWidget
  {
  private:
    std::vector<ReportCoordinate> m_measurements ;
    QTableWidget* m_markertable{0} ;
    int m_activerow{-1} ;
    CameraWindow* m_camerasvc{0} ;

  public:
    Q_OBJECT
  public:
    SideMeasurementReportPage(CameraWindow& camerasvc, ViewDirection viewdir)
      : QWidget{}, m_camerasvc{&camerasvc}
    {
      this->resize(500,300);
      auto layout =  new QVBoxLayout{} ;
      this->setLayout(layout) ;
      
      auto tmptext1 = new QLabel{this} ;
      tmptext1->setText("Create three tables that contain display various measurements, namely\n"
			"1. tilemarkers (4 per side)\n"
			"2. sensormarkers (1 per side?)\n"
			"3. z position of sensor near corners\n"
			"4. various surface measurements on the substrate") ;
      tmptext1->setWordWrap(true);
      layout->addWidget( tmptext1 ) ;
      
      // get the geometry service to retrieve predefined positions for all measured points
      const auto geosvc = GeometrySvc::instance() ;
      // we will first do just the tile measurements
      auto velopixmarkers = geosvc->velopixmarkers(viewdir) ;
      const double microchannelsurface = 0.25 ;
      const double gluelayerthickness  = 0.1 ;
      const double ascithickness       = 0.2 ;
      const double asicZ = microchannelsurface + gluelayerthickness + ascithickness ;
      std::transform(std::begin(velopixmarkers),std::end(velopixmarkers),
		     std::back_inserter(m_measurements),
		     [=](const FiducialDefinition& def) {
		       return ReportCoordinate{def,asicZ} ; } ) ;
      
      // make a table to view the measurements. we should do this with
      // the model/view thing, but I find that a bit too complicated.
      {
	int N = int(m_measurements.size()) ;
	m_markertable = new QTableWidget{N,4,this} ;
	m_markertable->setHorizontalHeaderLabels(QStringList{"Marker","X","Y","Z"}) ;
	layout->addWidget(m_markertable) ;
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
	connect(m_markertable,&QTableWidget::cellClicked,[&](int row, int /*column*/) { this->activateRow( row ) ; }) ;
	connect(m_camerasvc->cameraview(),&CameraView::recording,this,&SideMeasurementReportPage::record) ;
      }
    }
    void updateTableRow( int row, const ReportCoordinate& coord ) {
      m_markertable->item(row,1)->setText( QString::number( coord.m_x, 'g', 5 ) ) ;
      m_markertable->item(row,2)->setText( QString::number( coord.m_y, 'g', 5 ) ) ;
      m_markertable->item(row,3)->setText( QString::number( coord.m_z, 'g', 5 ) ) ;
    }
    void activateRow( int row ) {
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
    void record(const CoordinateMeasurement& measurement)
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
  } ;
  
  class MeasurementReportPage : public QDialog
  {
  public:
    Q_OBJECT
  private:
    //CameraWindow* m_parent ;
    QStandardItem m_modulename ;
    SideMeasurementReportPage* m_reports[2] ; // for both sides
  public:
  MeasurementReportPage( CameraWindow& parent )
    : QDialog{&parent}, m_modulename{QString{"Unknown"}}
    {
      this->resize(500,500);
      this->move(50,500) ;
      auto layout =  new QVBoxLayout{} ;
      this->setLayout(layout) ;
      layout->setContentsMargins(0, 0, 0, 0);

      auto tmptext1 = new QLabel{this} ;
      tmptext1->setText("Add field to enter module name") ;
      tmptext1->setWordWrap(true);
      layout->addWidget( tmptext1 ) ;
      
      // need a text field where we can change the name
      /*{
	auto namemodel = new QStandardItemModel{1,1,this} ;
	namemodel->setItem(0,0,&m_modulename) ;
	auto nameview = new QTableView{this} ;
	nameview->SetModel(namemodel) ;
	nameview->setHeaderData(0,Qt::Horizontal,QString{"Module name"}) ;
	layout->addWidget( nameview ) ;
      }
      */
      
      // add pages for N side and C side
      auto taskpages = new QTabWidget{this} ;
      layout->addWidget( taskpages ) ;
      for(int i=0; i<2; ++i) {
	m_reports[i] = new SideMeasurementReportPage{parent,ViewDirection(i)} ;
	taskpages->addTab( m_reports[i], i==ViewDirection::NSideView?"N-side":"C-side") ;
      }

      //layout->addWidget( new SideMeasurementReportPage{ViewDirection(0),this} ) ;
      //connect( taskpages, &QTabWidget::tabBarClicked, this, &CameraWindow::toggleView ) ;
    }
  } ;

 

  /* class MeasurementReportWindow : public QMainWindow */
  /* { */
    
  /*   Q_OBJECT */
  /* public: */
  /*   MeasurementReportWindow(QWidget *parent = 0) */
  /*     : QMainWindow(parent) */
  /*   { */
  /*     resize(500,500); */
  /*     setWindowTitle("Measurement report") ; */
      
      
  /*   } ; */
  /* } ; */
}
