#ifndef NAMEDVALUE_H
#define NAMEDVALUE_H

#include "MonitoredValue.h"

namespace PAP
{

  class NamedValueBase : virtual public MonitoredValueBase
  {
   Q_OBJECT 
  public:
    NamedValueBase( const QString& name ) : m_name(name) {}
    NamedValueBase(const NamedValueBase& rhs) : m_name(rhs.m_name) {}
    // make sure that we have a v-table
    virtual ~NamedValueBase() {}
    //const char* name() const { return m_name.c_str() ; }
    const QString& name() const { return m_name ; }
    const QStringRef shortname() const {
      int pos = m_name.lastIndexOf(".") ;
      return m_name.rightRef(m_name.size()-pos-1 ) ;
    } ;
    
    // to interface with property service
    //virtual QString toString() const = 0 ;
    // return false in case of failure
    //virtual bool fromString( const QString& val ) = 0 ;
    //signals:
    //void valueChanged() ;
  private:
    QString m_name ;
  } ;

  template<typename T>
    class NamedValue : public NamedValueBase, public MonitoredValue<T>
  {
  public:
    using ValueType = T ;
    NamedValue( const QString&name ) : NamedValueBase(name) {}
    NamedValue( const QString& name, const ValueType& v) :  NamedValueBase(name), MonitoredValue<T>(v)  {}
    //  NamedValue( const QString& name, const ValueType& v, const ValueType& min, const ValueType& max)
    //	: NamedValueBase(name), m_value(v), m_min(min), m_max(max) {}
    //NamedValue(const NamedValue& rhs) : NamedValueBase(rhs),m_value(rhs.m_value),m_min(rhs.m_min),m_max(rhs.m_max) {}
    // we also want automatic type conversion
    // conversion
    NamedValue& operator=(const ValueType& rhs) { MonitoredValue<T>::setValue(rhs) ; return *this ; }
  private:
    T m_value ;
  } ;
 
  /**
  template<typename T>
    class NamedValue : public MonitoredValue<T>, virtual public NamedValueBase
  {
  public:
    using ValueType = T ;
    NamedValue( const QString&name ) : NamedValueBase(name) {}
    NamedValue( const QString& name, const ValueType& v) :  NamedValueBase(name), m_value(v) {}
    NamedValue( const QString& name, const ValueType& v, const ValueType& min, const ValueType& max)
	: NamedValueBase(name), m_value(v), m_min(min), m_max(max) {}
      //NamedValue(const NamedValue& rhs) : NamedValueBase(rhs),m_value(rhs.m_value),m_min(rhs.m_min),m_max(rhs.m_max) {}
      // we also want automatic type conversion
      const ValueType& value() const { return m_value; }
      operator ValueType() const { return m_value ; }
      const T& get() const { return m_value ; }
      void setValue(const ValueType& value) { return set(value) ; }
      void set(const ValueType& value) {
	qDebug() << "NamedValue: set" << name() << this << value ;
	if (value != m_value) {
	  m_value = value;
	  emit valueChanged() ;
	}
      }
      void setWithoutSignal( const ValueType& value) {
	m_value = value;
      }
      // conversion
      NamedValue& operator=(const ValueType& rhs) { setValue(rhs) ; return *this ; }
      // to interface with property service
      QString toString() const { return QVariant(m_value).toString() ; }
      // this one will be implemented by template specialization
      bool fromString( const QString& val ) ;
            
      // return the qvariant type
      QVariant::Type type() const { return QVariant{m_value}.type() ; }

      // range: we should make one more base class level to include this at the right place
      const ValueType& min() const { return m_min; }
      const ValueType& max() const { return m_max; }
    private:
      T m_value ;
      T m_min ;
      T m_max ;
    } ;
  */

  using NamedVariant = NamedValue<QVariant> ;
  using NamedDouble  = NamedValue<double> ;
  using NamedFloat  = NamedValue<float> ;
  using NamedInteger = NamedValue<int> ;
}
#endif
