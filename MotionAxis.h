#ifndef MOTIONAXIS_H
#define MOTIONAXIS_H

#include <string>
#include "MotionAxisParameters.h"

namespace PAP
{

  class MotionController ;
  
  class MotionAxis : public QObject
  {
    
    Q_OBJECT
    
  public:
    using MSParameter = PAP::MSParameter ;
    using MotionAxisParameters = std::vector<MSParameter>  ;
    MotionAxis( const MotionAxisID& id, const QString& name, const QString& type,
		const MotionController& c) ;
    void configure() ;
    void searchHome() ;
    void step( Direction dir ) ;   // move default stepsize in the direction dir
    void move( Direction dir ) ;   // move until stoppped
    void move( float delta ) ;     // move by delta
    void moveTo( float position ) ;
    void stop() ;
    //float position() const { return m_position ; }
    //void setPosition( float p ) { m_position = p ; }
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

    void readParameters() ;

    signals:
      void movementStarted() ;
      void movementStopped() ;
      
    public slots:
      //void setDefaultVelocity( float v ) const ;
      void handleParameterUpdate() const ;
      void writeParameter( const MSParameter& par ) const ;
      void readParameter( MSParameter& par ) ;
      void applyPosition() { moveTo( m_position ) ; }
      void readPosition() ;
      
  private:
      float updatePosition() ;
  private:
      MotionAxisID m_id ;
      QString m_name ;
      QString m_type ;
      // measurements from the machine
      NamedDouble m_position ;
      NamedDouble m_stepsize ;
      MotionAxisParameters m_parameters ;
      bool m_isMoving ;
      unsigned int m_status ;
      const MotionController* m_controller ;
  };
}

#endif // MOTIONAXIS_H
