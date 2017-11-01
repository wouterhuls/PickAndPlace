#ifndef MOTIONSYSTEMSVC_H
#define MOTIONSYSTEMSVC_H

//#include <QObject>

#include "Singleton.h"
#include "MotionAxis.h"
#include "MotionController.h"
#include "MotionSystemSerialPort.h"

class Console ;

// singleton class for interfacing to the motion system This needs to
// be reimplemented because it is a mess: it now is both the container
// of the axis and the object that talks to the controllers. The
// interface to talk to the controllers should not be visible to
// clients from the 'service'.

namespace PAP
{

  class MotionSystemSvc : public QObject
  {
    Q_OBJECT
    
  public:
    typedef std::map<MotionAxisID,MotionAxis*> AxisContainer ;
    typedef std::map<MotionControllerID,MotionController*> ControllerContainer ;
    // enumerate type with identifiers for all axes
    enum { MainX, MainY, StackX, StackY, StackPhi, Focus } MotionAxisName ;

  public:

    MotionSystemSvc() ;
    virtual ~MotionSystemSvc() ;
    static MotionSystemSvc* instance() ;

    // this should all go into a new class "MotionSystem"
    // we should replace MotionAxisID by MotionAxisAddress
    void configure(const MotionAxisID& id ) const ;
    bool isMoving( const MotionAxisID& id ) ;
    // movements
    void applyAxisCommand( const MotionAxisID& id, const char* command, double nn ) const ;
    void applyAxisCommand( const MotionAxisID& id, const char* command, int nn ) const ;
    void applyAxisCommand( const MotionAxisID& id, const char* command, const char* nn ) const ;
    void applyAxisCommand( const MotionAxisID& id, const char* command ) const ;
    void applyAxisReadCommand( const MotionAxisID& id, const char* command ) ;
    void readAxisVariable( const MotionAxisID& id, const char* command, NamedValueBase& ) const ;
    
    float position( const MotionAxisID& id ) const ;
    std::string name( const MotionAxisID& id ) const ;

    //const AxisContainer& axes() const { return m_axes ; }
    AxisContainer& axes() { return m_axes ; }
    ControllerContainer& controllers() { return m_controllers; }

    // switch all motors on or off
    void switchMotorsOn(MotionControllerID id, bool on = true) const ;
    // tell if the serial port is ready and the system connected
    bool isReady() const { return m_serialport->isOpen() && m_isReady ; }
    
    void setConsole( Console* console ) { m_console = console ; }
    
    MotionAxis* axis( const QString& name ) ;

    void parseData( int controllerid, const QByteArray& data ) ;

  private:
      void write( int motioncontrollerid, const char* command ) const ;
      void write( const char* command ) const ;
      QByteArray read() const ;
      
      //private slots:
      // this is new for asynchronous read/write
      /*
	void writeData( const QByteArray& data ) ;
	void readData() ;
	void parseData( const QByteArray& data ) ;
      */
      
  private:
      static MotionSystemSvc* gInstance ;
      MotionSystemSerialPort* m_serialport ;
      // we should actually separate this, but let's keep it here for now
      AxisContainer m_axes ;
      ControllerContainer m_controllers ;
      
      // Console for monitoring in and output.
      Console *m_console ;
      bool m_isReady ;
  };
}

#endif // MOTIONSYSTEMSVC_H
