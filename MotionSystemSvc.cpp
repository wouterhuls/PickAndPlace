#include "MotionSystemSvc.h"
#include <QDebug>
#include <cstdlib>
#include <QSerialPortInfo>
#include <QThread>
#include "Console.h"

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
  
  void MotionSystemSvc::dummyfunction() {}
  
  MotionSystemSvc::MotionSystemSvc()
    :  m_currentcontrollerid(-1), m_console(0)
  {
    if( gInstance==0 ) gInstance = this ;
    
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
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
      //qDebug() << "Name : " << info.portName();
      //qDebug() << "Description : " << info.description();
      //qDebug() << "Manufacturer: " << info.manufacturer();
      // Example use QSerialPort
      if( info.portName()=="cu.usbserial-PX1FXU5V" /*"cu.usbserial-PX9I7SZ6"*/) {
	m_serialport.setPort(info)  ;
      }
    }
    m_serialport.setBaudRate(QSerialPort::Baud115200) ;
    //m_serialport.setBaudRate(QSerialPort::Baud1200) ;
    //m_serialport.setPortName("cu.usbserial-PX9I7SZ6") ;
    if( m_serialport.open(QIODevice::ReadWrite) ) {
      qDebug() << "MotionSystemSvs: Successfully opened port"
               << m_serialport.baudRate() << " "
               << m_serialport.errorString() << " "
               << m_serialport.flowControl() << " "
               << m_serialport.parity() ;
      
      // let's connect all output asynchroneously to the readData
      // function. unfortunately, that seems to be the only fool proof
      // method.
      
      //connect(&m_serialport, &QSerialPort::readyRead, this, &MotionSystemSvc::readData);
      
      // configuration: make sure to disable autoread, because there
      // is a problem with latency between controller and GPIB-USB
      // converter
      m_serialport.write("++auto 0\n") ;
      
      m_serialport.write("++ver\n") ;
      // here we have a problem with the time-out.
      // perhaps that's a configuration issue, or something inside QSerialIO.
      // (there is no problem when communicating from the command line.)
      QByteArray readData = m_serialport.readAll();
      while (m_serialport.waitForReadyRead(100))
	readData.append(m_serialport.readAll());
      qDebug() << readData ;
      
      // let's not actually enable the motors:-)
      // write(4,"TE") ;
      // auto data = read() ;
      // qDebug() <<  "AA: " << data ;
      // write(4,"TS") ;
      // data = read() ;
      // qDebug() <<  "AB: " << data ;

    } else {
      qDebug() << "MotionSystemSvc: Cannot open port" ;
    }
    
    // let's create the controllers and the axis. actally, we cannot do
    // anything with them without a serial port, but it is convenient to
    // have them for developing the widgets. this looks still very ugly,
    // but I'll fix it once I really know what I need.
    m_controllers[2] = new MotionController(2,"Controller A") ;
    m_controllers[4] = new MotionController(4,"Controller B") ;
    
    std::vector<int> controllerid = {4,4,2,2,4,2} ;
    std::vector<int> axisid       = {1,2,1,2,3,3} ;
    std::vector<std::string> axisname  = { "MainX","MainY","StackX","StackY","StackR","Focus" } ;
    for(int i=0; i<6; ++i) {
      MotionAxisID id(controllerid[i],axisid[i]) ;
      std::string atype ;
      // let's read the type, if we can
      if( m_serialport.isOpen() ) {
	char acommand[256] ;
	sprintf(acommand,"%2dTA", id.axis ) ;
	write( id.controller, acommand ) ;
	atype = read().constData() ;
      }
      qDebug() << id.controller << " " << id.axis ;
      m_axes[id] = new MotionAxis{id,axisname[i],atype,*(m_controllers[id.controller])} ;
    }
    
    if( m_serialport.isOpen() ) {
      // let's first just read the status
      write(2,"TS") ;
      qDebug() << "status of controller 2: " << read() ;
      write(4,"TS") ;
      qDebug() << "status of controller 4: " << read() ;
      //motorsOff() ;
      //motorsOn() ;
    }
    
    // create a QTimer that will update all information from the motion system
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(updateAllData()));
    timer->start(5000);
    
    qDebug() << "End of MotionSystemSvc constructor!" ;
  }
  
  MotionSystemSvc* MotionSystemSvc::instance()
  {
    if( !gInstance ) gInstance = new MotionSystemSvc() ;
    return gInstance ;
  }
  
  void MotionSystemSvc::write( int motioncontrollerid, const char* command ) const
  {
    if( m_currentcontrollerid != motioncontrollerid ) {
      char controlleridcommand[256] ;
      sprintf(controlleridcommand,"++addr %d",motioncontrollerid) ;
      write(controlleridcommand) ;
      m_currentcontrollerid = motioncontrollerid ;
      QThread::msleep(100) ;
    }
    write(command) ;
  }
  
  void MotionSystemSvc::write(const char* command) const
  {
    qDebug() << "Writing command: \"" << command << "\"" ;
    if( m_serialport.isOpen() ) {
      m_serialport.write(command) ;
      m_serialport.write("\n") ;
      m_serialport.waitForBytesWritten(1000) ;
    } ;
  }
  
  
  QByteArray MotionSystemSvc::read() const
  {
    QByteArray readData ;
    if( m_serialport.isOpen() ) {
      // I now think that there is a latency problem between the
      // GPIB-USB converter and the motion system: the read command
      // comes too early. The default time-out is 500 ms, which you
      // would think is enough. We could perhaps try with a 'wait' call
      // right here:
      QThread::msleep(50) ;
      m_serialport.write("++read eoi\n") ;
      
      // the sleep call is very annoying because it pauses the event
      // loop. the correct solution is to put this in a separate thread,
      // but then I need to learn a lot:
      
      // http://doc.qt.io/qt-5/qthread.html#details
      
      // this is how it was:    
      // m_serialport.waitForBytesWritten(500) ;
      // readData = m_serialport.readAll();
      // while (m_serialport.waitForReadyRead(m_timeout))
      //   readData.append(m_serialport.readAll());
      // and this is how I want to try it:
      /*bool success = */ m_serialport.waitForReadyRead(500) ;
      readData = m_serialport.readAll();
    }
    //qDebug() << "Read: \"" << readData << "\"" ; //<< std::endl ;
    return readData ;
  }
  
  
  /*
    QByteArray MotionSystemSvc::read() const
    {
    QByteArray readData ;
    if( m_serialport.isOpen() ) {
    // I now think that there is a latency problem between the
    // GPIB-USB converter and the motion system: the read command
    // comes too early. The default time-out is 500 ms, which you
    // would think is enough. We could perhaps try with a 'wait' call
    // right here:
    const int maxtries = 5 ;
    int i=0;
    int success = 0 ;
    for(i=0; i<maxtries; ++i) {
      QThread::msleep(50) ;
      m_serialport.write("++read eoi\n") ;
      // this is how it was:    
      // m_serialport.waitForBytesWritten(500) ;
      // readData = m_serialport.readAll();
      // while (m_serialport.waitForReadyRead(m_timeout))
      //   readData.append(m_serialport.readAll());
      // and this is how I want to try it:
      success = m_serialport.waitForReadyRead(1000) ;
      readData = m_serialport.readAll();
      if( success || readData.size() > 0 ) break ;
    }
    qDebug() << "Number of read cycles: " << i << " " << success<< " " << readData ;
    }    
    //qDebug() << "Read: \"" << readData << "\"" ; //<< std::endl ;
    return readData ;
    }
  */
  
  /*
    QByteArray MotionSystemSvc::writeAndRead(const char* command) const
    {
    QByteArray readData ;
    if( m_serialport.isOpen() ) {
    write(command) ;
    readData = m_serialport.readAll();
    while (m_serialport.waitForReadyRead(m_timeout))
    readData.append(m_serialport.readAll());
    }
    return readData ;
    }
  */
  
  void MotionSystemSvc::writeData(const QByteArray &data)
  {
    m_serialport.write(data) ;
  }
  
  void MotionSystemSvc::readData()
  {
    QByteArray data = m_serialport.readAll();
    if(m_console) m_console->putData(data);
    // now try to see what we can do with this. this is going to be a
    // lot of work: we need a way to parce lines in the input.
    qDebug() << "Received some data!" << data ;
    parseData( data ) ;
  }
  
  namespace {
    // here we can do some serious coding optimization
    int extractAxisFloat( const char* command, const QString& line, float& value)
    {
      int pos = line.indexOf(command) ;
      // assume everyting before it is the axis
      int axis  = line.leftRef(pos).toInt() ;
      value = line.rightRef(line.size() - (pos + 2) ).toFloat() ;
      return axis ;
    }
  }
  
  void MotionSystemSvc::parseData( const QByteArray& data )
  {
    // this all assumes that we receive data for the current controller.
    QStringList lines = QString(data).split(QRegExp("\n|\r\n|\r"));
    for( const auto& line : lines ) {
      if( line.startsWith("TE") || line.startsWith("TB") ) {
	QStringRef errorstring = line.rightRef(line.size() - 2 ) ;
	//QChar statusbyte = line[2] ;
	//qDebug() << "Reading a error string: " << errorstring  ;
	m_controllers[ m_currentcontrollerid ]->setError( std::string(errorstring.toLocal8Bit().constData()) ) ;
      } else if( line.contains("TS") ) {
	int pos = line.indexOf("TS") ;
	QChar errorbyte = line[pos+2] ;
	m_controllers[ m_currentcontrollerid ]->setStatus(errorbyte.toLatin1()) ;
	//qDebug() << "Reading an error string: " << errorbyte ;
      } else if( line.contains("TP") ) {
	// now we need some serious parsing. this needs to be improved:
	float position ;
	int axis = extractAxisFloat( "TP", line, position) ;
	m_axes[ MotionAxisID( m_currentcontrollerid, axis ) ]->position().setValue( position ) ; 
	//qDebug() << "Read position: " << line << " --> "
	//<< axis << " : " << position ;
      } 
    }
  }
  
  
  void MotionSystemSvc::configure( const MotionAxisID& /*id*/ ) const
  {
    qDebug() << "To be implemented!" ;
  }
  
  int MotionSystemSvc::statusFlag( int motioncontrollerid )
  {
    // We should also use "MS" which is the "motor status"
    write(motioncontrollerid,"TS") ;
    auto line = QString(read()) ;
    //if( line.contains("TS") ) {
    int pos = line.indexOf("TS") ;
    //QChar errorbyte = line[pos+2] ;
    int status = line[pos+2].toLatin1() ;
    m_controllers[ m_currentcontrollerid ]->setStatus(status) ;
    qDebug() << "Read status byte:" << line << " " << status ;
    return status ;
  }
  
  bool MotionSystemSvc::isMoving( const MotionAxisID& id )
  {
    // if the the system is moving, we don't want to issue new moving commands!
    int status = statusFlag(id.controller) ;
    bool ismoving = status & (1<<(id.axis-1)) ;
    return ismoving ;
  }
  
  void MotionSystemSvc::applyAxisCommand( const MotionAxisID& id,
					  const char* command,
					  double nn ) const
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s%f",id.axis,command,nn) ;
    write(id.controller,acommand) ;
  }
  
  void MotionSystemSvc::applyAxisCommand( const MotionAxisID& id,
					  const char* command,
					  int nn ) const
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s%d",id.axis,command,nn) ;
    write(id.controller,acommand) ;
  }
  
  void MotionSystemSvc::applyAxisCommand( const MotionAxisID& id,
					  const char* command,
					  const char* nn ) const
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s%s",id.axis,command,nn) ;
    write(id.controller,acommand) ;
  }
  
  void MotionSystemSvc::applyAxisCommand( const MotionAxisID& id,
					  const char* command ) const
  {
    char acommand[256] ;
    sprintf(acommand,"%d%s",id.axis,command) ;
    write(id.controller,acommand) ;
  }
  
  double MotionSystemSvc::readAxisFloat( const MotionAxisID& id,
					 const char* command ) const
  {
    applyAxisCommand(id,command) ;
    auto line = QString(read()) ;
    int pos = line.indexOf(command) ;
    // assume everyting before it is the axis
    int axis  = line.leftRef(pos).toInt() ;
    if( axis != id.axis ) 
      // need to throw some sort of error ...
      qDebug() << "Problem reading value for axis: " << id.axis << " " << axis ;
    return line.rightRef(line.size() - (pos + 2) ).toFloat() ;
  }
  
  void MotionSystemSvc::readAxisVariable(  const MotionAxisID& id,
					   const char* command,
					   NamedValue& var) const
  {
    applyAxisCommand(id,command) ;
    auto line = QString(read()) ;
    int pos = line.indexOf(command) ;
    // assume everyting before it is the axis
    int axis  = line.leftRef(pos).toInt() ;
    if( axis != id.axis ) {
      // need to throw some sort of error ...
      qDebug() << "Problem reading value. Axis number returned is different from sent: " << id.axis << " " << axis ;
    } else {
      int endpos = line.indexOf("\n") ;
      if(endpos==-1) endpos = line.size() ;
      QStringRef valuestring{&line,pos+2,endpos-(pos+2)} ;
      qDebug() << "line,valuestring: " << line << valuestring ;
      //auto valuestring = line.rightRef(endpos - (pos + 2) ) ;
      //valuestring.remove("\n") ;
      //valuestring.remove(" ") ;
      if(      var.type() == QVariant::Double )
	var.setValue(valuestring.toDouble()) ;
      else if( var.type() == QVariant::Int )
	var.setValue(valuestring.toInt()) ;
      else if( var.type() == QVariant::String )
	var.setValue(valuestring.toString()) ;
    }
  }
  
  float MotionSystemSvc::position( const MotionAxisID& id ) const
  {
    char acommand[256] ;
    sprintf(acommand,"%02dTP",id.axis) ;
    write( id.controller,acommand) ;
    auto data = read() ;
    // now we need some serious parsing.
    QString line(data) ;
    int pos = line.indexOf("TP") ;
    // assume everyting before it is the axis
    //int axis  = line.leftRef(pos).toInt() ;
    float position = line.rightRef(line.size() - (pos + 2) ).toFloat() ;
    //qDebug() << "Where I think the float starts: \"" << line.rightRef(line.size() - (pos + 2)  ) << "\"" ;
    //qDebug() << "Position: " << pos << " : " << axis << " : " << position  ;
    return position ;
    //return data.toFloat() ; //atof(data) ;
  }
  
  void MotionSystemSvc::switchMotorsOn( int controllerid, bool on ) const
  {
    qDebug() << "Switching motors on for controller " << controllerid << " " << on ;
    write( controllerid, on ? "MO" : "MF") ;
  }
  
  /* THIS DOES NOT WORK: the controller does not buffer, it seems. On read it only sends the data from the last request. */
  void MotionSystemSvc::updateAllData()
  {
    qDebug() << "updateAllData is called!" ;
    // very tricky. let's see where this goes then fix it.
    std::vector<int> controllers = {2,4} ;
    for(int controllerid : controllers ) {
      write(controllerid,"TS") ;
      parseData( read() ) ;
      write(controllerid,"TB") ;
      parseData( read() ) ;
    }
  }
}
  
