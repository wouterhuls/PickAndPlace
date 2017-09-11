#ifndef MOTIONCONTROLLER_H
#define MOTIONCONTROLLER_H

//#include "MotionAxis.h"
#include <string>

class MotionController
{
 public:
  MotionController( const int& id, const std::string& name ) ;
  void switchMotorsOn(bool on = true) const ;
  bool hasMotorsOn() const ;
  const std::string name() const { return m_name ; }
  int id() const { return m_id ; }
  char status() const { return m_status ; }
  const std::string& error() const { return m_error; }
  void setStatus( unsigned int status ) { m_status = status ; }
  void setError( const std::string& error ) { m_error = error ; }
 private:
  int m_id ;
  std::string m_name ;
  unsigned int m_status ;
  std::string m_error ;
};

#endif // MOTIONAXIS_H
