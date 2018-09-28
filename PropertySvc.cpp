#include "PropertySvc.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>

namespace PAP
{
  
  void PropertySvc::add( Property& var)
  {
    m_props.push_back( &var ) ;
  }
    
  Property* PropertySvc::find( const char* name )
  {
    auto key = QString(name) ;
    auto it = std::find_if( std::begin(m_props),
			    std::end(m_props),
			    [key](Property* p)->bool { return p->name()==key; }  ) ;
    return it != std::end(m_props) ? *it : 0 ;
  } ;
  
  void PropertySvc::read( const char* filename )
  {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      return;

    QTextStream in(&file);
    while (!in.atEnd()) {
      QString line = in.readLine();
      if( line.size() > 0 && line[0] != '#') {
	auto pos = line.indexOf("=") ;
	auto name  = line.leftRef(pos).trimmed() ;
	auto value = line.rightRef(line.size()-pos-1).trimmed() ;
	//qDebug() << "name,value: " << name << " " << value  ;
	Property* prop = find( name.toString().toStdString().c_str() ) ;
	if (! prop )
	  qDebug() << "Cannot find property read from file!" << name.toString() ;
	else {
	  if( !prop->fromString( value.toString() ) ) {
	    qDebug() << "Cannot convert property to target type!" ;
	  }
	}
      }
    }
  }
  
  void PropertySvc::write( const char* filename )
  {
    qDebug() << "Writing config to " << filename  ;
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    
    QTextStream out(&file);
    for( const auto& i : m_props )
      out << i->name() << "=" << i->toString() << endl ;
  }
}

