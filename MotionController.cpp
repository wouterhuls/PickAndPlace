#include "MotionController.h"
#include "MotionSystemSvc.h"

#include <QDebug>


MotionController::MotionController( const int& id, const std::string& name )
  : m_id(id), m_name(name), m_status(0), m_error("none")
{
}

void MotionController::switchMotorsOn( bool on) const
{
  MotionSystemSvc::instance()->switchMotorsOn( m_id, on ) ;
}

bool MotionController::hasMotorsOn() const
{
  // analyse the status flag to see if the motors are on
  return m_status & (1>>4) ;
}
