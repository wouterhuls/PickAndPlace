#ifndef PROPERTYSVC_H
#define PROPERTYSVC_H

// singleton class to deal with 'properties' of tools r algorithms
// that can be written to a file
//
// this class is not the owner: it just keeps the list of all properties
//

#include "NamedValue.h"
#include "Singleton.h"

namespace PAP
{

  using Property=NamedValueBase ;
  
  class PropertySvc : public Singleton<PropertySvc>, public QObject
  {
  public:
    void add( Property& var ) ;
    void add( Property& var, const QString& name ) ;
    void read( const char* filename ) ;
    void write( const char* filename ) ;
    Property* find( const char* name ) ;
    ~PropertySvc() ;
    PropertySvc() ;
  private:
    std::vector< Property* > m_props ;
  } ;

  
}

#endif
