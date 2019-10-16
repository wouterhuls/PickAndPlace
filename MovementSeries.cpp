
#include "MovementSeries.h"
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
#include <QPlainTextEdit>
#include "CameraView.h"
#include "AutoFocus.h"
#include "MetrologyReport.h"
#include "TextEditStream.h"
#include <iostream>

// Let's implement the list of measurements as a qabstracttablemodel
// http://doc.qt.io/qt-5/qabstracttablemodel.html#details
// https://stackoverflow.com/questions/11906324/binding-model-to-qt-tableview


namespace PAP
{
  
  MovementSeries::MovementSeries(CameraWindow& camerasvc, ViewDirection viewdir)
    : QWidget{&camerasvc}, m_camerasvc{&camerasvc}, m_viewdir{viewdir}, m_textbox{new QPlainTextEdit{this}}
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
    m_table = new QTableWidget{1,4,this} ;
    m_table->resize(500,150) ;
    //m_table->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding) ;
    m_table->setHorizontalHeaderLabels(QStringList{"Marker","X","Y","Z","residual Z"}) ;
    hlayout->addWidget(m_table) ;
    connect(m_table,&QTableWidget::cellClicked,[&](int row, int /*column*/) { this->activateRow( row ) ; }) ;
    //connect(camerasvc()->cameraview(),&CameraView::recording,this,&MovementSeries::record) ;

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
      connect(button,&QPushButton::clicked,this,[=]{ this->addCoordinate() ; }) ;
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
    {
      auto startbutton = new QPushButton{"Start",this} ;
      connect(startbutton,&QPushButton::clicked,[&]() {
	  //m_activerow = 0 ;
	  this->connectsignals() ;
	  // if active row outside range, reset. otherwise start from where we are.
	  if(m_currentcoordinate<0 || m_currentcoordinate>=int(m_coordinates.size())) {
	    this->initialize() ;
	    m_currentcoordinate = 0 ;
	  }
	  move() ;
	});
      m_buttonlayout->addWidget(startbutton) ;
    }
    {
      auto stopbutton = new QPushButton{"Stop",this} ;
      connect(stopbutton,&QPushButton::clicked,[&]() {
	  // connect the signals
	  this->disconnectsignals() ;
	});
      m_buttonlayout->addWidget(stopbutton) ;
    }
  }

  void MovementSeries::reset()
  {
    m_textbox->setPlainText("") ;
    m_coordinates.clear() ;
    initializeCoordinates() ;
  }

  QString MovementSeries::defaultFileName() const
  {
    return QString("/home/velouser/Documents/PickAndPlaceData/") +
      camerasvc()->moduleName() + "/" + pageName() + ".csv" ;
  }
  
  void MovementSeries::fillTable()
  {
    int N = int(m_coordinates.size()) ;
    if(N>=0) {
      m_table->setRowCount(N) ;
      QTableWidgetItem prototype ;
      prototype.setBackground( QBrush{ QColor{Qt::white} } ) ;
      prototype.setFlags( Qt::ItemIsSelectable ) ;
      int row{0} ;
      for( const auto& m : m_coordinates ) {
	for(int icol=0; icol<m_table->columnCount(); ++icol )  
	  m_table->setItem(row,icol,new QTableWidgetItem{prototype} ) ;
	m_table->item(row,0)->setText( m.name() ) ;
	updateTableRow( row++, m ) ;
      }
    } 
  }

  void MovementSeries::setCoordinates( const std::vector<Coordinate>& newpoints )
  {
    m_coordinates = newpoints ;
    fillTable() ;
    m_currentcoordinate = -1 ;
  }
  
  void MovementSeries::addCoordinate()
  {
    // popup a window to ask for a name
    bool ok{true} ;
    QString label = QInputDialog::getText(this,
					      tr("QInputDialog::getText()"),
					      tr("Measurement label:"), QLineEdit::Normal, "Unknown", &ok);
    if (ok && !label.isEmpty()) {
      auto measurement = camerasvc()->cameraview()->coordinateMeasurement() ;
      const auto viewdir = camerasvc()->cameraview()->currentViewDirection() ;
      const QTransform fromGlobalToModule = GeometrySvc::instance()->fromModuleToGlobal(viewdir).inverted() ;
      const auto modulecoordinates = fromGlobalToModule.map( measurement.globalcoordinates ) ;
      const auto z = GeometrySvc::instance()->moduleZ( m_viewdir, measurement.mscoordinates.focus ) ;
      m_coordinates.emplace_back( label, modulecoordinates.x(), modulecoordinates.y(),
				   z, ReportCoordinate::Initialized) ;
      qDebug() << "Measurement test: "
	       << measurement.modulecoordinates
	       << m_coordinates.back().position() ;
      fillTable() ;
      activateRow( m_coordinates.size()-1 ) ;
    }
  }  
  
  void MovementSeries::updateTableRow( int row, const ReportCoordinate& coord ) {
    m_table->item(row,1)->setText( QString::number( coord.x(), 'g', 5 ) ) ;
    m_table->item(row,2)->setText( QString::number( coord.y(), 'g', 5 ) ) ;
    m_table->item(row,3)->setText( QString::number( coord.z(), 'g', 5 ) ) ;
    const auto color = m_coordinates[row].status() == ReportCoordinate::Status::Ready ? Qt::green : Qt::gray ;
    m_table->item(row,0)->setBackground( QBrush{ QColor{color} } ) ;
  }
  
  void MovementSeries::activateRow( int row ) {
    // uncolor the current row
    if( row != m_currentcoordinate ) {
      if( m_currentcoordinate>=0 && m_currentcoordinate<int(m_coordinates.size()) ) {
	m_table->item(m_currentcoordinate,0)->
	  setBackground( QBrush{ QColor{m_coordinates[m_currentcoordinate].status() == ReportCoordinate::Status::Ready ? Qt::green : Qt::gray} } ) ;
      }
      m_currentcoordinate = row ;
      auto& m = m_coordinates[row] ;
      m_table->item(row,0)->setBackground( QBrush{ QColor{m.status() ==  ReportCoordinate::Status::Ready ? Qt::blue : Qt::yellow} } ) ;
      // now move to the marker position as currently set in the measurement
      if( int(m.status()) >= int(ReportCoordinate::Status::Initialized) ) {
	camerasvc()->cameraview()->moveCameraToPointInModule( QPointF{m.x(),m.y()} ) ;
	if( m.status() == ReportCoordinate::Status::Ready ) {
	  qDebug() << "Moving camera to z: " << m.z() ;
	  camerasvc()->autofocus()->moveFocusToModuleZ( m.z() ) ;
	  // move the camera to the current focus point, if any
	  //camerasvc()->autofocus()->moveFocusToModuleZ( m.m_z ) ;
	  // call the autofocus!
	  // camerasvc()->autofocus()->startNearFocusSequence() ;
	}	
      }
    }
  }

  void MovementSeries::focus() const
  {
    qDebug() << camerasvc() ;
    const double currentfocus = camerasvc()->autofocus()->currentFocus() ;
    camerasvc()->autofocus()->startFocusSequence(currentfocus-0.3,currentfocus+0.3) ;
  }
  
  void MovementSeries::move() const
  {
    if( m_currentcoordinate < int(m_coordinates.size()) ) {
      const auto& m = m_coordinates[m_currentcoordinate] ;
      camerasvc()->cameraview()->moveCameraToPointInModule( QPointF{m.x(),m.y()} ) ;
      m_table->item(m_currentcoordinate,0)->setBackground( QBrush{ QColor{m.status() ==  ReportCoordinate::Status::Ready ? Qt::blue : Qt::yellow} } ) ;
    }
  }

  void MovementSeries::disconnectsignals()
  {
    for( auto& c : m_conns ) QObject::disconnect(c) ;
    m_conns.clear() ;
  }
  
  void MovementSeries::connectsignals()
  {
    Qt::ConnectionType type = Qt::QueuedConnection;
    m_conns.emplace_back( connect( this, &MovementSeries::movementReady, this, [=]() { this->execute() ; },type) ) ;
    m_conns.emplace_back( connect( MotionSystemSvc::instance(), &MotionSystemSvc::mainStageStopped, this, [=]() { emit this->movementReady() ; },type) ) ;
    m_conns.emplace_back( connect( this, &MovementSeries::executeReady, this, [=]() {
	  if( ++m_currentcoordinate < int(m_coordinates.size()) ) this->move() ;
	  else this->finalize() ;
	}, type ) ) ;
    m_conns.emplace_back( connect( this, &MovementSeries::finalizeReady, this, [=]() { this->disconnectsignals() ; }, type ) ) ;
  }
  
  void MovementSeries::record(const CoordinateMeasurement& measurement)
  {
    if( m_currentcoordinate >= 0 && m_currentcoordinate < int(m_coordinates.size()) ) {
      // initialize enough measurements, if possible needed
      if( m_currentcoordinate >= int(m_coordinates.size()) )
	m_coordinates.resize( m_currentcoordinate+1 ) ;
      // now store the measurement
      auto& reportcoordinate = m_coordinates[m_currentcoordinate] ;
      reportcoordinate = measurement.modulecoordinates ;
      reportcoordinate.setStatus( ReportCoordinate::Ready ) ;
      // finally update the table
      //updateTableRow(m_currentcoordinate,reportcoordinate) ;
      fillTable() ;
      // Let's also export it to the text stream, such that we can easily copy paste it.
      std::stringstream os ;
      os << reportcoordinate.name().toStdString() << ": " << reportcoordinate.x() << ", " << reportcoordinate.y() << ", "
	 << reportcoordinate.z()  ;
      m_textbox->appendPlainText( os.str().c_str() ) ;
    }
  }
  
  void MovementSeries::recordCentre()
  {
    return record(camerasvc()->cameraview()->coordinateMeasurement()) ;
  }
    
  void MovementSeries::exportToFile() const
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
      for( int c = 0; c < m_table->columnCount(); ++c ) {
	strList <<  "\" " +
	  m_table->horizontalHeaderItem(c)->data(Qt::DisplayRole).toString() +
	  "\" ";
      }
      data << strList.join(";") << "\n";
    }
    // now the data. why would we take it from the table? only to add the residuals?
    for(int irow=0; irow<= m_table->rowCount() && irow<int(m_coordinates.size()); ++irow ) {
      if( m_coordinates[irow].status() == ReportCoordinate::Ready ) {
	QStringList strList;
	for( int icol=0; icol < m_table->columnCount(); ++icol ) 
	  strList << m_table->item(irow,icol)->text() ;
	data << strList.join(";") << "\n";
      }
    }
    f.close() ;
    f.setPermissions( QFileDevice::ReadOther|QFileDevice::ReadGroup|QFileDevice::ReadOwner ) ;
  }

  void MovementSeries::importFromFile()
  {
    
    // pop up a dialog to get a file name
    auto filename = QFileDialog::getOpenFileName(nullptr, tr("Save data"),
						 defaultFileName(),
						 tr("CSV files (*.csv)"));
    
    importFromFile( filename ) ;
  }
    
  void MovementSeries::importFromFile(const QString& filename)
  {
    m_coordinates.clear();
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
	  m_table->setColumnCount( columns.size() ) ;
	  // let's assume these are the headers
	  m_table->setHorizontalHeaderLabels(columns) ;
	  ++row ;
	} else if( columns.size()==m_table->columnCount()) {
	  /*
	  m_table->setRowCount( row+1 ) ;
	  for( int col = 0; col < m_table->columnCount(); ++col ) {
	    QTableWidgetItem *entry = m_table->item(row,col) ;
	    if( !entry ) {
	      entry = new QTableWidgetItem(columns.at(col));
	      m_table->setItem(row,col,entry);
	    } else {
	      qDebug() << "hoi" ;
	      entry->setData(0, columns.at(col) );
	    }
	    }*/
	  ++row;
	  m_coordinates.emplace_back( columns.at(0),
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
}
