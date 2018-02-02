#ifndef MOTIONAXIS_H
#define MOTIONAXIS_H

#include <vector>
#include <QString>
#include "MotionAxisParameter.h"
#include "MotionSystemTypes.h"

namespace PAP
{

  class MotionController ;
  
  class MotionAxis : public QObject
  {
    
    Q_OBJECT
    
  public:
    //using MSParameter = PAP::MSParameter ;
    enum Direction { Up=+1, Down=-1 } ;
    using MotionAxisParameters = std::vector<MotionAxisParameter*>  ;
    MotionAxis( const MotionAxisID& id, const QString& name, const QString& type,
		const MotionController& c) ;
    void configure() ;
    void searchHome() ;
    void defineHome() ;
    void setZero() ;
    void step( Direction dir ) ;   // move default stepsize in the direction dir
    void move( Direction dir ) ;   // move until stoppped
    void move( double delta ) ;     // move by delta
    void moveTo( double position ) ;
    void stop() ;
    //double position() const { return m_position ; }
    //void setPosition( double p ) { m_position = p ; }
    const QString name() const { return m_name ; }
    const MotionAxisID& id() const { return m_id ; }
    const MotionController& controller() const { return *m_controller ; }
    void setController( const MotionController* c) { m_controller = c ; }
    bool isMoving() const { return m_isMoving ; }
    void setIsMoving( bool ismoving ) ;
    bool hasMotorOn() const ;
    bool parseData( const QString& cmd, const QString& value ) ;
    
    const MotionAxisParameters& parameters() const { return m_parameters ; }
    MotionAxisParameters& parameters() { return m_parameters ; }
    
    const NamedDouble& position() const { return m_position ; }
    NamedDouble& position() { return m_position ; }
    const NamedDouble& stepsize() const { return m_stepsize ; }
    NamedDouble& stepsize() { return m_stepsize ; }
    
    double leftTravelLimit() const { return m_leftTravelLimit->getValue().value().toDouble() ; }
    double rightTravelLimit() const { return m_rightTravelLimit->getValue().value().toDouble() ; }
    
    void readParameters() ;
    void setAllowPassTravelLimit( bool a ) { m_allowPassTravelLimit = a ; }
    bool allowPassTravelLimit() const { return m_allowPassTravelLimit ; }
    
    signals:
      void movementStarted() ;
      void movementStopped() ;
      
    public slots:
      //void setDefaultVelocity( double v ) const ;
      // void handleParameterUpdate() const ;
      //void writeParameter( const MSParameter& par ) const ;
      //void readParameter( MSParameter& par ) ;
      void applyPosition() { moveTo( m_position ) ; }
      void readPosition() ;
      
  private:
      double updatePosition() ;
  private:
      MotionAxisID m_id ;
      QString m_name ;
      QString m_type ;
      // measurements, axis state
      bool m_isMoving ;
      unsigned int m_status ;
      NamedDouble m_position ;
      // parameters of the axis
      MotionAxisParameters m_parameters ;
      MotionAxisParameter* m_leftTravelLimit ;
      MotionAxisParameter* m_rightTravelLimit ;
      // internal parameters
      NamedDouble m_stepsize ;
      bool m_allowPassTravelLimit ;
      // parent controller
      const MotionController* m_controller ;
  };
}

#endif // MOTIONAXIS_H
