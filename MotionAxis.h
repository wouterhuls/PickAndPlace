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
    void configure() const ;
    void searchHome() const ;
    void step( Direction dir ) const ;   // move default stepsize in the direction dir
    void move( Direction dir ) const ;   // move until stoppped
    void move( float delta ) const ;     // move by delta
    void moveTo( float position ) const ;
    void stop() const ;
    //float position() const { return m_position ; }
    //void setPosition( float p ) { m_position = p ; }
    const QString name() const { return m_name ; }
    const MotionAxisID& id() const { return m_id ; }
    void setController( const MotionController* c) { m_controller = c ; }
    bool isMoving() const ;
    
    const MotionAxisParameters& parameters() const { return m_parameters ; }
    MotionAxisParameters& parameters() { return m_parameters ; }
    
    const NamedValue& position() const { return m_position ; }
    NamedValue& position() { return m_position ; }
    const NamedValue& stepsize() const { return m_stepsize ; }
    NamedValue& stepsize() { return m_stepsize ; }
    
    public slots:
      //void setDefaultVelocity( float v ) const ;
      void handleParameterUpdate() const ;
      void writeParameter( const MSParameter& par ) const ;
      void readParameter( MSParameter& par ) ;
      void applyPosition() const { moveTo( m_position.value().toFloat() ) ; }
      void readPosition() ;
      
  private:
      float updatePosition() ;
  private:
      MotionAxisID m_id ;
      QString m_name ;
      QString m_type ;
      // measurements from the machine
      NamedValue m_position ;
      NamedValue m_stepsize ;
      MotionAxisParameters m_parameters ;
      const MotionController* m_controller ;
  };
}

#endif // MOTIONAXIS_H
