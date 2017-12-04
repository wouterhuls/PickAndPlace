#include "MotionAxisParameter.h"
#include "MotionSystemSvc.h"
#include "MotionAxis.h"
#include "MotionSystemCommandLibrary.h"
#include <QDebug>

namespace PAP
{
  MotionAxisParameter::MotionAxisParameter(MotionAxis& axis,
					   const MSCommandLibrary::ParameterDefinition& pardef)
    :
    m_axis{&axis},
    m_pardef{&pardef},
    m_setValue{axis.name() + "." + pardef.name, pardef.type},
    m_getValue{pardef.type},
    m_initialized(false)
  {
    //QObject::connect(&m_setValue,&ValueChangedEmitter::valueChanged,[&](){ this->write() ; }) ; 
    //QObject::connect(&m_setValue,&ValueChangedEmitter::valueChanged,this,&MotionAxisParameter::write) ;
    QObject::connect(&m_setValue,&NamedVariant::valueChanged,this,&MotionAxisParameter::write) ;
    QObject::connect(&m_getValue,&NamedVariant::valueChanged,this,&MotionAxisParameter::initSetValue) ;
    //qDebug() << "In MAP constructor: " << m_setValue.name() << &m_setValue ;
  }
  
  void MotionAxisParameter::write()
  {
    
    qDebug() << "MotionAxisParameter::write"
	     << m_setValue.value()
	     << m_getValue.value() ;
    if( m_setValue.value() != m_getValue.value() ) {
      m_initialized = true ;
      auto id = m_axis->id() ;
      switch(m_pardef->type) {
      case QVariant::Double:
	MotionSystemSvc::instance()->applyAxisCommand(id,m_pardef->setcmd,m_setValue.value().toDouble()) ;
	break;
      case QVariant::String:
	MotionSystemSvc::instance()->applyAxisCommand(id,m_pardef->setcmd,m_setValue.toString().toUtf8().constData()) ;
	break;
      default:
	qDebug() << "applyParameter: Don't know what to do with this parameter type: " << m_pardef->type ;
      }
      // update the getvalue
      read() ;
    }
  }
  
  void MotionAxisParameter::read()
  {
    // qDebug() << "Reading motion axis parameter "
    // 	     << m_setValue.name()
    // 	     << m_setValue.value() << m_getValue.value() ;
    MotionSystemSvc::instance()->readAxisVariable(m_axis->id(),m_pardef->getcmd,m_getValue) ;
  }

  void MotionAxisParameter::initSetValue()
  {
    // qDebug() << "MotionAxisParameter: Call back from getvalue change" << m_setValue.name()
    // 	     << m_getValue.value() << m_initialized ;
    m_setValue.setWithoutSignal(m_getValue.value()) ;
    m_initialized = true ;
  }
}
