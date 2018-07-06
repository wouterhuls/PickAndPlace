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
    Direction direction() const { return m_direction ; }
    
    const MotionAxisParameters& parameters() const { return m_parameters ; }
    MotionAxisParameters& parameters() { return m_parameters ; }
    MotionAxisParameter* parameter(const QString& name) ;
    
    const NamedDouble& position() const { return m_position ; }
    NamedDouble& position() { return m_position ; }
    const NamedDouble& setPosition() const { return m_setPosition ; }
    NamedDouble& setPosition() { return m_setPosition ; }
    const NamedDouble& tolerance() const { return m_tolerance ; }
    NamedDouble& tolerance() { return m_tolerance ; }
    const NamedDouble& stepsize() const { return m_stepsize ; }
    NamedDouble& stepsize() { return m_stepsize ; }
    const NamedDouble& antiHysteresisStep() const { return m_antihysteresisstep ; }
    NamedDouble& antiHysteresisStep() { return m_antihysteresisstep ; }
    
    double leftTravelLimit() const { return m_leftTravelLimit->getValue().value().toDouble() ; }
    double rightTravelLimit() const { return m_rightTravelLimit->getValue().value().toDouble() ; }
    
    void readParameters() ;
    void setAllowPassTravelLimit( bool a ) { m_allowPassTravelLimit = a ; }
    bool allowPassTravelLimit() const { return m_allowPassTravelLimit ; }

    void applyAntiHysteresisStep() ;
    
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
      void updateDirection() ;
  private:
      MotionAxisID m_id ;
      QString m_name ;
      QString m_type ;
      // measurements, axis state
      bool m_isMoving{false} ;
      unsigned int m_status{0} ;
      NamedDouble m_position ;
      NamedDouble m_setPosition ;
      NamedDouble m_tolerance ;
      double m_prevPosition{0} ;
      Direction m_direction{Direction::Down} ;
      // parameters of the axis
      MotionAxisParameters m_parameters ;
      MotionAxisParameter* m_leftTravelLimit{0} ;
      MotionAxisParameter* m_rightTravelLimit{0} ;
      // internal parameters
      NamedDouble m_stepsize ;
      NamedDouble m_antihysteresisstep ;
      bool m_allowPassTravelLimit{false} ;
      // parent controller
      const MotionController* m_controller{0} ;
  };
}

#endif // MOTIONAXIS_H
