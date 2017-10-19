#ifndef QVARIABLE_H
#define QVARIABLE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>

/*
class QValueChangedEmitter : public QObject
{
 Q_OBJECT
   signals:
   void valueChanged() ;
} ;
*/

namespace PAP
{
  
  //template<typename T=QVariant, typename StringType = std::string>
  class NamedValue : public QObject
  {
    Q_OBJECT
    
    public:
      using ValueType = QVariant ;
      using StringType = QString ;
      using NameType   = StringType ;
      //NamedValue( const StringType& name ) : m_name(name) {}
      //NamedValue( const StringType& name, const ValueType& v ) : m_value(v), m_name(name) {}
      NamedValue( const QString& name, QVariant::Type type=QVariant::Invalid ) : m_name(name), m_value(type), m_min(type), m_max(type) {}
      NamedValue( const QString& name, const ValueType& v) : m_name(name), m_value(v), m_min(v.type()), m_max(v.type()) {}
      NamedValue( const QString& name, const ValueType& v, const ValueType& min, const ValueType& max)
	: m_name(name), m_value(v) { setLimits(min,max) ; }
      
      // QObject does not have a copy constructor. Let's hpoe that this works:-(
      NamedValue(const NamedValue& rhs) : QObject(),m_name(rhs.m_name),m_value(rhs.m_value),m_min(rhs.m_min),m_max(rhs.m_max) {}
      virtual ~NamedValue() {}
    
      //const char* name() const { return m_name.c_str() ; }
      const QString& name() const { return m_name ; }
        const QStringRef shortname() const {
	int pos = m_name.lastIndexOf(".") ;
	return m_name.rightRef(m_name.size()-pos-1 ) ;
      } ;
      
      const ValueType& value() const { return m_value; }
      operator ValueType() const { return m_value ; }
    
      void setValue(const ValueType& value) {
	if (value != m_value) {
	  m_value = value;
	  //emit valueChanged( m_value.toFloat() ) ;
	  //emit valueChanged(this);
	  //emit valueChanged( this ) ;
	}
      }
      
      NamedValue& operator=(const ValueType& rhs) { setValue(rhs) ; return *this ; }
    
      QVariant::Type type() const { return m_value.type() ; }

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
