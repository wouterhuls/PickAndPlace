#include <QDebug>
#include <QTimer>
#include "MotionAxis.h"
#include "MotionController.h"
#include "MotionSystemSvc.h"
#include "PropertySvc.h"
#include "MotionSystemCommandLibrary.h"
#include "NamedValue.h"

// Let;s make a list of all parameters. Perhaps we are not dealing with so much
namespace PAP
{

  MotionAxis::MotionAxis(const MotionAxisID& id, const QString& name, const QString& type,
			 const MotionController& c) 
    : m_id{id},
      m_name{name},
      m_type{type},
      m_position{name + ".Position",0.},
      m_stepsize{name + ".Stepsize",0.005,0.0,1.0},
      m_isMoving(false),
      m_controller{&c}
  {
    qInfo() << "MotionAxis: defined controller for "
	    << id.controller << " " << id.axis << " "
	    << m_name << " " << m_type ;
    //m_position = MotionSystemSvc::instance()->readAxisFloat(m_id,"TP") ;
    //QObject::connect(&m_position,&QVariable::valueChanged,this,&MotionAxis::applyPosition);
    if( false && MotionSystemSvc::instance()->isReady() ) {
      QTimer *timer = new QTimer(this);
      QObject::connect(timer, SIGNAL(timeout()), this, SLOT(readPosition()));
      timer->start(1000);
    }
    // read the position now and again every time the motors have stopped
    //    readPosition() ;
    QObject::connect(this,&MotionAxis::movementStopped,this,&MotionAxis::readPosition);
    
    // connect some of the slots
    m_parameters.reserve( MSCommandLibrary::Parameters.size() ) ;
    for( const auto& p: MSCommandLibrary::Parameters ) {
      if(p.configurable) {
	m_parameters.push_back( MSParameter{ QString{name} + "." + p.name,
					     QVariant{p.type},p.minvalue, p.maxvalue } ) ;
	MSParameter& par = m_parameters.back() ;
	// set the initial value:
	// readParameter( par ) ;
	// for now, disable the callbacks!
	QObject::connect( &par, &NamedValue::valueChanged, this, &MotionAxis::handleParameterUpdate ) ;
	PAP::PropertySvc::instance()->add( par ) ;
      }
    }
    PAP::PropertySvc::instance()->add( m_stepsize ) ;
  }
  
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

  void MotionAxis::readParameters()
  {
    for(auto& par : m_parameters ) readParameter( par ) ;
    MotionSystemSvc::instance()->applyAxisReadCommand(m_id,"TA") ;
    MotionSystemSvc::instance()->applyAxisReadCommand(m_id,"TP") ;
  }
  
  void MotionAxis::readParameter( MSParameter& par )
  {
    // make sure to disable the callback
    QObject::disconnect( &par, &NamedValue::valueChanged, this, &MotionAxis::handleParameterUpdate ) ;
    // note: at the moment this bypasses the parser
    auto pardef = MSCommandLibrary::findParDef(par.shortname()) ;
    if(pardef)
      //MotionSystemSvc::instance()->readAxisVariable(m_id,pardef->getcmd,par) ;
      MotionSystemSvc::instance()->applyAxisReadCommand(m_id,pardef->getcmd) ;
    else 
      qDebug() << "readParameter: Cannot find parameter definition for: " << par.shortname() ;
    // enable the callback again
    // QObject::connect( &par, &NamedValue::valueChanged, this, &MotionAxis::handleParameterUpdate ) ;
  }

  void MotionAxis::readPosition()
  {
    MotionSystemSvc::instance()->readAxisVariable(m_id,"TP",m_position) ;
  }

  bool MotionAxis::hasMotorOn() const {
    return m_controller->hasMotorsOn() ;
  }
  
  void MotionAxis::step( Direction dir )
  {
    //setIsMoving( true ) ;
    move( dir * m_stepsize ) ;
  }

  void MotionAxis::move( float delta )
  {
    //setIsMoving( true ) ;
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"PR",delta) ;
  }
  
  void MotionAxis::move( Direction dir )
  {
    //setIsMoving( true ) ;
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"MT",dir == Up ? "+" : "-") ;
  }
  
  void MotionAxis::stop()
  {
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"ST") ;
  }
  
  void MotionAxis::moveTo( float position )
  {
    //setIsMoving( true ) ;
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"PA",position) ;
  }
  
  void MotionAxis::searchHome()
  {
    //setIsMoving( true ) ;
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"OR") ;
  }
  
  void MotionAxis::setIsMoving(bool ismoving)
  {
    if(  ismoving && !m_isMoving )
      emit movementStarted() ;
    else if( !ismoving && m_isMoving )
      emit movementStopped() ;
    m_isMoving = ismoving ;
  }

  bool MotionAxis::parseData( const QString& cmd, const QString& value) {
    bool success = true ;
    if( cmd == "TP" ) {
      m_position = value.toFloat() ;
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
	auto pardef = MSCommandLibrary::findParDef(p.shortname()) ;
	if( !pardef ) {
	  qWarning() << "Strange: cannot find pardef: "
		   << p.shortname() ;
	} else if( cmd == pardef->getcmd ) {
	  QVariant var{value} ;
	  if( var.convert( pardef->type ) ) {
	    p.set( var );
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
}
  
