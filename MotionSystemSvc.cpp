#include "MotionSystemSvc.h"
#include <QDebug>
#include <cstdlib>
#include <QSerialPortInfo>
#include <QThread>
#include <QRegularExpression>
#include <QObject>
#include <QTimer>
//#include <iostream>

namespace PAP
{
  
  MotionSystemSvc* MotionSystemSvc::gInstance=0 ;
  
  MotionSystemSvc::~MotionSystemSvc()
  {
    if(gInstance==this) gInstance = 0 ;
  }
  
  MotionSystemSvc::MotionSystemSvc()
    :  m_serialport(0), m_isReady(false)
  {
    if( gInstance==0 ) gInstance = this ;
    

    // let's create the controllers and the axis. actally, we cannot do
    // anything with them without a serial port, but it is convenient to
    // have them for developing the widgets. this looks still very ugly,
    // but I'll fix it once I really know what I need.
    m_controllers[2] = new MotionController(2,"Controller RIGHT") ;
    m_controllers[4] = new MotionController(4,"Controller LEFT") ;
    std::vector<int> controllerid = {4,4,2,2,4,2} ;
    std::vector<int> axisid       = {2,1,1,2,3,3} ;
    std::vector<double> tolerance = {0.001,0.001,0.0001,0.0001,1e-6,0.0001} ;
    std::vector<QString> axisname  = { "MainX","MainY","StackX","StackY","StackR","Focus" } ;
    for(int i=0; i<6; ++i) {
      MotionAxisID id(controllerid[i],axisid[i]) ;
      QString atype ;
      m_axes[id] = new MotionAxis{id,axisname[i],atype,*(m_controllers[id.controller])} ;
      m_controllers[controllerid[i]]->addAxis( m_axes[id] ) ;
      m_axes[id]->tolerance() = tolerance[i] ;
    }
    m_mainXAxis = axis("MainX") ;
    m_mainYAxis = axis("MainY") ;
    m_focusAxis = axis("Focus") ;
    m_stackXAxis = axis("StackX") ;
    m_stackYAxis = axis("StackY") ;
    m_stackRAxis = axis("StackR") ;
    
    // some signal 'forwarding'
    connect(&(m_mainXAxis->position()),&NamedValueBase::valueChanged,this,&MotionSystemSvc::mainStageMoved) ;
    connect(&(m_mainYAxis->position()),&NamedValueBase::valueChanged,this,&MotionSystemSvc::mainStageMoved) ;

    connect(&(m_stackXAxis->position()),&NamedValueBase::valueChanged,this,&MotionSystemSvc::stackStageMoved) ;
    connect(&(m_stackYAxis->position()),&NamedValueBase::valueChanged,this,&MotionSystemSvc::stackStageMoved) ;
    connect(&(m_stackRAxis->position()),&NamedValueBase::valueChanged,this,&MotionSystemSvc::stackStageMoved) ;
    
    // This explains who to do this in c
    //https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
    
    // I now know what to do. We use the example from
    //  /Users/wouter/Qt/5.7/Src/qtserialport/examples/serialport/terminal
    //  * it seems that the reading works only asynchronous. luckily the
    //  motion system always returns an asnwer with the code of the
    //  original question attached. the only trouble is that you need to
    //  know which controller it was.
    // * I would like one window where I echo all in and outcoming
    //  communication and eventually specify communication by hand. For
    //  that I will use the 'terminal' in the example.
    
    // for now, open with a hardcoded name. later we'll put this somewhere in a menu
    qInfo() << "Listing available ports:" ;
    QSerialPortInfo theinfo ;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
      qInfo() << "Name : " << info.portName();
      qInfo() << "Description : " << info.description();
      qInfo() << "Manufacturer: " << info.manufacturer();
      // Example use QSerialPort
      if( QString(info.description()).contains("GPIB") ) {
	qInfo() << "Choosing this port" ;
	theinfo = info ;
      }
    }
    
    m_serialport = new MotionSystemSerialPort(*this,theinfo) ;
    if( m_serialport->isOpen() ) {
      // read parameter values for all axis
      for( auto& axis : m_axes) axis.second->readParameters() ;
    }
    qDebug() << "status of controller 2: " << m_controllers[2]->status() ;
    qDebug() << "status of controller 4: " << m_controllers[4]->status() ;
    // FIXME: soon only set 'ready' if we actually see the controllers
    m_isReady = true; //m_controllers[2]->status()!=0 && m_controllers[4]->status()!=0 ;
  }
  
  MotionSystemSvc* MotionSystemSvc::instance()
  {
    if( !gInstance ) gInstance = new MotionSystemSvc() ;
    return gInstance ;
  }
  
  void MotionSystemSvc::parseData(int controllerid, const QByteArray& data )
  {
    // this all assumes that we receive data for the current controller.
    //qDebug() << "MotionSystemSvc, parsing message: " << data ;
    QStringList lines = QString(data).split(QRegularExpression("\n|\r\n|\r|\\,\\d"));
    for( const auto& line : lines )
      if( line.size()>0 ) {
	//qDebug() << "MotionSystemSvc, parsing line: " << line ;
	QRegularExpression re{"^(\\d*)(\\w\\w)(.+)"} ;
	QRegularExpressionMatch match = re.match( line ) ;
	if( match.hasMatch() ) {
	  //qDebug() << "Matched a regularexpression!"
	  //	   << match.captured(1) << " "
	  //	   << match.captured(2) << " "
	  //	   << match.captured(3) ;
	  QString axisstr = match.captured(1) ;
	  QString command = match.captured(2) ;
	  QString result  = match.captured(3) ;
	  if( axisstr.size()==0 ) {
	    MotionController* controller = m_controllers[ controllerid ] ;
	    if(       command.startsWith("TS") ) {
	      QChar statusbyte = result[0] ;
	      controller->setStatus(statusbyte.toLatin1() )  ;
	    } else if(command.startsWith("TE") || command.startsWith("TB") ) {
	      QChar errorbyte = result[0] ;
	      controller->setErrorCode(errorbyte.toLatin1() )  ;
	      controller->setErrorMsg(result) ;
	    }
	  } else {
	    int axis = match.captured(1).toInt() ;
	    auto it = m_axes.find( MotionAxisID( controllerid, axis ) ) ;
	    if( it != m_axes.end() ) {
	      if( !(it->second->parseData( command, result )) ) {
		qWarning() << "Problem parsing data for axis: "
			   << line ;
	      }
	    }
	  }
	} else {
	  qWarning() << "Don't know how to parse this line: "
		     << line ;
	  qDebug() << "Data was: " << data ;
	}
      }
  }
  
  
  void MotionSystemSvc::configure( const MotionAxisID& /*id*/ ) const
  {
    qDebug() << "To be implemented!" ;
  }
  
  void MotionSystemSvc::applyAxisCommand( const MotionAxisID& id,
					  const char* command,
					  double nn ) const
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s%f",id.axis,command,nn) ;
    m_serialport->addCommand(id.controller,acommand) ;
  }
  
  void MotionSystemSvc::applyAxisCommand( const MotionAxisID& id,
					  const char* command,
					  int nn ) const
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s%d",id.axis,command,nn) ;
    m_serialport->addCommand(id.controller,acommand) ;
  }
  
  void MotionSystemSvc::applyAxisCommand( const MotionAxisID& id,
					  const char* command,
					  const char* nn ) const
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s%s",id.axis,command,nn) ;
    m_serialport->addCommand(id.controller,acommand) ;
  }
  
  void MotionSystemSvc::applyAxisCommand( const MotionAxisID& id,
					  const char* command ) const
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s",id.axis,command) ;
    m_serialport->addCommand(id.controller,acommand) ;
  }

  void MotionSystemSvc::applyAxisReadCommand( const MotionAxisID& id,
					      const char* command )
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s",id.axis,command) ;
    m_serialport->addCommand(id.controller,acommand,true) ;
  }
  
  void MotionSystemSvc::readAxisVariable(  const MotionAxisID& id,
					   const char* command,
					   MonitoredValueBase& var) const
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s",id.axis,command) ;
    m_serialport->addCommand(id.controller,acommand,true,&var) ;
  }

  void MotionSystemSvc::applyControllerCommand( const MotionControllerID& id,
						const char* command ) const
  {
    m_serialport->addCommand(id, command ) ;
  }
  
  void MotionSystemSvc::emergencyStop() const
  {
    qWarning() << "Emergency stop" ;
    m_serialport->addCommand(2,"ST") ;
    m_serialport->addCommand(4,"ST") ;
    m_serialport->addCommand(2,"MF") ;
    m_serialport->addCommand(4,"MF") ;
  }
  
  MSCoordinates MotionSystemSvc::coordinates() const
  {
    MSCoordinates rc ;
    rc.main = maincoordinates() ;
    rc.stack = stackcoordinates() ;
    rc.focus = m_focusAxis->position() ;
    return rc ;
  }
  
  MSMainCoordinates MotionSystemSvc::maincoordinates() const
  {
    MSMainCoordinates rc ;
    rc.x = m_mainXAxis->position() ;
    rc.y = m_mainYAxis->position() ;
    return rc ;
  }
  
  MSStackCoordinates MotionSystemSvc::stackcoordinates() const
  {
    MSStackCoordinates rc ;
    rc.x = m_stackXAxis->position() ;
    rc.y = m_stackYAxis->position() ;
    rc.phi = m_stackRAxis->position() ;
    return rc ;
  }
    
  MotionAxis* MotionSystemSvc::axis( const QString& name)
  {
    MotionAxis* rc(0) ;
    for(  auto& it : m_axes ) {
      if(it.second->name() == name ) {
	rc = it.second ;
	break;
      }
    }
    return rc ;
  }
}
  
