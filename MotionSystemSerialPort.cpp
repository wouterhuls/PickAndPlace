#include "MotionSystemSerialPort.h"
#include "MotionSystemSvc.h"

#include <QRegularExpression>
#include <algorithm>

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
  MSWorker::MSWorker( const QSerialPortInfo& portinfo) :
    m_serialport(portinfo),
    m_currentcontrollerid(-1),
    m_sleeptimeaddresschange("MotionSystem.SleepTimeAddressChange",200),
    m_sleeptimereadcommand("MotionSystem.SleepTimeReadCommand",200)
  {
    // let;s open it in this thread and not in the other one.
    m_serialport.setParent(this) ;
    if( m_serialport.open(QIODevice::ReadWrite) ) {
      qInfo() << "MotionSystemSvs: Successfully opened port"
	      << m_serialport.baudRate() << " "
	      << m_serialport.errorString() << " "
	      << m_serialport.flowControl() << " "
	      << m_serialport.parity() ;
      m_serialport.write("++auto 0\n") ;
      m_serialport.write("++ver\n") ;
      // here we have a problem with the time-out.
      // perhaps that's a configuration issue, or something inside QSerialIO.
      // (there is no problem when communicating from the command line.)
      QByteArray readData = m_serialport.readAll();
      while (m_serialport.waitForReadyRead(100))
	readData.append(m_serialport.readAll());
      qInfo() << "Version of GPIB-USB interface: " << readData ;
    }
  }
  
  MSWorker::~MSWorker() {}

  void MSWorker::doWork( const MSCommand& command ) {
    if( !command.isreadcommand)
      qDebug() << "MSWorker::doWork: " << command.cmd.c_str() ;
    if( m_serialport.isOpen() ) {
      write( command.controller, command.cmd.c_str() ) ;
      if( command.isreadcommand ) {
	MSResult result ;
	result.controller = command.controller ;
	result.data = read() ;
	
	QString line{result.data} ;
	if( !line.contains( command.cmd.c_str() ) ) {
	  // very often we are just one read behind. let's just read once more.
	  auto newdata = read() ;
	  qWarning() << "Test in handleOuput failed already in worker! We'll try to read again."
		     << command.cmd.c_str()
	     	     << result.data << newdata ;
	  
	  if( QString{newdata}.contains( command.cmd.c_str() ) ) {
	    result.data = newdata ;
	  } else {
	    // concatenate the two lines, which often seems to be the problem too sometimes solves the problem as well.
	    result.data.append(newdata) ;
	    if( !QString{result.data}.contains( command.cmd.c_str() ) ) {
	      qWarning() << "Test in handleOuput still failed in worker" << result.data ;
	    }
	  }
	}
	//qDebug() << "Result ready! "
	//<< command.cmd.c_str()
	//	       << result.data.size() ;
	result.timestamp = QDateTime::currentDateTime() ;
	emit resultReady(result) ;
      } else {
	emit ready() ;
      }
    }
  }
  
  void MSWorker::write( int motioncontrollerid, const char* command )
  {
    if( m_serialport.isOpen() ) {
      if( m_currentcontrollerid != motioncontrollerid ) {
	//qDebug() << "MSWorker::write: " << motioncontrollerid << command ;
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
    //qDebug() << "Writing command: \"" << command << "\"" ;
    if( m_serialport.isOpen() ) {
      m_serialport.write(command) ;
      m_serialport.write("\n") ;
      m_serialport.waitForBytesWritten(1000) ;
      //QThread::msleep(m_sleeptimeaddresschange) ;
    } ;
  }
  
  QByteArray MSWorker::read()
  {
    QByteArray readData ;
    if( m_serialport.isOpen() ) {
      // I now think that there is a latency problem between the
      // GPIB-USB converter and the motion system: the read command
      // comes too early. The default time-out is 500 ms, which you
      // would think is enough. We could perhaps try with a 'wait' call
      // right here:
      QThread::msleep(m_sleeptimereadcommand) ;
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
      bool success = m_serialport.waitForReadyRead(2000) ;
      readData = m_serialport.readAll();
      if(!success)
	qWarning() << "WaitForReadyRead timed out!" ;
      //qDebug() << "Size was in read(): " << readData.size() ;
      // FIXME! so, I don't know what is wrong, but we are receiving garbage.
      // for now, truncate at the first newline.
      //int newend = readData.indexOf("\n") ;
      //readData.truncate(newend) ;
    }
    //qDebug() << "Read: \"" << readData << "\"" ; //<< std::endl ;
    return readData ;
  }
    
  MotionSystemSerialPort::MotionSystemSerialPort( MotionSystemSvc& parent,
						  const QSerialPortInfo &portinfo)
    : m_parent(&parent)
  {
    // register types used for signals
    qRegisterMetaType<PAP::MSCommand>() ;
    qRegisterMetaType<PAP::MSResult>() ;
    // from now on only the worker can communicate to the serialport!
    MSWorker* worker = new MSWorker(portinfo) ;
    worker->moveToThread(&m_workerthread) ;
    // make sure that it is deleted, at some point
    connect( &m_workerthread, &QThread::finished, worker, &QObject::deleteLater) ;
    // make the connection to give tasks to the worker
    //connect( this, &MotionSystemSerialPort::operate, worker, &Worker::doWork ) ;
    connect( this, &MotionSystemSerialPort::operate, worker, &MSWorker::doWork ) ;
    // make the connection to let worker tell us that it is done
    //connect( worker, &Worker::ready, this, &MotionSystemSerialPort::next) ;
    connect( worker, &MSWorker::ready, this, &MotionSystemSerialPort::next ) ;
    // make the connection to get result from worker
    //connect( worker, &Worker::resultReady, this, &MotionSystemSerialPort::handleOutput) ;
    connect( worker, &MSWorker::resultReady, this, &MotionSystemSerialPort::handleOutput) ;
    
    // start the thread
    m_workerthread.setObjectName("MotionSystemSerialPort") ;
    m_workerthread.start() ;
    //m_workerthread.setPriority(QThread::LowPriority) ;
    
    // add commands to start controllers in local mode
    // FIXME: move this to parent
    addCommand(2,"ML") ;
    addCommand(4,"ML") ;

    // create a list of commands that we'll run when the queue is empty
    m_idlecommands = { MSCommand{2,"TS",true},
		       MSCommand{2,"TB",true},
		       MSCommand{4,"TS",true},
		       MSCommand{4,"TB",true} } ;
    m_currentidlecommandindex=0 ;
    
    // to get the circus going, we now need to start the loop
    next() ;
  }

  void MotionSystemSerialPort::addCommand( int controller, const char* cmd, bool isreadcommand,
					   MonitoredValueBase* target)
  {
    // first add it to the queue. we would actually like to sort by
    // priority here: write commands before readcommands.
    MSCommand mscmd{controller,cmd,isreadcommand,target} ;
    // the fork is just to make it faster ...
    if( isreadcommand ) {
      m_commandqueue.push_back( mscmd ) ;
    } else {
      auto it = std::upper_bound( m_commandqueue.begin(),
				  m_commandqueue.end(), mscmd ) ;
      m_commandqueue.insert(it,mscmd) ;
      qDebug() << "Inserting write command: " << cmd << m_workerthread.isRunning() << m_commandqueue.size() ;
    }
    // should we now emit a signal to the worker? perhaps better not?
    // I don't have a clue ... perhaps my design is still wrong.
  }
  
  void MotionSystemSerialPort::next()
  {
    //qDebug() << "MotionSystemSerialPort::next()" << ++m_eventindex ;
    // if the queu is empty, then request just status and errors
    // FIXME: make sure it orders these such that we don't need to change controller id more than once.
    if( m_commandqueue.empty() ) {
      m_commandqueue.push_back( m_idlecommands[ m_currentidlecommandindex ] ) ;
      m_currentidlecommandindex = (m_currentidlecommandindex+1)%m_idlecommands.size() ;
      //   const int c1 = m_lastcommand.controller==4 ? 4 : 2 ;
      //   const int c2 = m_lastcommand.controller==4 ? 2 : 4 ;
      //   addCommand(c1,"TS",true) ;
      //   addCommand(c1,"TB",true) ;
      //   addCommand(c2,"TS",true) ;
      //   addCommand(c2,"TB",true) ;
    }
    m_lastcommand = m_commandqueue.front() ;
    m_commandqueue.pop_front() ;
    //qDebug() << "MotionSystemSerialPort::next()" << m_lastcommand.cmd.c_str() ;
    emit operate( m_lastcommand ) ;
  }

  void MotionSystemSerialPort::handleOutput(const MSResult& result)
  {
    // it would be very good to check that the output that we
    // receive actually corresponds to the last issued command!
    QString line{result.data} ;

    // FIXME. for now very simple. Later use the pattern match:
    if( !line.contains( m_lastcommand.cmd.c_str() ) ) {
      qWarning() << "Test in handleOuput failed!" 
		 << m_lastcommand.cmd.c_str()
		 << result.data ;
      // just try again?
      m_commandqueue.push_back( m_lastcommand ) ;
    } else {
      if( m_lastcommand.target==0 )
	m_parent->parseData( result.controller, result.data, result.timestamp ) ;
      else {
	// this code should be moved to MotionSystemSvc.
	// check that the command is actually part of the result!
	if( line.contains( m_lastcommand.cmd.c_str() ) ) {
	  QRegularExpression re{"^(\\d*)(\\w\\w)(.+)"} ;
	  QRegularExpressionMatch match = re.match( line ) ;
	  if( match.hasMatch() ) {
	    m_lastcommand.target->fromString( match.captured(3), result.timestamp ) ;
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
    next() ;
  }
  
}
