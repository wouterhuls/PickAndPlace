#include "NamedValue.h"

namespace PAP
{
  
  // template specializations

  template<>
  bool NamedValueT<QVariant>::fromString( const QString& val ) {
    QVariant qval(val) ;
    bool success = qval.convert(type()) ;
    if( success ) setValue( val ) ;
    return success ;
  }
  
  template<>
  bool NamedValueT<double>::fromString( const QString& val ) {
    QVariant qval(val) ;
    bool success = qval.convert(QVariant::Double) ;
    if( success ) setValue( val.toDouble() ) ;
    return success ;
  }

}
  
