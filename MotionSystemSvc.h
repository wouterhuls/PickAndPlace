#ifndef MOTIONSYSTEMSVC_H
#define MOTIONSYSTEMSVC_H

//#include <QObject>

#include "Singleton.h"
#include "MotionAxis.h"
#include "MotionController.h"
#include "MotionSystemSerialPort.h"
#include "Coordinates.h"

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
    void readAxisVariable( const MotionAxisID& id, const char* command, MonitoredValueBase& ) const ;

    void applyControllerCommand( const MotionControllerID&, const char* command ) const ;

    
    double position( const MotionAxisID& id ) const ;
    std::string name( const MotionAxisID& id ) const ;

    //const AxisContainer& axes() const { return m_axes ; }
    AxisContainer& axes() { return m_axes ; }
    ControllerContainer& controllers() { return m_controllers; }

    // issue a 'ST' to both controllers. alternative is to turn both
    // motors off.
    void emergencyStop() const ;
    // tell if the serial port is ready and the system connected
    bool isReady() const { return m_serialport->isOpen() && m_isReady ; }
    
    // finally decided just to have direct access to axes
    MotionAxis& mainXAxis() const { return *m_mainXAxis ; }
    MotionAxis& mainYAxis() const { return *m_mainYAxis ; }
    MotionAxis& focusAxis() const { return *m_focusAxis ; }    
    MotionAxis& stackXAxis() const { return *m_stackXAxis ; }
    MotionAxis& stackYAxis() const { return *m_stackYAxis ; }
    MotionAxis& stackRAxis() const { return *m_stackRAxis ; }    
    
    void parseData( int controllerid, const QByteArray& data, const QDateTime& timestamp ) ;

    // return the coordinates of all axis
    MSCoordinates coordinates() const ;
    MSMainCoordinates maincoordinates() const ;
    MSStackCoordinates stackcoordinates() const ;
    
  signals:
    void mainStageMoved() const ;
    void mainStageStopped() const ;
    void stackStageMoved() const ;
    
  private: 
    void write( int motioncontrollerid, const char* command ) const ;
    void write( const char* command ) const ;
    QByteArray read() const ;

    // this one is obsolete
    MotionAxis* axis( const QString& name ) ;
	
  private:
      static MotionSystemSvc* gInstance ;
      MotionSystemSerialPort* m_serialport ;
      // we should actually separate this, but let's keep it here for now
      AxisContainer m_axes ;
      MotionAxis* m_mainXAxis ;
      MotionAxis* m_mainYAxis ;
      MotionAxis* m_focusAxis ;
      MotionAxis* m_stackXAxis ;
      MotionAxis* m_stackYAxis ;
      MotionAxis* m_stackRAxis ;      
      ControllerContainer m_controllers ;
      bool m_isReady ;
  } ;
}

#endif // MOTIONSYSTEMSVC_H
