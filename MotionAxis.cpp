#include <QDebug>
#include <QTimer>
#include "MotionAxis.h"
#include "MotionController.h"
#include "MotionSystemSvc.h"
#include "PropertySvc.h"
#include "MotionSystemCommandLibrary.h"
#include "NamedValue.h"

#include <assert.h>
#include <cstring>

// Let;s make a list of all parameters. Perhaps we are not dealing with so much
namespace PAP
{

  MotionAxis::MotionAxis(const MotionAxisID& id, const QString& name, const QString& type,
			 const MotionController& c) 
    : m_id{id},
      m_name{name},
      m_type{type},
      m_isMoving(false),
      m_status(0),
      m_position{name + ".Position",0.},
      m_setPosition{name + ".SetPosition",0.},
      m_tolerance{name + ".Tolerance",0.0001},
      m_stepsize{name + ".Stepsize",0.005},
      m_antihysteresisstep{ name + ".AntiHysteresisStep",0.},
      m_allowPassTravelLimit(false),
      m_controller{&c}
  {
    qInfo() << "MotionAxis: defined controller for "
	    << id.controller << " " << id.axis << " "
	    << m_name << " " << m_type ;
    //m_position = MotionSystemSvc::instance()->readAxisFloat(m_id,"TP") ;
    //QObject::connect(&m_position,&QVariable::valueChanged,this,&MotionAxis::applyPosition);
    if( false/*false && MotionSystemSvc::instance()->isReady() */) {
      QTimer *timer = new QTimer(this);
      QObject::connect(timer, SIGNAL(timeout()), this, SLOT(readPosition()));
      timer->start(1000);
    }
    
    // read the position every time the motors have started, stopped,
    // or the position has changed
    QObject::connect(this,&MotionAxis::movementStarted,this,&MotionAxis::readPosition);
    QObject::connect(this,&MotionAxis::movementStopped,this,&MotionAxis::readPosition);
    QObject::connect(&m_position,&NamedValueBase::valueChanged,this,&MotionAxis::readPosition);
    QObject::connect(&m_position,&NamedValueBase::valueChanged,this,&MotionAxis::updateDirection);

    // initialize the set position the first time the position is read
    QObject::connect(&m_position,&NamedValueBase::valueChanged,
		     [&]() { if(m_setPosition.value()==0) m_setPosition.setValue( m_position.value() ) ; } ) ;
          
    // create the list of motion axis parameters.
    m_parameters.reserve( MSCommandLibrary::Parameters.size() ) ;
    for( const auto& p: MSCommandLibrary::Parameters ) {
      if(p.configurable) {
	m_parameters.push_back( new MotionAxisParameter{*this,p} ) ;
	auto& par = m_parameters.back() ;
	PAP::PropertySvc::instance()->add(par->setValue()) ;
	if( std::strcmp(p.name,"LeftTravelLimit")==0 ) m_leftTravelLimit = par;
	if( std::strcmp(p.name,"RightTravelLimit")==0 ) m_rightTravelLimit = par;
      }
    }
    assert( m_leftTravelLimit ) ;
    assert( m_rightTravelLimit ) ;
    PAP::PropertySvc::instance()->add( m_stepsize ) ;
    PAP::PropertySvc::instance()->add( m_antihysteresisstep ) ;
    PAP::PropertySvc::instance()->add( m_tolerance ) ;
  }

  /*
  void MotionAxis::handleParameterUpdate() const
  {
    auto par = dynamic_cast<const MSParameter*>(sender()) ;
    if( par ) {
      writeParameter( *par ) ;
    } else {
      qDebug() << "Received a signal but cannot cast to MSParameter" ;
    }
  }
  
  void MotionAxis::writeParameter( const MSParameter& par ) const
  {
    auto pardef = MSCommandLibrary::findParDef(par.shortname()) ;
    if( pardef ) {
      switch(pardef->type) {
      case QVariant::Double:
	MotionSystemSvc::instance()->applyAxisCommand(m_id,pardef->setcmd,par.value().toDouble()) ;
	break;
      case QVariant::String:
	MotionSystemSvc::instance()->applyAxisCommand(m_id,pardef->setcmd,par.value().toString().toUtf8().constData()) ;
	break;
      default:
	qDebug() << "applyParameter: Don't know what to do with this parameter type: " << pardef->type ;
      }
    } else {
      qDebug() << "applyParameter: Cannot find parameter definition for: " << par.shortname() ;
    }
  }
  */

  void MotionAxis::readParameters()
  {
    for(auto& par : m_parameters ) par->read() ;
    readPosition() ;
    MotionSystemSvc::instance()->applyAxisReadCommand(m_id,"TA") ;
    MotionSystemSvc::instance()->applyAxisReadCommand(m_id,"TP") ;
  }

  /*
  void MotionAxis::readParameter( MSParameter& par )
  {
    auto pardef = MSCommandLibrary::findParDef(par.shortname()) ;
    if(pardef)
      MotionSystemSvc::instance()->applyAxisReadCommand(m_id,pardef->getcmd) ;
    else 
      qDebug() << "readParameter: Cannot find parameter definition for: " << par.shortname() ;
  }
  */

  void MotionAxis::readPosition()
  {
    MotionSystemSvc::instance()->readAxisVariable(m_id,"TP",m_position) ;
  }

  bool MotionAxis::hasMotorOn() const {
    return m_controller->hasMotorsOn() ;
  }
  
  void MotionAxis::step( Direction dir )
  {
    move( dir * m_stepsize ) ;
  }

  void MotionAxis::move( double delta )
  {
    m_setPosition.setValue(m_position.value() + delta) ;
    setIsMoving( true ) ;
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"PR",delta) ;
  }
  
  void MotionAxis::move( Direction dir )
  {

    //MotionSystemSvc::instance()->applyAxisCommand(m_id,
    //     allowPassTravelLimit() ? "MT" : "MV",dir == Up ? "+" : "-") ;
    if( allowPassTravelLimit() ) {
      setIsMoving( true ) ;      
      MotionSystemSvc::instance()->applyAxisCommand(m_id,"MT",dir == Up ? "+" : "-") ;
    } else {
      // for some reason the MV command does not work: is there something that disables continuous motion?
      // as an alternative, we could try to move to the travel limit explicitly.
      // MotionSystemSvc::instance()->applyAxisCommand(m_id,"MV",dir == Up ? "+" : "-") ;
      if( dir == Up )
	moveTo( m_rightTravelLimit->setValue().value().toDouble() ) ;
      else
	moveTo( m_leftTravelLimit->setValue().value().toDouble() ) ;
    }
  }
  
  void MotionAxis::applyAntiHysteresisStep()
  {
    if(m_antihysteresisstep!=0) {
      move( -m_antihysteresisstep ) ;
      // For some reason, this does not do anything.
      //MotionSystemSvc::instance()->applyAxisCommand(m_id,"WS10") ;
      move( +m_antihysteresisstep ) ;
    }
  }
  
  void MotionAxis::stop()
  {
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"ST") ;
    m_setPosition.setValue( m_position.value() ) ;
  }
  
  void MotionAxis::moveTo( double position )
  {
    qDebug() << "MotionAxis::moveTo: " << position ;
    m_setPosition.setValue( position ) ;
    setIsMoving( true ) ;
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"PA",position) ;
  }
  
  void MotionAxis::defineHome()
  {
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"DH") ;
  }
  
  void MotionAxis::searchHome()
  {
    setIsMoving( true ) ;
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"OR") ;
  }

  void MotionAxis::setZero()
  {
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"ZP") ;
    readPosition() ;
    m_rightTravelLimit->read() ;
    m_leftTravelLimit->read() ;
  }
 
  void MotionAxis::setIsMoving(bool ismoving)
  {
    //qDebug() << "axis: " << name() << "set is moving: " << ismoving << " " << m_isMoving ;
    // it seems I have some weird problem with synchronization here.
    std::swap(m_isMoving,ismoving) ;
    if( !ismoving && m_isMoving ) {
      //qDebug() << "axis: " << name() << "emitted movementStarted" ;
      emit movementStarted() ;
    }
    else if( ismoving && !m_isMoving ) {
      //qDebug() << "axis: " << name() << "emitted movementStopped" ;
      emit movementStopped() ;
    }
  }

  bool MotionAxis::parseData( const QString& cmd, const QString& value) {
    bool success = true ;
    if( cmd == "TP" ) {
      m_position = value.toDouble() ;
    } else if (cmd == "MS") {
      // it may be that this doesn't work, because we need to go via a char
      //m_status = value.toUInt() ;
      m_status = value[0].toLatin1() ;
    } else if (cmd == "TA") {
      m_type = value ;
    } else {
      // finally, go through list of parameters. far too slow ..
      success = false ;
      for( auto& p : m_parameters ) {
	if( cmd  == p->pardef().getcmd ) {
	  QVariant var{value} ;
	  if( var.convert( p->pardef().type ) ) {
	    p->getValue().setValue( var );
	    success = true ;
	  }
	  break ;
	}
      }
    }
    if( !success ) 
      qWarning() << "MotionAxis: Could not parse data " << cmd << value ;
    return success ;
  }

  void MotionAxis::updateDirection()
  {
    if( m_prevPosition > m_position.value() + m_tolerance.value() ) {
      m_direction = MotionAxis::Direction::Down ;
      m_prevPosition = m_position ;
    } else if( m_prevPosition < m_position.value() - m_tolerance.value() ) {
      m_direction = MotionAxis::Direction::Up ;
      m_prevPosition = m_position ;
    }
  }
  
  MotionAxisParameter* MotionAxis::parameter(const QString& name)
  {
    auto it = std::find_if( m_parameters.begin(),
			    m_parameters.end(),
			    [name,this](const MotionAxisParameter* par) {
			      return par->name()==name || par->name() == this->name() + "." + name ;
			    } ) ;
    if( it == m_parameters.end() ) {
      qDebug() << "Cannot find parameter '" << name << "'. Choose from: " ;
      for_each(m_parameters.begin(),
	       m_parameters.end(), [](const MotionAxisParameter* p) {
		 qDebug() << p->name() ;
	       }) ;
    }
    
    return it != m_parameters.end() ? *it : 0 ;
  }
}
