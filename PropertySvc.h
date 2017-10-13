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

  using Property=NamedValue ;
  
  class PropertySvc : public Singleton<PropertySvc>
  {
  public:
    void add( Property& var ) ;
    void read( const char* filename ) ;
    void write( const char* filename ) ;
    Property* find( const char* name ) ;
    virtual ~PropertySvc() { write("config_last.txt") ; }
  private:
    std::vector< Property* > m_props ;
  } ;

  
}

#endif
