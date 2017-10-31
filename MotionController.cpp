#include "MotionController.h"
#include "MotionSystemSvc.h"

#include <QDebug>

namespace PAP
{

  MotionController::MotionController( const int& id, const std::string& name )
    : m_id(id),
      m_name(name),
      m_status(0),
      m_errorcode(0),
      m_error("none")
  {
  }
  
  void MotionController::switchMotorsOn( bool on) const
  {
    MotionSystemSvc::instance()->switchMotorsOn( m_id, on ) ;
  }
  
  bool MotionController::hasMotorsOn() const
  {
    // analyse the status flag to see if the motors are on
    return m_status & (1<<4) ;
  }

  void MotionController::setStatus( unsigned int status )
  {
    if( status != m_status ) {
      m_status = status ;
      for(auto& axis : m_axes)
	axis->setIsMoving( status & (1 << (axis->id().axis-1) ) ) ;
      //emit statusChanged() ;
    }
    emit statusChanged() ;
  }
  
  void MotionController::setErrorCode( unsigned int errorcode )
  {
    if( m_errorcode != errorcode ) {
      m_errorcode = errorcode ;
      emit statusChanged() ;
    }
  }
  
  void MotionController::setErrorMsg( const QString& error )
  {
    if( error != m_error ) {
      m_error = error ;
      emit statusChanged() ;
    }
  }

}
