#include <QSerialPort>
#include <QThread>
#include <deque>

class QSerialPortInfo ;

namespace PAP
{
  class MotionSystemSvc ;
  
  struct MSCommand
  {
    template<class T>
    MSCommand( int c, const T& acmd, bool _isreadcommand ) :
    controller(c), cmd(acmd), isreadcommand(_isreadcommand) {}
    int controller ;
    std::string cmd ;
    bool isreadcommand ;
  } ;

  struct MSResult
  {
    int controller ;
    QByteArray data ;
  } ;

  class MSWorker : public QObject
  {
    Q_OBJECT
  public:
    MSWorker( QSerialPort& port,
	    int sleeptimeaddresschange = 100,
	    int sleeptimereadcommand = 50 ) :
    m_serialport(&port),
      m_currentcontrollerid(-1),
      m_sleeptimeaddresschange( sleeptimeaddresschange ),
      m_sleeptimereadcommand( sleeptimereadcommand ) {}
    virtual ~MSWorker() ;
    
  signals:
    void resultReady(const MSResult& result) ;
    void ready() ;					 
  public slots:
    void doWork( const MSCommand& command ) ;
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
  
  class MotionSystemSerialPort : public QObject
  {
    Q_OBJECT
  public:
    MotionSystemSerialPort( MotionSystemSvc& parent,
			    const QSerialPortInfo &info) ;
    // This is how the motionsystemsvc adds commands to the queue
    void addCommand( int controller, const char* cmd, bool isreadcommand ) ;
    // check that we are actually ready
    bool isOpen() const { return m_serialport.isOpen() ; }

  signals:
    void operate(const MSCommand& command) ; // send command to worker
  public slots:
    void next() ; // to be called by worker to notify that it is ready for next command
    void handleOutput(const MSResult& result) ; // to be called by worker to provide results of read commands

  private:
    MotionSystemSvc* m_parent ;
    QSerialPort m_serialport ;
    std::deque<MSCommand> m_commandqueue ;
    QThread m_workerthread ;
  } ;

}
