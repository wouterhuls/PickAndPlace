#include "MetrologyPages.h"
#include "GeometrySvc.h"
#include <QStandardItemModel>
#include <QTableView>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QFileDialog>
#include "CameraWindow.h"
#include "CameraView.h"
#include "AutoFocus.h"
#include "MetrologyReport.h"
#include "GraphicsItems.h"
#include "TextEditStream.h"
#include <iostream>
#include <Eigen/Dense>

namespace Eigen {
  using Vector6d = Eigen::Matrix<double,6,1> ;
  using Matrix6d = Eigen::Matrix<double,6,6> ;
}

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
  protected:
    //SideMetrologyReport* m_report ;
    QTableWidget* m_markertable{0} ;
    int m_activerow{-1} ;
    CameraWindow* m_camerasvc{0} ;
    ViewDirection m_viewdir{ViewDirection::NumViews} ;
    QHBoxLayout* m_buttonlayout{0} ;
    QPlainTextEdit* m_textbox{0} ;
  protected:
    std::vector<ReportCoordinate> m_measurements ;
    void createTable() ;
    virtual void definemarkers() = 0 ;
    CameraWindow* camerasvc() { return m_camerasvc ; } ;
  public:
    MarkerMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
    void updateTableRow( int row, const ReportCoordinate& coord ) ;
    void activateRow( int row ) ;
    void record(const CoordinateMeasurement& measurement) ;
    ViewDirection viewdir() const { return m_viewdir ; }
    void exportToFile() const ;
  } ;
  
  MarkerMetrologyPage::MarkerMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
    : QWidget{&camerasvc}, m_camerasvc{&camerasvc}, m_viewdir{viewdir}
  {
    //this->resize(500,300);
    auto layout =  new QVBoxLayout{} ;
    this->setLayout(layout) ;
    
    //auto tmptext1 = new QLabel{this} ;
    //tmptext1->setText("Page to measure the coordinates of the tile/sensor markers for one side of the module.") ;
    //tmptext1->setWordWrap(true);
    //layout->addWidget( tmptext1 ) ;

    auto hlayout = new QHBoxLayout{} ;
    layout->addLayout(hlayout) ;
    m_markertable = new QTableWidget{1,4,this} ;
    m_markertable->resize(500,150) ;
    //m_markertable->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding) ;
    m_markertable->setHorizontalHeaderLabels(QStringList{"Marker","X","Y","Z","residual Z"}) ;
    hlayout->addWidget(m_markertable) ;
    connect(m_markertable,&QTableWidget::cellClicked,[&](int row, int /*column*/) { this->activateRow( row ) ; }) ;
    connect(m_camerasvc->cameraview(),&CameraView::recording,this,&MarkerMetrologyPage::record) ;

    m_textbox = new QPlainTextEdit{this} ;
    hlayout->addWidget( m_textbox ) ;
    m_textbox->resize(300,150) ;
    
    m_buttonlayout = new QHBoxLayout{} ;
    layout->addLayout(m_buttonlayout) ;
    {
      auto button = new QPushButton{"Reset",this} ;
      m_buttonlayout->addWidget(button) ;
      connect(button,&QPushButton::clicked,this,[=]{ this->definemarkers() ; }) ;
    }
    {
      auto button = new QPushButton{"Export",this} ;
      m_buttonlayout->addWidget(button) ;
      connect(button,&QPushButton::clicked,this,[=]{ this->exportToFile() ; }) ;
    }
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
	for(int icol=0; icol<m_markertable->columnCount(); ++icol )  
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
	  setBackground( QBrush{ QColor{m_measurements[m_activerow].m_status == ReportCoordinate::Status::Ready ? Qt::green : Qt::gray} } ) ;
      }
      m_activerow = row ;
      auto& m = m_measurements[row] ;
      m_markertable->item(row,0)->setBackground( QBrush{ QColor{m.m_status ==  ReportCoordinate::Status::Ready ? Qt::blue : Qt::yellow} } ) ;
      // now move to the marker position as currently set in the measurement
      if( int(m.m_status) >= int(ReportCoordinate::Status::Initialized) ) {
	m_camerasvc->cameraview()->moveCameraToPointInModule( QPointF{m.m_x,m.m_y} ) ;
	if( m.m_status == ReportCoordinate::Status::Ready ) {
	  m_camerasvc->autofocus()->moveFocusToModuleZ( m.m_z ) ;
	  // move the camera to the current focus point, if any
	  //m_camerasvc->autofocus()->moveFocusToModuleZ( m.m_z ) ;
	  // call the autofocus!
	  // m_camerasvc->autofocus()->startNearFocusSequence() ;
	}	
      }
    }
  }
  
  void MarkerMetrologyPage::record(const CoordinateMeasurement& measurement)
  {
    if( m_activerow >= 0 ) {
      // initialize enough measurements, if possible
      m_measurements.resize( m_activerow+1 ) ;
      // now store the measureent
      auto& reportcoordinate = m_measurements[m_activerow] ;
      const auto viewdir = m_camerasvc->cameraview()->currentViewDirection() ;
      const QTransform fromGlobalToModule = GeometrySvc::instance()->fromModuleToGlobal(viewdir).inverted() ;
      const auto modulecoordinates = fromGlobalToModule.map( measurement.globalcoordinates ) ;
      reportcoordinate.m_x = modulecoordinates.x() ;
      reportcoordinate.m_y = modulecoordinates.y() ;
      reportcoordinate.m_z = m_camerasvc->autofocus()->zFromFocus( measurement.mscoordinates.focus ) ;
      reportcoordinate.m_status = ReportCoordinate::Ready ;
      // finally update the table
      updateTableRow(m_activerow,reportcoordinate) ;
    }
  }

  void MarkerMetrologyPage::exportToFile() const
  {
    // pop up a dialog to get a file name
    auto filename = QFileDialog::getSaveFileName(nullptr, tr("Save data"),
						 QString("/home/velouser/Documents/PickAndPlaceData/") +
						 m_camerasvc->moduleName() + "/untitled.txt",
						 tr("Text files (*.txt)"));
    QFile f( filename );
    QTextStream data( &f );
    // first the header
    {
      QStringList strList;
      for( int c = 0; c < m_markertable->columnCount(); ++c ) {
	strList <<  "\" " +
	  m_markertable->horizontalHeaderItem(c)->data(Qt::DisplayRole).toString() +
	  "\" ";
      }
      data << strList.join(";") << "\n";
    }
    // now the data
    for(int irow=0; irow<= m_markertable->rowCount() && irow<int(m_measurements.size()); ++irow ) {
      QStringList strList;
      if( m_measurements[irow].m_status == ReportCoordinate::Ready ) 
	for( int icol=0; icol < m_markertable->columnCount(); ++icol ) 
	  strList << m_markertable->item(irow,icol)->text() ;
      data << strList.join(";") << "\n";
    }
    f.close() ;
  }
  
  struct PlaneFitResult
  {
    enum Parameters{Z0,dZdX,dZdY,d2ZdX2,d2ZdXY,d2ZdY2} ;
    Eigen::Vector6d parameters{Eigen::Vector6d::Zero()} ;
    Eigen::Matrix6d covariance{Eigen::Matrix6d::Zero()} ;
    double x0{0} ;
    double y0{0} ;
  } ;
  
  class SurfaceMetrologyPage : public MarkerMetrologyPage
  {
  public:
    enum FitModel { Plane, CurvedPlane} ;
    SurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
      : MarkerMetrologyPage{camerasvc,viewdir}
    {
      {
	auto button = new QPushButton{"Fit plane", this} ;
	m_buttonlayout->addWidget(button) ;
	connect(button,&QPushButton::clicked,this,[=]{ this->fitPlane(FitModel::Plane) ; }) ;
      }
      {
	auto button = new QPushButton{"Fit curved plane", this} ;
	m_buttonlayout->addWidget(button) ;
	connect(button,&QPushButton::clicked,this,[=]{ this->fitPlane(FitModel::CurvedPlane) ; }) ;
      }
    }
    PlaneFitResult fitPlane(FitModel mode = FitModel::Plane, double originx=0, double originy=0) ;
  private:
    // fit result. is there a better place to keep this? do we need to keep it at all?
    
  } ;  

  PlaneFitResult SurfaceMetrologyPage::fitPlane(FitModel mode, double originx, double originy)
  {
    Eigen::Vector6d halfdchi2dpar   = Eigen::Vector6d::Zero() ;
    Eigen::Matrix6d halfd2chi2dpar2 = Eigen::Matrix6d::Zero() ;
    size_t numvalid{0} ;
    for( const auto& m : m_measurements )
      if( m.m_status == ReportCoordinate::Status::Ready ) {
	++numvalid ;
	Eigen::Vector6d deriv ;
	double dx = m.m_x-originx ;
	double dy = m.m_y-originy ;
	deriv(0) = 1 ;
	deriv(1) = dx ;
	deriv(2) = dy ;
	deriv(3) = dx*dx ;
	deriv(4) = dx*dy ;
	deriv(5) = dy*dy ;
	halfdchi2dpar += m.m_z * deriv ;
	for(int irow=0; irow<6; ++irow)
	  for(int icol=0; icol<6; ++icol)
	    halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
      }
    // compute solution. switch between different polynomial orders here.
    Eigen::Vector6d delta = Eigen::Vector6d::Zero() ;
    std::stringstream os ;
    int ndof{0} ;
    switch(mode) {
    case CurvedPlane:
      if(numvalid>5) {
	delta = halfd2chi2dpar2.ldlt().solve(halfdchi2dpar) ;
	ndof = numvalid - 6 ;
      } else {
	os << "Not enough measurements to fit curved plane: " << numvalid << '\n' ;
      }
      break;
    default:
      if(numvalid>2) {
	delta.block<3,1>(0,0) = halfd2chi2dpar2.block<3,3>(0,0).ldlt().solve( halfdchi2dpar.block<3,1>(0,0) ) ;
	ndof = numvalid - 3 ;
      } else {
	os << "Not enough measurements to fit plane: " << numvalid << '\n' ;
      }
    }
    
    // fill the column with residuals
    m_markertable->setColumnCount(5) ;
    m_markertable->setHorizontalHeaderItem(4,new QTableWidgetItem{"residual Z"}) ;
    int row(0) ;
    double sumr2{0} ;
    for( const auto& m : m_measurements ) {
      double dx = m.m_x-originx ;
      double dy = m.m_y-originy ;
      double residual = m.m_z - (delta(0) + delta(1)*dx + delta(2)*dy + delta(3)*dx*dx + delta(4)*dx*dy+ delta(5)*dy*dy ) ;
      if( !m_markertable->item(row,4) )
	m_markertable->setItem(row,4,new QTableWidgetItem{}) ;
      m_markertable->item(row,4)->setText( QString::number( residual, 'g', 5 ) ) ;
      ++row ;
      if(  m.m_status == ReportCoordinate::Status::Ready ) {
	sumr2 += residual ;
      }
    }
    const double sigmaz2 = 0.005*0.005 ;
    if(ndof>0) {
      os << "Fit result: " ;
      for(int i=0; i<6; ++i) os << delta(i) << " " ;
      os << '\n' ;
      os << "chi2/dof: " << sumr2/sigmaz2 << " / " << ndof << std::endl ;
    }
    m_textbox->appendPlainText( os.str().c_str() ) ;
    PlaneFitResult rc ;
    rc.x0 = originx ;
    rc.y0 = originy ;
    rc.parameters = delta ;
       
    return rc ;
  }
  
  //****************************************************************************//
  
  class TileMetrologyPage : public MarkerMetrologyPage
  {
  public: 
    TileMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
      : MarkerMetrologyPage{camerasvc, viewdir}
    {
      definemarkers() ;
    }
    void definemarkers() final {
      m_measurements.clear() ;
      const auto geosvc = GeometrySvc::instance() ;
      // we will first do just the tile measurements. there are far
      // too many markers here. make a subselection.
      auto allvelopixmarkers = geosvc->velopixmarkers(viewdir()) ;
      
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

  //****************************************************************************//
  
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

  class SensorSurfaceMetrologyPage : public SurfaceMetrologyPage
  {
  public:
    SensorSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
      : SurfaceMetrologyPage{camerasvc,viewdir}
    {
      definemarkers() ;
    }
    void definemarkers() final
    {
      m_measurements.clear() ;
      // now I need to think, because I created these 'reference'
      // position only inside the displayed geometry. there is no
      // concept of a tile anywhere else. perhaps we can extract them
      // from the full list, then filter on view by marker name?
      std::vector<const PAP::ReferenceMarker*> markers ;
      const auto& camview = camerasvc()->cameraview() ;
      collectReferenceMarkers( *(viewdir() == ViewDirection::NSideView ? camview->nsidemarkers() : camview->csidemarkers()),
			       markers) ;
      for( const auto& m: markers )
	if(m->toolTip().contains("Sensor"))
	   m_measurements.emplace_back( m->toolTip(), m->pos().x(), m->pos().y(), sensorZ ) ;
      createTable() ;
    }
  } ;
 
  QWidget* createSensorSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
  {
    return new SensorSurfaceMetrologyPage{camerasvc,viewdir} ;
  }

  //****************************************************************************//

  class SubstrateSurfaceMetrologyPage : public SurfaceMetrologyPage
  {
  public:
    SubstrateSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
      : SurfaceMetrologyPage{camerasvc,viewdir}
    {
      definemarkers() ;
    }
    void definemarkers() final
    {
      m_measurements.clear() ;
      const auto geosvc = GeometrySvc::instance() ;
      // we will first do just the tile measurements. there are far
      // too many markers here. make a subselection.
      auto markers = geosvc->substratemarkers(viewdir()) ;
      for( const auto& def: markers )
	m_measurements.emplace_back( def, microchannelsurfaceZ ) ;
      createTable() ;
    }
  } ;

  QWidget* createSubstrateSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
  {
    return new SubstrateSurfaceMetrologyPage{camerasvc,viewdir} ;
  }

  //****************************************************************************//
  
  class GenericSurfaceMetrologyPage : public SurfaceMetrologyPage
  {
  public:
    GenericSurfaceMetrologyPage( CameraWindow& camerasvc, ViewDirection viewdir)
      : SurfaceMetrologyPage{camerasvc,viewdir}
    {
      definemarkers() ;
    }
    void definemarkers() final
    {
      m_measurements.clear() ;
      m_measurements.resize(16) ;
      createTable() ;
    }
  } ;

  QWidget* createGenericSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
  {
    return new GenericSurfaceMetrologyPage{camerasvc,viewdir} ;
  }
  
  /*
    I have the pages that do most of the actual work. Now we need
    todecide what is the best way to store the data, and keep track
    of the fact that all measurements need to be done before the
    actual report is exported.
    
    What is the minimum that we want in the report:
    - x, y coordinates of tile/sensor markers in LHCb frame
    - estimates of distance between sensor surface and substrate surface

    Other relevant information would be:
    - curvature of substrate ?
    - curvature of sensors?


  */

    
  
  
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
