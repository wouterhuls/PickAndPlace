#include <QDebug>
#include <QTimer>
#include "MotionAxis.h"
#include "MotionController.h"
#include "MotionSystemSvc.h"
#include "PropertySvc.h"
#include "MotionSystemCommandLibrary.h"

// Let;s make a list of all parameters. Perhaps we are not dealing with so much
namespace PAP
{

  MotionAxis::MotionAxis(const MotionAxisID& id, const QString& name, const QString& type,
			 const MotionController& c) 
    : m_id{id}, m_name{name},
      m_type{type},
      m_position{name + ".Position",0.},
      m_stepsize{name + ".Stepsize",0.005,0.0,1.0},
      m_controller{&c}
  {
    qInfo() << "MotionAxis: defined controller for "
	    << id.controller << " " << id.axis << " "
	    << m_name << " " << m_type ;
    //m_position = MotionSystemSvc::instance()->readAxisFloat(m_id,"TP") ;
    //QObject::connect(&m_position,&QVariable::valueChanged,this,&MotionAxis::applyPosition);
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(readPosition()));
    timer->start(5000);
    
    // connect some of the slots
    m_parameters.reserve( MSCommandLibrary::Parameters.size() ) ;
    for( const auto& p: MSCommandLibrary::Parameters ) {
      if(p.configurable) {
	m_parameters.push_back( MSParameter{ QString{name} + "." + p.name,
					     QVariant{p.type},p.minvalue, p.maxvalue } ) ;
	MSParameter& par = m_parameters.back() ;
	// set the initial value:
	readParameter( par ) ;    
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
  
  void MotionAxis::readParameter( MSParameter& par )
  {
    // note: at the moment this bypasses the 
    auto pardef = MSCommandLibrary::findParDef(par.shortname()) ;
    if(pardef)
      MotionSystemSvc::instance()->readAxisVariable(m_id,pardef->getcmd,par) ;
    else 
      qDebug() << "readParameter: Cannot find parameter definition for: " << par.shortname() ; 
  }

  void MotionAxis::readPosition()
  {
    m_position = MotionSystemSvc::instance()->readAxisFloat(m_id,"TP") ;
  }
  
  void MotionAxis::step( Direction dir ) const
  {
    move( dir * m_stepsize.value().toDouble() ) ;
  }

  void MotionAxis::move( float delta ) const
  {
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"PR",delta) ;
  }
  
  void MotionAxis::move( Direction dir ) const
  {
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"MT",dir == Up ? "+" : "-") ;
  }
  
  void MotionAxis::stop() const
  {
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"ST") ;
  }
  
  void MotionAxis::moveTo( float position ) const
  {
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"PA",position) ;
  }
  
  void MotionAxis::searchHome() const
  {
    MotionSystemSvc::instance()->applyAxisCommand(m_id,"OR") ;
  }
  
  bool MotionAxis::isMoving() const
  {
    return m_controller->status() & (1 << (m_id.axis-1) ) ;
  }
}
  
