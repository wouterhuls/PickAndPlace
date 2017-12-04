#include "MonitoredValue.h"

namespace PAP
{
  
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

}
  
