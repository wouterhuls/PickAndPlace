#ifndef MOTIONAXISPARAMETERS_H
#define MOTIONAXISPARAMETERS_H

#include "NamedValue.h"

namespace PAP
{
  
  using MotionControllerID = int ;
  
  enum Direction { Up=+1, Down=-1 } ;
  
  struct MotionAxisID
  {
  MotionAxisID(int c=1, int a=1) : controller(c),axis(a) {}
    int controller ;
    int axis ;
    bool operator<(const MotionAxisID& rhs) const {
      return controller < rhs.controller ||
	( controller == rhs.controller && axis < rhs.axis ) ;
    }
  };

  class MSParameter : public NamedValue
  {
  public:
    MSParameter( const char* name, const ValueType& v ) : NamedValue(name,v) {}
    MSParameter( const QString& name, const ValueType& v ) : NamedValue(name,v) {}
    virtual ~MSParameter() {}
  } ;
  
  
  /*
    struct MotionAxisParameters
    {
    MotionAxisParameters() : stepsize(0.01),speed(1),acceleration(1) {}
    float stepsize ;
    float speed ;
    float acceleration ;
    };
    
    struct MotionAxisData
    {
    float speed ;
    float acceleration ;
    float position ;
    } ;
  */
  
  struct MotionControllerData
  {
    int status ;
    int error ;
  } ;
}


#endif // MOTIONAXISPARAMETERS_H
