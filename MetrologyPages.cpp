#include "MetrologyPages.h"
#include "GeometrySvc.h"
#include "MotionSystemSvc.h"
#include <QStandardItemModel>
#include <QTableView>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QCheckBox>
#include "CameraWindow.h"
#include "CameraView.h"
#include "AutoFocus.h"
#include "MetrologyReport.h"
#include "GraphicsItems.h"
#include "TextEditStream.h"
#include "NamedValueInputWidget.h"
#include <iostream>
#include <Eigen/Dense>

// Let's implement the list of measurements as a qabstracttablemodel
// http://doc.qt.io/qt-5/qabstracttablemodel.html#details
// https://stackoverflow.com/questions/11906324/binding-model-to-qt-tableview

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
    void fillTable() ;
    virtual void definemarkers() = 0 ;
    CameraWindow* camerasvc() { return m_camerasvc ; } ;
    virtual QString pageName() const { return QString{"unknown"} ; }
    void addMarker() ;
  public:
    MarkerMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
    void updateTableRow( int row, const ReportCoordinate& coord ) ;
    void activateRow( int row ) ;
    void record(const CoordinateMeasurement& measurement) ;
    void recordCentre() ;
    //void record() {}
    ViewDirection viewdir() const { return m_viewdir ; }
    void exportToFile() const ;
    void importFromFile() ;
    void importFromFile(const QString& filename) ;
    void reset() {
      m_textbox->setPlainText("") ;
      definemarkers();
    }
    QString defaultFileName() const {
      return QString("/home/velouser/Documents/PickAndPlaceData/") +
	m_camerasvc->moduleName() + "/" + pageName() + ".csv" ;
    }
    virtual void focus() const ;
    void move() const ;
    const std::vector<ReportCoordinate>& measurements() const { return  m_measurements ; }
    void acceptall() { for(auto&m:m_measurements) m.setStatus( ReportCoordinate::Ready ) ; }
    void resetall() { for(auto&m:m_measurements) m.setStatus( ReportCoordinate::Initialized ) ; }
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
    //connect(m_camerasvc->cameraview(),&CameraView::recording,this,&MarkerMetrologyPage::record) ;

    m_textbox = new QPlainTextEdit{this} ;
    hlayout->addWidget( m_textbox ) ;
    m_textbox->resize(300,150) ;
    
    m_buttonlayout = new QHBoxLayout{} ;
    layout->addLayout(m_buttonlayout) ;
    {
      auto button = new QPushButton{"Focus",this} ;
      m_buttonlayout->addWidget(button) ;
      connect(button,&QPushButton::clicked,this,[=]{ this->focus() ; }) ;
    }   
    {
      auto button = new QPushButton{"Record",this} ;
      m_buttonlayout->addWidget(button) ;
      connect(button,&QPushButton::clicked,this,[=]{ this->recordCentre() ; }) ;
    }   
    {
      auto button = new QPushButton{"Add",this} ;
      m_buttonlayout->addWidget(button) ;
      connect(button,&QPushButton::clicked,this,[=]{ this->addMarker() ; }) ;
    }   
   {
      auto button = new QPushButton{"Export",this} ;
      m_buttonlayout->addWidget(button) ;
      connect(button,&QPushButton::clicked,this,[=]{ this->exportToFile() ; }) ;
    }
    {
      auto button = new QPushButton{"Import",this} ;
      m_buttonlayout->addWidget(button) ;
      connect(button,&QPushButton::clicked,this,[=]{ this->importFromFile() ; }) ;
    }
    {
      auto button = new QPushButton{"Reset",this} ;
      m_buttonlayout->addWidget(button) ;
      connect(button,&QPushButton::clicked,this,[=]{ this->reset() ; }) ;
    }
 }
  
  void MarkerMetrologyPage::focus() const
  {
    if( !MotionSystemSvc::instance()->mainXAxis().isMoving() &&
	!MotionSystemSvc::instance()->mainYAxis().isMoving() ) {
      const double currentfocus = m_camerasvc->autofocus()->currentFocus() ;
      m_camerasvc->autofocus()->startFocusSequence(currentfocus-0.3,currentfocus+0.3) ;
    } ;
  }
  
  void MarkerMetrologyPage::fillTable()
  {
    int N = int(m_measurements.size()) ;
    if(N>=0) {
      m_markertable->setRowCount(N) ;
      QTableWidgetItem prototype ;
      prototype.setBackground( QBrush{ QColor{Qt::white} } ) ;
      prototype.setFlags( Qt::ItemIsSelectable ) ;
      int row{0} ;
      for( const auto& m : m_measurements ) {
	for(int icol=0; icol<m_markertable->columnCount(); ++icol )  
	  m_markertable->setItem(row,icol,new QTableWidgetItem{prototype} ) ;
	m_markertable->item(row,0)->setText( m.name() ) ;
	updateTableRow( row++, m ) ;
      }
    } 
  }

  void MarkerMetrologyPage::addMarker()
  {
    // popup a window to ask for a name
    bool ok{true} ;
    QString label = QInputDialog::getText(this,
					      tr("QInputDialog::getText()"),
					      tr("Measurement label:"), QLineEdit::Normal, "Unknown", &ok);
    if (ok && !label.isEmpty()) {
      auto measurement = m_camerasvc->cameraview()->coordinateMeasurement() ;
      const auto viewdir = m_camerasvc->cameraview()->currentViewDirection() ;
      const QTransform fromGlobalToModule = GeometrySvc::instance()->fromModuleToGlobal(viewdir).inverted() ;
      const auto modulecoordinates = fromGlobalToModule.map( measurement.globalcoordinates ) ;
      const auto z = m_camerasvc->autofocus()->zFromFocus( measurement.mscoordinates.focus ) ;
      m_measurements.emplace_back( label, modulecoordinates.x(), modulecoordinates.y(),
				   z, ReportCoordinate::Initialized) ;
      qDebug() << "Measurement test: "
	       << measurement.modulecoordinates
	       << m_measurements.back().position() ;
      fillTable() ;
      activateRow( m_measurements.size()-1 ) ;
    }
  }  
  
  void MarkerMetrologyPage::updateTableRow( int row, const ReportCoordinate& coord ) {
    m_markertable->item(row,1)->setText( QString::number( coord.x(), 'g', 5 ) ) ;
    m_markertable->item(row,2)->setText( QString::number( coord.y(), 'g', 5 ) ) ;
    m_markertable->item(row,3)->setText( QString::number( coord.z(), 'g', 5 ) ) ;
    const auto color = m_measurements[row].status() == ReportCoordinate::Status::Ready ? Qt::green : Qt::lightGray ;
    m_markertable->item(row,0)->setBackground( QBrush{ QColor{color} } ) ;
  }
  
  void MarkerMetrologyPage::activateRow( int row ) {

    if( row>=0 && row<=int(m_measurements.size()) ) {
      const auto& m = m_measurements[row] ;
      
      if( row != m_activerow ) {
	// uncolor the current row
	if( m_activerow>=0 && m_activerow<int(m_measurements.size()) ) {
	  m_markertable->item(m_activerow,0)->
	    setBackground( QBrush{ QColor{m_measurements[m_activerow].status() == ReportCoordinate::Status::Ready ? Qt::green : Qt::lightGray} } ) ;
	  //m_markertable->item(m_activerow,0)->setSelected(false) ;
	}
	// move the camera
	if( int(m.status()) >= int(ReportCoordinate::Status::Initialized) ) {
	  m_camerasvc->cameraview()->moveCameraToPointInModule( QPointF{m.x(),m.y()} ) ;
	  if( m.status() == ReportCoordinate::Status::Ready ) {
	    qDebug() << "Moving camera to z: " << m.z() ;
	    m_camerasvc->autofocus()->moveFocusToModuleZ( m.z() ) ;
	    // move the camera to the current focus point, if any
	    //m_camerasvc->autofocus()->moveFocusToModuleZ( m.m_z ) ;
	    // call the autofocus!
	    // m_camerasvc->autofocus()->startNearFocusSequence() ;
	  }	
	}
      }

      // recolor the new row
      m_markertable->item(row,0)->setBackground( QBrush{ QColor{m.status() ==  ReportCoordinate::Status::Ready ? Qt::darkGreen : Qt::gray} } ) ;
      //m_markertable->item(row,0)->setSelected(true) ;
      // make sure to set the active row flag
      m_activerow = row ;
    }
  }
  
  void MarkerMetrologyPage::move() const
  {
    if( m_activerow < int(m_measurements.size()) ) {
      const auto& m = m_measurements[m_activerow] ;
      m_camerasvc->cameraview()->moveCameraToPointInModule( QPointF{m.x(),m.y()} ) ;
      m_markertable->item(m_activerow,0)->setBackground( QBrush{ QColor{m.status() ==  ReportCoordinate::Status::Ready ? Qt::blue : Qt::yellow} } ) ;
    }
  }
  
  void MarkerMetrologyPage::record(const CoordinateMeasurement& measurement)
  {
    if( m_activerow >= 0 ) {
      // initialize enough measurements, if possible needed
      if( m_activerow >= int(m_measurements.size()) )
	m_measurements.resize( m_activerow+1 ) ;
      // now store the measurement
      auto& reportcoordinate = m_measurements[m_activerow] ;
      reportcoordinate = measurement.modulecoordinates ;
      reportcoordinate.setStatus( ReportCoordinate::Ready ) ;
      // finally update the table
      updateTableRow(m_activerow,reportcoordinate) ;
      // Let's also export it to the text stream, such that we can easily copy paste it.
      std::stringstream os ;
      os << reportcoordinate.name().toStdString() << ": " << reportcoordinate.x() << ", " << reportcoordinate.y() << ", "
	 << reportcoordinate.z()  ;
      m_textbox->appendPlainText( os.str().c_str() ) ;
    }
  }
  
  void MarkerMetrologyPage::recordCentre()
  {
    return record(m_camerasvc->cameraview()->coordinateMeasurement()) ;
  }
    
  void MarkerMetrologyPage::exportToFile() const
  {
    // pop up a dialog to get a file name
    auto filename = QFileDialog::getSaveFileName(nullptr, tr("Save data"),
						 defaultFileName(),
						 tr("Csv files (*.csv)"));
    QFile f( filename );
    f.open(QIODevice::WriteOnly) ;
    QTextStream data( &f );
    // first the header. those we get from the table.
    {
      QStringList strList;
      for( int c = 0; c < m_markertable->columnCount(); ++c ) {
	strList <<  "\" " +
	  m_markertable->horizontalHeaderItem(c)->data(Qt::DisplayRole).toString() +
	  "\" ";
      }
      data << strList.join(";") << "\n";
    }
    // now the data. why would we take it from the table? only to add the residuals?
    for(int irow=0; irow<= m_markertable->rowCount() && irow<int(m_measurements.size()); ++irow ) {
      if( m_measurements[irow].status() == ReportCoordinate::Ready ) {
	QStringList strList;
	for( int icol=0; icol < m_markertable->columnCount(); ++icol ) 
	  strList << m_markertable->item(irow,icol)->text() ;
	data << strList.join(";") << "\n";
      }
    }
    f.close() ;
    f.setPermissions( QFileDevice::ReadOther|QFileDevice::ReadGroup|QFileDevice::ReadOwner ) ;
  }

  void MarkerMetrologyPage::importFromFile()
  {
    
    // pop up a dialog to get a file name
    auto filename = QFileDialog::getOpenFileName(nullptr, tr("Save data"),
						 defaultFileName(),
						 tr("CSV files (*.csv)"));
    
    importFromFile( filename ) ;
  }
    
  void MarkerMetrologyPage::importFromFile(const QString& filename)
  {
    m_measurements.clear();
    QFile f( filename );
    if( f.open(QIODevice::ReadOnly) ) {
      int row = -1;
      while (!f.atEnd()){
	QString line = f.readLine();
	QStringList columns = line.split(";") ;
	columns = columns.replaceInStrings(" ","") ;
	columns = columns.replaceInStrings("\"","") ;
	columns = columns.replaceInStrings("\n","") ;
	qDebug() << "read columns: " << row << columns.size() << columns ;
	if( row==-1 ) {
	  m_markertable->setColumnCount( columns.size() ) ;
	  // let's assume these are the headers
	  m_markertable->setHorizontalHeaderLabels(columns) ;
	  ++row ;
	} else if( columns.size()==m_markertable->columnCount()) {
	  /*
	  m_markertable->setRowCount( row+1 ) ;
	  for( int col = 0; col < m_markertable->columnCount(); ++col ) {
	    QTableWidgetItem *entry = m_markertable->item(row,col) ;
	    if( !entry ) {
	      entry = new QTableWidgetItem(columns.at(col));
	      m_markertable->setItem(row,col,entry);
	    } else {
	      qDebug() << "hoi" ;
	      entry->setData(0, columns.at(col) );
	    }
	    }*/
	  ++row;
	  m_measurements.emplace_back( columns.at(0),
				       columns.at(1).toDouble(),columns.at(2).toDouble(),
				       columns.at(3).toDouble(),ReportCoordinate::Ready);
	} else {
	  qDebug() << "Skipped line" << line ;
	}
      }
    }
    f.close();
    fillTable() ;
  } ;
  
  struct PlaneFitResult
  {
    enum Parameters{Z0,dZdX,dZdY,d2ZdX2,d2ZdXY,d2ZdY2} ;
    Eigen::Vector6d parameters{Eigen::Vector6d::Zero()} ;
    Eigen::Matrix6d covariance{Eigen::Matrix6d::Zero()} ;
    double x0{0} ;
    double y0{0} ;
    double z(double x, double y) const {
      const double dx{x-x0},dy{y-y0} ;
      return parameters[Z0]
	+ dx * parameters[dZdX]
	+ dy * parameters[dZdY]
	+ dx*dx * parameters[d2ZdX2]
	+ dy*dy * parameters[d2ZdY2]
	+ dx*dy * parameters[d2ZdXY] ;
    }
  } ;
  
  class SurfaceMetrologyPage : public MarkerMetrologyPage
  {
  public:
    enum FitModel { Plane, CurvedPlane} ;
    SurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
      : MarkerMetrologyPage{camerasvc,viewdir}
    {
      {
	auto startbutton = new QPushButton{"Auto",this} ;
	connect(startbutton,&QPushButton::clicked,[&]() {
	    //m_activerow = 0 ;
	    this->connectsignals() ;
	    // if active row outside range, reset. otherwise start from where we are.
	    if(m_activerow<0 || m_activerow>=int(m_measurements.size()))
	      m_activerow = 0 ;
	    move() ;
	  });
	m_buttonlayout->addWidget(startbutton) ;
      }
      {
	auto stopbutton = new QPushButton{"Abort",this} ;
	connect(stopbutton,&QPushButton::clicked,[&]() {
	    // connect the signals
	    this->disconnectsignals() ;
	  });
	m_buttonlayout->addWidget(stopbutton) ;
      }
      {
	auto button = new QPushButton{"Fit plane", this} ;
	m_buttonlayout->addWidget(button) ;
	connect(button,&QPushButton::clicked,this,[=]{ this->fitPlane(FitModel::Plane) ; }) ;
      }
      {
	auto button = new QPushButton{"Fit curved", this} ;
	m_buttonlayout->addWidget(button) ;
	connect(button,&QPushButton::clicked,this,[=]{ this->fitPlane(FitModel::CurvedPlane) ; }) ;
      }
    }
    PlaneFitResult fitPlane(FitModel mode = FitModel::Plane, double originx=0, double originy=0) ;
    void disconnectsignals() ;
    void connectsignals() ;
    void measure() ;
  private:
    // fit result. is there a better place to keep this? do we need to keep it at all?
    std::vector<QMetaObject::Connection> m_conns ;
  protected:
    QCheckBox m_ignoreFocusFailed{"Ignore focus failed",this} ;
  } ;
  
  void SurfaceMetrologyPage::disconnectsignals()
  {
    for( auto& c : m_conns ) QObject::disconnect(c) ;
    m_conns.clear() ;
  }

  void SurfaceMetrologyPage::measure()
  {
    // force a recording, but we do not want to go via the signals,
    // because then I cannot control the flow:-(
    //   m_camerasvc->cameraview()->record( camview->localOrigin() ) ;
    auto measurement = m_camerasvc->cameraview()->coordinateMeasurement() ;
    record( measurement ) ;
    if( !m_camerasvc->autofocus()->isFocussed() )
      m_measurements[m_activerow].setStatus( ReportCoordinate::Failed ) ;
    ++m_activerow ;
    if( m_activerow<int(m_measurements.size())) move() ;
    else {
      disconnectsignals();
      this->fitPlane(FitModel::Plane) ;
    }
  }
    
  void SurfaceMetrologyPage::connectsignals()
  {
    Qt::ConnectionType type = Qt::QueuedConnection;
    m_conns.emplace_back( connect( &(MotionSystemSvc::instance()->mainXAxis()), &MotionAxis::movementStopped, this, [=]() { this->focus() ; },
				   type) ) ;
    m_conns.emplace_back( connect( &(MotionSystemSvc::instance()->mainYAxis()), &MotionAxis::movementStopped, this, [=]() { this->focus() ; },
				   type ) ) ;
    m_conns.emplace_back( connect( m_camerasvc->autofocus(), &AutoFocus::focussed, this, [=]() { this->measure() ; },type ) ) ;
    m_conns.emplace_back( connect( m_camerasvc->autofocus(), &AutoFocus::focusfailed, this, [=]() {
	  if( m_ignoreFocusFailed.isChecked() ) this->measure() ;
	  else this->disconnectsignals() ; },
	type ) ) ;
  }
  
  PlaneFitResult SurfaceMetrologyPage::fitPlane(FitModel mode, double originx, double originy)
  {
    Eigen::Vector6d halfdchi2dpar   = Eigen::Vector6d::Zero() ;
    Eigen::Matrix6d halfd2chi2dpar2 = Eigen::Matrix6d::Zero() ;
    size_t numvalid{0} ;
    double sumX{0} ;
    double sumY{0} ;
    double sumZ{0} ;
    
    for( const auto& m : m_measurements )
      if( m.status() == ReportCoordinate::Status::Ready ) {
	++numvalid ;
	Eigen::Vector6d deriv ;
	double dx = m.x() -originx ;
	double dy = m.y() -originy ;
	deriv(0) = 1 ;
	deriv(1) = dx ;
	deriv(2) = dy ;
	deriv(3) = dx*dx ;
	deriv(4) = dx*dy ;
	deriv(5) = dy*dy ;
	halfdchi2dpar += m.z() * deriv ;
	for(int irow=0; irow<6; ++irow)
	  for(int icol=0; icol<6; ++icol)
	    halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol) ;
	sumX += m.x() ;
	sumY += m.y() ;
	sumZ += m.z() ;
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
      double dx = m.x()-originx ;
      double dy = m.y()-originy ;
      double residual = m.z() - (delta(0) + delta(1)*dx + delta(2)*dy + delta(3)*dx*dx + delta(4)*dx*dy+ delta(5)*dy*dy ) ;
      if( !m_markertable->item(row,4) )
	m_markertable->setItem(row,4,new QTableWidgetItem{}) ;
      m_markertable->item(row,4)->setText( QString::number( residual, 'g', 5 ) ) ;
      ++row ;
      if(  m.status() == ReportCoordinate::Status::Ready )
	sumr2 += residual*residual ;
    }
    const double sigmaz2 = 0.005*0.005 ;
    if(ndof>0) {
      const double cogx = sumX/numvalid ;
      const double cogy = sumY/numvalid ;
      os << "Centre-of-gravity: ("
	 << cogx << "," << cogy << "," <<  sumZ/numvalid << ")"
	 << "-->" << (delta(0) + delta(1)*cogx + delta(2)*cogy + delta(3)*cogx*cogx + delta(4)*cogx*cogy+ delta(5)*cogy*cogy) << std::endl ;
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
      fillTable() ;
    }
    QString pageName() const override
    {
      return (viewdir()==ViewDirection::NSideView) ? "NSideTilePositions" : "CSideTilePositions" ;
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
  private:
    QString m_tilename ;
  public:
    SensorSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir, const QString& tilename)
      : SurfaceMetrologyPage{camerasvc,viewdir},m_tilename{tilename}
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
	if(m->toolTip().contains("Sensor") && m->toolTip().contains(m_tilename))
	   m_measurements.emplace_back( m->toolTip(), m->pos().x(), m->pos().y(), sensorZ ) ;
      fillTable() ;
    }
    QString pageName() const override {
      return m_tilename + "SensorSurface" ;
    }
  } ;
 
  QWidget* createSensorSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir, const char* tilename)
  {
    return new SensorSurfaceMetrologyPage{camerasvc,viewdir,tilename} ;
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
      fillTable() ;
    }
    QString pageName() const override {
      return QString{viewdir() == NSideView ? "NSide" : "CSide"} + "SubstrateSurface" ;
    }
  } ;

  QWidget* createSubstrateSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
  {
    return new SubstrateSurfaceMetrologyPage{camerasvc,viewdir} ;
  }

  //****************************************************************************//

  namespace {
    template<typename T>
    struct BoundingRange
    {
      T min{std::numeric_limits<T>::max()} ;
      T max{std::numeric_limits<T>::min()} ;
      void add(const T& x) {
	if(min>x)      min=x ;
	else if(max<x) max=x ;
      }
    } ;
    
    template<typename T=double>
    struct BoundingBox
    {
      BoundingRange<T> x, y ;
      void add( double ax, double ay ) {
	x.add(ax) ;
	y.add(ay) ;
      }
    } ;
  }
  
  class GenericSurfaceMetrologyPage : public SurfaceMetrologyPage
  {
  private:
    //std::vector<ModuleCoordinates> m_trajectory ;
    NamedValue<double> m_zmin{"LineScan.ZMin",23.0} ;  // Minimal z for focus search
    NamedValue<double> m_zmax{"LineScan.ZMax",24.5} ;  // Maximal z for focus search
    NamedValue<double> m_spacing{"AutoFocus.GridSpacing",2.0} ; // maximal distance between grid points
    bool m_keepOriginalMeasurements{true} ;
    QDialog m_settingsdialog ;
  public:
    GenericSurfaceMetrologyPage( CameraWindow& camerasvc, ViewDirection viewdir)
      : SurfaceMetrologyPage{camerasvc,viewdir},
	m_settingsdialog{this}
    {
      // auto button = new QPushButton{"run", this} ;
      // m_buttonlayout->addWidget(button) ;
      // connect(button,&QPushButton::clicked,[=]() { this->run() ; } ) ;

      // configure the settingsdialog
      {
	m_settingsdialog.setWindowTitle( "LineScanPageSettings" ) ;
	m_settingsdialog.setContentsMargins(0, 0, 0, 0);
	//auto layout = new QGridLayout{} ;
	auto layout = new QVBoxLayout{} ;
	m_settingsdialog.setLayout(layout) ;
	layout->addWidget( new NamedValueInputWidget<double>{m_zmin,18.0,25.0,2} ) ;
	layout->addWidget( new NamedValueInputWidget<double>{m_zmax,18.0,26.0,2} ) ;
	layout->addWidget( new NamedValueInputWidget<double>{m_spacing,0,50,2} ) ;
	{
	  auto button = new QPushButton{"build grid", this} ;
	  layout->addWidget(button) ;
	  connect(button,&QPushButton::clicked,[=]() { this->run() ; } ) ;
	}
	{
	  auto button = new QPushButton{"reset all", this} ;
	  layout->addWidget(button) ;
	  connect(button,&QPushButton::clicked,[=]() { this->resetall(); } ) ;
	}
	{
	  auto button = new QPushButton{"accept all", this} ;
	  layout->addWidget(button) ;
	  connect(button,&QPushButton::clicked,[=]() { this->acceptall(); } ) ;
	}
	layout->addWidget(&m_ignoreFocusFailed) ;
      }
      
      auto settingsbutton = new QPushButton(QIcon(":/images/settings.png"),"",this) ;
      connect( settingsbutton, &QPushButton::clicked, [&](){ m_settingsdialog.show() ; } ) ;
      m_buttonlayout->addWidget(settingsbutton);
    }
    //
    void definemarkers() final {
      m_measurements.clear() ;
      fillTable() ;
    }
    // override the focus routine such that we can use the external z-range
    void focus() const override
    {
      if( !MotionSystemSvc::instance()->mainXAxis().isMoving() &&
	  !MotionSystemSvc::instance()->mainYAxis().isMoving() ) {
	m_camerasvc->autofocus()->startFocusSequence(m_zmin,m_zmax) ;
      } ;
    }
    void run()
    {
      // insert measurements such that we get a certain density of points
      if( m_measurements.size()>=4 ) {
	qDebug() << "Number of trajectory points: " << m_measurements.size() ;
	
	// Tag: find the smallest rectangle around all the points
	// This is called the "smallest surrounding rectangle" problem

	// the simplest alg I just found is:
	// * loop over all combinations of points (better if you have them ordered: complex hull problem)
	// * define axis from those points
	// * transform every point to that frame and compute the min/max coordinates to get a bounding box
	// * of all combinations of initial points, take the one with the smallest surface

	const double x0 = m_measurements.front().position().x() ;
	const double y0 = m_measurements.front().position().y() ;

	double cosphi{1},sinphi{0} ;
	BoundingBox<double> bbox ;
	BoundingRange<double> zrange ;
	
	//double x1prime{+9999},x2prime{-9999},y1prime{+9999},y2prime{-9999} ;
	double minarea = -1 ;
	for( size_t i=0;i<m_measurements.size(); ++i) {
	  zrange.add(m_measurements[i].position().z()) ;
	  //sumz += m_measurements[i].position().z() ;
	  for( size_t j=0;j<i; ++j) {
	    const auto& p1 = m_measurements[i].position() ;
	    const auto& p2 = m_measurements[j].position() ;
	    // we can actually choose phi to be in the domain
	    // [-pi/4,pi/4], which means we could significantly
	    // simplify this:-)
	    double thisphi    = std::atan2(p2.y()-p1.y(),p2.x()-p1.x()) ;
	    while( thisphi < -M_PI/4) thisphi += M_PI/2 ; 
	    while( thisphi > +M_PI/4) thisphi -= M_PI/2 ; 
	    const double thiscosphi =std::cos(thisphi) ;
	    const double thissinphi =std::sin(thisphi) ;
	    BoundingBox<double> thisbbox ;
	    {
	      for( const auto& m : m_measurements ) {
		const auto& pos = m.position() ;
		const auto x = (pos.x()-x0)*thiscosphi + (pos.y()-y0)*thissinphi ;
		const auto y = (pos.y()-y0)*thiscosphi - (pos.x()-x0)*thissinphi ;
		thisbbox.add(x,y) ;
	      }
	    }
	    const double thisarea = (thisbbox.x.max-thisbbox.x.min)*(thisbbox.y.max-thisbbox.y.min) ;
	    if( minarea<0 || thisarea < minarea) {
	      minarea = thisarea ;
	      cosphi  = thiscosphi ;
	      sinphi  = thissinphi ;
	      bbox = thisbbox ;
	    }
	  }
	}
	std::cout << "phi angle = " << std::atan2(sinphi,cosphi)*180/M_PI
		  << " " << sinphi << " " << cosphi << std::endl ;
	const auto& x1prime = bbox.x.min ;
	const auto& x2prime = bbox.x.max ;
	const auto& y1prime = bbox.y.min ;
	const auto& y2prime = bbox.y.max ;
	std::cout << "x1prime, x2prime: " << bbox.x.min << " " << bbox.x.max << std::endl ;
	std::cout << "y1prime, y2prime: " << bbox.y.min << " " << bbox.y.max << std::endl ;
	// whap happens if we rotate these back
	std::cout << "x1, y1: "
		  << x0 + x1prime*cosphi - y1prime*sinphi << ", "
		  << y0 + y1prime*cosphi + x1prime*sinphi << std::endl
		  << "x2, y2: "
		  << x0 + x2prime*cosphi - y2prime*sinphi << ", "
		  << y0 + y2prime*cosphi + x2prime*sinphi << std::endl ;
	// compute the new grid points
	const int nx = std::max(int((x2prime-x1prime)/m_spacing),2) ;
	const int ny = std::max(int((y2prime-y1prime)/m_spacing),2) ;
	std::cout << "nx,ny: " << nx << " " << ny << std::endl ;
	std::vector<ReportCoordinate>  newmeasurements  ;
	newmeasurements.reserve( nx*ny ) ;
	for(int ix=0; ix<nx; ++ix) 
	  for(int iy=0; iy<ny; ++iy) {
	    const double xprime = x1prime + ix*(x2prime-x1prime)/(nx-1) ;
	    const double yprime = y1prime + iy*(y2prime-y1prime)/(ny-1) ;
	    const double x = x0 + xprime*cosphi - yprime*sinphi ;
	    const double y = y0 + yprime*cosphi + xprime*sinphi ;
	    QString name = QString{"p"} + QString::number(ix) + QString::number(iy) ;
	    newmeasurements.emplace_back( name , x, y, 0.5*(zrange.min+zrange.max) ) ;
	  }
	if(m_keepOriginalMeasurements) {
	  m_measurements.insert(m_measurements.end(),newmeasurements.begin(),newmeasurements.end()) ;
	} else {
	  m_measurements.swap(newmeasurements) ;
	}
	m_zmin = m_camerasvc->autofocus()->focusFromZ( zrange.max ) - 0.1 ;
	m_zmax = m_camerasvc->autofocus()->focusFromZ( zrange.min ) + 0.1 ;
      }
      fillTable() ;
    }
  } ;
  


  QWidget* createGenericSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir)
  {
    return new GenericSurfaceMetrologyPage{camerasvc,viewdir} ;
  }

  //****************************************************************************//
  class LineScanPage : public SurfaceMetrologyPage
  {
  private:
    //std::vector<ModuleCoordinates> m_trajectory ;
    NamedValue<double> m_zmin{"LineScan.ZMin",23.0} ;  // Minimal z for focus search
    NamedValue<double> m_zmax{"LineScan.ZMax",24.5} ;  // Maximal z for focus search
    NamedValue<double> m_spacing{"AutoFocus.GridSpacing",2.0} ; // maximal distance between grid points
    QDialog m_settingsdialog ;
  public:
    LineScanPage( CameraWindow& camerasvc, ViewDirection viewdir)
      : SurfaceMetrologyPage{camerasvc,viewdir},
	m_settingsdialog{this}
    {
      // auto button = new QPushButton{"run", this} ;
      // m_buttonlayout->addWidget(button) ;
      // connect(button,&QPushButton::clicked,[=]() { this->run() ; } ) ;

      // configure the settingsdialog
      {
	m_settingsdialog.setWindowTitle( "LineScanPageSettings" ) ;
	m_settingsdialog.setContentsMargins(0, 0, 0, 0);
	//auto layout = new QGridLayout{} ;
	auto layout = new QVBoxLayout{} ;
	m_settingsdialog.setLayout(layout) ;
	layout->addWidget( new NamedValueInputWidget<double>{m_zmin,18.0,25.0,2} ) ;
	layout->addWidget( new NamedValueInputWidget<double>{m_zmax,18.0,26.0,2} ) ;
	layout->addWidget( new NamedValueInputWidget<double>{m_spacing,0,50,2} ) ;
	auto button = new QPushButton{"run", this} ;
	layout->addWidget(button) ;
	connect(button,&QPushButton::clicked,[=]() { this->run() ; } ) ;
      }
      
      auto settingsbutton = new QPushButton(QIcon(":/images/settings.png"),"",this) ;
      connect( settingsbutton, &QPushButton::clicked, [&](){ m_settingsdialog.show() ; } ) ;
      m_buttonlayout->addWidget(settingsbutton);
    }
    //
    void definemarkers() final {
      m_measurements.clear() ;
      fillTable() ;
    }
    
    void run()
    {
      // insert measurements such that we get a certain density of points
      if( !m_measurements.empty() ) {
	qDebug() << "Number of trajectory points: " << m_measurements.size() ;
	std::vector<ReportCoordinate>  newmeasurements  ;
	newmeasurements.reserve( m_measurements.size() ) ;
	newmeasurements.emplace_back( m_measurements.front() ) ;
	// add the first point
	auto it = m_measurements.begin() ;
	auto prev = it ;
	for( ++it ; it != m_measurements.end(); ++it ) {
	  const auto p1 = prev->position() ;
	  const auto p2 = it->position() ;
	  const auto delta = p2 - p1 ;
	  const unsigned int numpoints = int(delta.length()/m_spacing) ;
	  for(unsigned int j=1; j<numpoints; ++j) {
	    const auto p = p1 + delta * j/numpoints ;
	    newmeasurements.emplace_back( prev->name() + "_" + QString::number(j+1), p ) ;
	  }
	  newmeasurements.emplace_back( *it ) ;
	  prev = it ;
	}
	m_measurements.swap(newmeasurements) ;	    
      }
      fillTable() ;
    }
  } ;

  
  
  //****************************************************************************//
  class SideReportPage : public QWidget
  {
  private:
    TileMetrologyPage* m_tilemarkerpage{0} ;
    SensorSurfaceMetrologyPage* m_sensorsurfacepage[2] ;
    SubstrateSurfaceMetrologyPage* m_substratesurfacepage{0} ;
  public:
    SideReportPage( QWidget* parent,
		    TileMetrologyPage* tp,
		    SensorSurfaceMetrologyPage* sensp1,
		    SensorSurfaceMetrologyPage* sensp2,
		    SubstrateSurfaceMetrologyPage* subp)
      : QWidget{parent},
	m_tilemarkerpage{tp},
	m_sensorsurfacepage{sensp1,sensp2},
	m_substratesurfacepage{subp}
    {
      auto layout =  new QVBoxLayout{} ;
      this->setLayout(layout) ;
      {
	auto button = new QPushButton{"Import",this} ;
	layout->addWidget(button) ;
	connect(button,&QPushButton::clicked,this,[=]{ this->import() ; }) ;
      }
      {
	auto button = new QPushButton{"FitThickness",this} ;
	layout->addWidget(button) ;
	connect(button,&QPushButton::clicked,this,[=]{ this->fitThickness() ; }) ;
      }
      {
	auto button = new QPushButton{"Reset",this} ;
	layout->addWidget(button) ;
	connect(button,&QPushButton::clicked,this,[=]{ this->reset() ; }) ;
      }
    }
    
    void fitThickness() {
      // first fit the substrate
      PlaneFitResult substratefit = m_substratesurfacepage->fitPlane(SurfaceMetrologyPage::FitModel::CurvedPlane) ;
      // now fit the tiles
      for(int itile=0; itile<2; ++itile) {
	PlaneFitResult sensorfit = m_sensorsurfacepage[itile]->fitPlane(SurfaceMetrologyPage::FitModel::CurvedPlane) ;
	// so now I have tow planes. but where do I compare them? and
	// do I actually know in which reference frame we are?
	for( const auto& m : m_sensorsurfacepage[itile]->measurements() ) {
	  const auto zsensor = sensorfit.z( m.x(),m.y()) ;
	  const auto zsubstrate = substratefit.z( m.x(),m.y()) ;
	  qDebug() << m.name() << zsensor - zsubstrate ;
	}
      }
    }

    void import()
    {
      std::vector< MarkerMetrologyPage* > pages{m_tilemarkerpage,
	  m_sensorsurfacepage[0],m_sensorsurfacepage[1],m_substratesurfacepage} ;
      for( auto& page : pages )
	page->importFromFile( page->defaultFileName() ) ;
    }
    void reset()
    {
      QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, "Reset", "Reset all metrology data?",QMessageBox::Yes|QMessageBox::No);
      if (reply == QMessageBox::Yes) {
	std::vector< MarkerMetrologyPage* > pages{m_tilemarkerpage,
	    m_sensorsurfacepage[0],m_sensorsurfacepage[1],m_substratesurfacepage} ;
	for( auto& page : pages ) page->reset() ;
      }
    }
  } ;
  
  //****************************************************************************//
  class SideMetrologyPage : public QTabWidget
  {
  public:
    SideMetrologyPage( CameraWindow& camerasvc, ViewDirection view)
    {
      TileMetrologyPage* tilemarkerpage = new TileMetrologyPage{camerasvc,view} ;
      this->addTab(tilemarkerpage,"Tile metrology") ;
      
      SensorSurfaceMetrologyPage* sensorsurfacepage[2] ;
      for(int tile=0; tile<2; ++tile) {
	const auto name = getTileInfo(view,TileType(tile)).name ;
	sensorsurfacepage[tile] =
	  new SensorSurfaceMetrologyPage{camerasvc,view,name} ;
	this->addTab( sensorsurfacepage[tile],name + " surface metrology") ;
      }
      SubstrateSurfaceMetrologyPage* 
	substratesurfacepage = new SubstrateSurfaceMetrologyPage{camerasvc,view} ;
      this->addTab(substratesurfacepage,"Substrate surface metrology") ;
      
      this->addTab(new SideReportPage{this,tilemarkerpage,
	    sensorsurfacepage[0],sensorsurfacepage[1],substratesurfacepage},"Report") ;

      //this->addTab(new LineScanPage{camerasvc,view}, "Generic surface") ;
      this->addTab(new GenericSurfaceMetrologyPage{camerasvc,view}, "Generic surface") ;
    }
  } ;
  
  QTabWidget* createSideMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) {
    return new SideMetrologyPage{camerasvc,viewdir} ;
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
