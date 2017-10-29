#ifndef QVARIABLE_H
#define QVARIABLE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>

#include "MonitoredValue.h"


namespace PAP
{
  
  class NamedValueBase : public ValueChangedEmitter
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
    virtual QString toString() const = 0 ;
    // return false in case of failure
    virtual bool fromString( const QString& val ) = 0 ;
  signals:
    void valueChanged() ;
  private:
    QString m_name ;
  } ;

  template<typename T>
    class NamedValueT : public NamedValueBase
    {
    public:
      using ValueType = T ;
      NamedValueT( const QString&name ) : NamedValueBase(name) {}
      NamedValueT( const QString& name, const ValueType& v) :  NamedValueBase(name), m_value(v) {}
      NamedValueT( const QString& name, const ValueType& v, const ValueType& min, const ValueType& max)
	: NamedValueBase(name), m_value(v), m_min(min), m_max(max) {}
      NamedValueT(const NamedValueT& rhs) : NamedValueBase(rhs),m_value(rhs.m_value),m_min(rhs.m_min),m_max(rhs.m_max) {}
      // we also want automatic type conversion
      const ValueType& value() const { return m_value; }
      operator ValueType() const { return m_value ; }
      const T& get() const { return m_value ; }
      void setValue(const ValueType& value) { return set(value) ; }
      void set(const ValueType& value) {
	if (value != m_value) {
	  m_value = value;
	  emit valueChanged() ;
	}
      }
      // conversion
      NamedValueT& operator=(const ValueType& rhs) { setValue(rhs) ; return *this ; }
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

  using NamedValue  = NamedValueT<QVariant> ;
  using NamedDouble = NamedValueT<double> ;

 
  /*
  class NamedValue : public NamedValueT<QVariant>
  {
    public:
      NamedValue( const QString& name, QVariant::Type type=QVariant::Invalid ) : NamedValueT(name,type) {}
      NamedValue( const QString& name, const ValueType& v) : NamedValueT(name,v) {}
      NamedValue( const QString& name, const ValueType& v, const ValueType& min, const ValueType& max)
	: NamedValueT(name,v,min,max) {}
      
      // QObject does not have a copy constructor. Let's hpoe that this works:-(
      NamedValue(const NamedValue& rhs) : NamedValueT(rhs) {}
    
      QVariant::Type type() const { return m_value.type() ; }
      void fromString( const QString& val ) { setValue(QVariant(val).convert(type())) ; }
      
      void setLimits( ValueType min, ValueType max)
      {
	if( min.convert( m_value.type() ) && max.convert( m_value.type() ) ) {
	  m_min = min ; 
	  m_max = max ;
	} else {
	  qWarning() << "NamedValue: Cannot convert limits to target type" ;
	  abort() ;
	}
      }

      const ValueType& min() const { return m_min; }
      const ValueType& max() const { return m_max; }
            
    signals:
      //void valueChanged( QVariant ) const ;
      void valueChanged() const ;
      //void valueChanged( const NamedValue* ) const ;
      
    private:
      StringType m_name ;
      ValueType m_value ;
      ValueType m_min ;
      ValueType m_max ;
  } ;
  */
  

  
}

//using NamedVariable = NamedValue ;

//using NamedValueF = NamedValue<float> ;
//using NamedValueI = NamedValue<int> ;
//using NamedValueS = NamedValue<std::string> ;

/*
template<typename T>
class NamedValueActiveLabel
{
  // class that show name + value and has a push button to change the type
  
  QAbstractWidget* NamedValue


bool ok;
  // get the current velocity
  double d = QInputDialog::getDouble(this,
				     tr("QInputDialog::getDouble()"),
				     tr("Velocity:"),
				     m_axis->parameters().velocity,
				     0, 1, 0.001, &ok);
  if (ok) {
    m_axis->setDefaultVelocity( d ) ;
  }
*/

#endif
