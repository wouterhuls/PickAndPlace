#include "MotionSystemSerialPort.h"
#include "MotionSystemSvc.h"

#include <QRegularExpression>
  
namespace PAP
{
  
  /* implementation of the worker */
  /*namespace {*/

    /*
    class Worker : public QObject
    {
      Q_OBJECT
    public:
      Worker( QSerialPort& port,
	      int sleeptimeaddresschange = 100,
	      int sleeptimereadcommand = 50 ) :
	m_serialport(&port),
	m_currentcontrollerid(-1),
	m_sleeptimeaddresschange( sleeptimeaddresschange ),
	m_sleeptimereadcommand( sleeptimereadcommand ) {}
      virtual ~Worker() ;
      
    signals:
      void resultReady(const MSResult& result) ;
      void ready() ;					 
    public slots:
      void doWork( const MSCommand& command ) {
	write( command.controller, command.cmd.c_str() ) ;
	if( command.isreadcommand ) {
	  MSResult result ;
	  result.controller = command.controller ;
	  result.data = read() ;
	  emit resultReady(result) ;
	}
	emit ready() ;
      }
      
    private:
      void write( int motioncontrollerid, const char* command ) ;
      void write( const char* command ) ;
      QByteArray read() ;
      
    private:
      QSerialPort* m_serialport ;
      int m_currentcontrollerid ;
      int m_sleeptimeaddresschange ;
      int m_sleeptimereadcommand ;
    } ;

    */

  MSWorker::~MSWorker() {}

  void MSWorker::doWork( const MSCommand& command ) {
    write( command.controller, command.cmd.c_str() ) ;
    if( command.isreadcommand ) {
      MSResult result ;
      result.controller = command.controller ;
      result.data = read() ;
      emit resultReady(result) ;
    }
    emit ready() ;
  }
  
  void MSWorker::write( int motioncontrollerid, const char* command )
  {
    if( m_serialport->isOpen() ) {
      if( m_currentcontrollerid != motioncontrollerid ) {
	char controlleridcommand[256] ;
	sprintf(controlleridcommand,"++addr %d",motioncontrollerid) ;
	write(controlleridcommand) ;
	m_currentcontrollerid = motioncontrollerid ;
	QThread::msleep(m_sleeptimeaddresschange) ;
      }
      write(command) ;
    }
  }
  
  void MSWorker::write(const char* command)
  {
    qDebug() << "Writing command: \"" << command << "\"" ;
    if( m_serialport->isOpen() ) {
      m_serialport->write(command) ;
      m_serialport->write("\n") ;
      m_serialport->waitForBytesWritten(1000) ;
    } ;
  }
  
  QByteArray MSWorker::read()
  {
    QByteArray readData ;
    if( m_serialport->isOpen() ) {
      // I now think that there is a latency problem between the
      // GPIB-USB converter and the motion system: the read command
      // comes too early. The default time-out is 500 ms, which you
      // would think is enough. We could perhaps try with a 'wait' call
      // right here:
      QThread::msleep(m_sleeptimereadcommand) ;
      m_serialport->write("++read eoi\n") ;
      
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
      /*bool success = */ m_serialport->waitForReadyRead(200) ;
      readData = m_serialport->readAll();
    }
    //qDebug() << "Read: \"" << readData << "\"" ; //<< std::endl ;
    return readData ;
  }
    
  MotionSystemSerialPort::MotionSystemSerialPort( MotionSystemSvc& parent,
						  const QSerialPortInfo &info)
    : m_parent(&parent), m_serialport(info)
  {
    if( m_serialport.open(QIODevice::ReadWrite) ) {
      qInfo() << "MotionSystemSvs: Successfully opened port"
	      << m_serialport.baudRate() << " "
	      << m_serialport.errorString() << " "
	      << m_serialport.flowControl() << " "
	      << m_serialport.parity() ;
      
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
      qInfo() << "Version of GPIB-USB interface: " << readData ;

      // from now on only the worker can communicate to the serialport!
      MSWorker* worker = new MSWorker(m_serialport) ;
      worker->moveToThread(&m_workerthread) ;
      // make sure that it is deleted, at some point
      connect( &m_workerthread, &QThread::finished, worker, &QObject::deleteLater) ;
      // make the connection to give tasks to the worker
      //connect( this, &MotionSystemSerialPort::operate, worker, &Worker::doWork ) ;
      connect( this, SIGNAL(MotionSystemSerialPort::operate()), worker, SLOT(MSWorker::doWork()) ) ;
      // make the connection to let worker tell us that it is done
      //connect( worker, &Worker::ready, this, &MotionSystemSerialPort::next) ;
      connect( worker, SIGNAL(MSWorker::ready()), this, SLOT(MotionSystemSerialPort::next())) ;
      // make the connection to get result from worker
      //connect( worker, &Worker::resultReady, this, &MotionSystemSerialPort::handleOutput) ;
      connect( worker, SIGNAL(MSWorker::resultReady()), this, SLOT(MotionSystemSerialPort::handleOutput())) ;

      // start the thread
      m_workerthread.setPriority(QThread::LowPriority) ;
      m_workerthread.start() ;
      
      // to get the circus going, we now need to start the loop
      next() ;
      
    } else {
      qWarning() << "Could not open serial port" ;
    }
  }

  void MotionSystemSerialPort::addCommand( int controller, const char* cmd, bool isreadcommand,
					   NamedValueBase* target)
  {
    // first add it to the queue
    m_commandqueue.push_back( MSCommand(controller,cmd,isreadcommand,target) ) ;
    // should we now emit a signal to the worker? perhaps better not?
    // I don't have a clue ... perhaps my design is still wrong.
  }

  void MotionSystemSerialPort::next()
  {
    // if the queu is empty, then request just status and errors
    // FIXME: make sure it orders these such that we don't need to change controller id more than once. 
    if( m_commandqueue.empty() ) {
      const int c1 = m_lastcommand.controller==4 ? 4 : 2 ;
      const int c2 = m_lastcommand.controller==4 ? 2 : 4 ;
      addCommand(c1,"TS",true) ;
      addCommand(c1,"TB",true) ;
      addCommand(c2,"TS",true) ;
      addCommand(c2,"TB",true) ;
    }
    m_lastcommand = m_commandqueue.front() ;
    m_commandqueue.pop_front() ;
    emit operate( m_lastcommand ) ;
  }

  void MotionSystemSerialPort::handleOutput(const MSResult& result)
  {
    // it would be very good to check that the output that we
    // receive actually corresponds to the last issued command!
    qDebug() << "Test handleOutput: " << m_lastcommand.cmd.c_str() << " " << result.data ;
    if( m_lastcommand.target==0 )
      m_parent->parseData( result.controller, result.data ) ;
    else {
      // this code should be moved to MotionSystemSvc.
      // check that the command is actually part of the result!
      QString line{result.data} ;
      if( line.contains( m_lastcommand.cmd.c_str() ) ) {
	QRegularExpression re{"^(\\d*)(\\w\\w)(.+)"} ;
	QRegularExpressionMatch match = re.match( line ) ;
	if( match.hasMatch() ) {
	  m_lastcommand.target->fromString( match.captured(3) ) ;
	} else {
	  qWarning() << "MotionSystemSerialPort cannot parse string!" ;
	}
      } else {
	qWarning() << "MotionSystemSerialPort command and result do not match: "
		   << m_lastcommand.cmd.c_str()
		   << result.data ;
      }
    }
  }
  
}
