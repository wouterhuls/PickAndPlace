#ifndef MOTIONSYSTEMSERIALPORT_H
#define MOTIONSYSTEMSERIALPORT_H

#include <QSerialPort>
#include <QThread>
#include <deque>
#include "NamedValue.h"


class QSerialPortInfo ;
class Console ;

namespace PAP
{
  class MotionSystemSvc ;
  class MonitoredValueBase ;
  
  class MSCommand
  {
  public:
  MSCommand() : controller(0),isreadcommand(false), target(0) {}
  ~MSCommand() {}
  
  template<class T>
    MSCommand( int c, const T& acmd, bool _isreadcommand, MonitoredValueBase* _target=0 ) :
    controller(c), cmd(acmd), isreadcommand(_isreadcommand), target(_target) {}
  int controller ;
  std::string cmd ;
  bool isreadcommand ;
  MonitoredValueBase* target ;
  // sort command used for inserting in queue. write commands get priority over read commands.
  bool operator<( const MSCommand& rhs ) const { return !this->isreadcommand && rhs.isreadcommand ; }
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
    MSWorker( const QSerialPortInfo& port ) ;
    virtual ~MSWorker() ;
    
  signals:
    void resultReady(const PAP::MSResult& result) ;
    void ready() ;					 
  public slots:
    void doWork( const PAP::MSCommand& command ) ;
    //void resetport() ;
  private:
    void write( int motioncontrollerid, const char* command ) ;
    void write( const char* command ) ;
    QByteArray read() ;
    
  private:
    QSerialPort m_serialport ;
    int m_currentcontrollerid ;
    NamedValue<int> m_sleeptimeaddresschange ;
    NamedValue<int> m_sleeptimereadcommand ;
    // Console for monitoring in and output.
    Console *m_console ;
  } ;
  
  class MotionSystemSerialPort : public QObject
  {
    Q_OBJECT
  public:
    MotionSystemSerialPort( MotionSystemSvc& parent,
			    const QSerialPortInfo &info) ;
    // This is how the motionsystemsvc adds commands to the queue
    void addCommand( int controller, const char* cmd, bool isreadcommand=false, MonitoredValueBase* target=0 ) ;
    // check that we are actually ready
    bool isOpen() const { return true; } //m_serialport.isOpen() ; }

  signals:
    void operate(const PAP::MSCommand& command) ; // send command to worker
  public slots:
    void next() ; // to be called by worker to notify that it is ready for next command
    void handleOutput(const PAP::MSResult& result) ; // to be called by worker to provide results of read commands

  private:
    MotionSystemSvc* m_parent ;
    std::deque<MSCommand> m_commandqueue ;
    QThread m_workerthread ;
    MSCommand m_lastcommand ;
    std::vector<MSCommand> m_idlecommands ;
    unsigned short m_currentidlecommandindex ;
  } ;

}

Q_DECLARE_METATYPE(PAP::MSCommand) ;
Q_DECLARE_METATYPE(PAP::MSResult) ;

#endif
