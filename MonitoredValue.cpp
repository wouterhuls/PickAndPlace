#include "MonitoredValue.h"
#include <QPointF>

namespace PAP
{
  bool MonitoredValueBase::fromString( const QString& val, const QDateTime& time)
  {
    auto rc = fromString(val) ;
    if( rc ) m_timestamp = time ;
    return rc ;
  }
  
  // template specializations
  template<>
  bool MonitoredValue<QVariant>::fromString( const QString& val ) {
    QVariant qval(val) ;
    bool success = qval.convert(type()) ;
    if( success ) setValue( val ) ;
    return success ;
  }
  
  template<>
  bool MonitoredValue<double>::fromString( const QString& val ) {
    QVariant qval(val) ;
    bool success = qval.convert(QVariant::Double) ;
    if( success ) setValue( val.toDouble() ) ;
    return success ;
  }

  template<>
  bool MonitoredValue<float>::fromString( const QString& val ) {
    QVariant qval(val) ;
    bool success = qval.convert(QVariant::Double) ;
    if( success ) setValue( val.toDouble() ) ;
    return success ;
  }
  
  template<>
  bool MonitoredValue<int>::fromString( const QString& val ) {
    QVariant qval(val) ;
    bool success = qval.convert(QVariant::Int) ;
    if( success ) setValue( val.toInt() ) ;
    return success ;
  }

  template<>
  bool MonitoredValue<QString>::fromString( const QString& val ) {
    qDebug() << "MonitoredValue<QString>::fromString: " << this << val ;
    setValue(val) ;
    return true ;
  }
 
  template<>
  bool MonitoredValue<QPointF>::fromString( const QString& val ) {
    QVariant qval(val) ;
    /*bool success = qval.convert(QVariant::QPointF) ;
      if( success ) */
    setValue( qval.toPointF() ) ;
    return true ;
  }

  template<>
  QString MonitoredValue<QPointF>::toString() const
  {
    char textje[256] ;
    sprintf(textje,"(%f,%f)", m_value.x(), m_value.y()) ;
    return QString{textje} ;
  }
 
}
  
