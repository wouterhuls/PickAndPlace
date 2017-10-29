#ifndef MONITOREDVALUE_H
#define MONITOREDVALUE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>

namespace PAP
{

  class ValueChangedEmitter : public QObject
  {
    Q_OBJECT
  signals:
    void valueChanged() ;
  public:
    virtual ~ValueChangedEmitter() {}
  } ;

  template<class T>
    class MonitoredValue : public ValueChangedEmitter
  {
  public:
    using ValueType = T ;
  MonitoredValue( const ValueType& v) :  m_value(v) {}
  MonitoredValue( const MonitoredValue& rhs) : m_value(rhs.m_value) {}
    // we also want automatic type conversion
    const ValueType& value() const { return m_value; }
    operator ValueType() const { return m_value ; }
    const T& get() const { return m_value ; }
    void set(const ValueType& value) {
      if (value != m_value) {
	m_value = value;
	emit valueChanged() ;
      }
    }
    // conversion
    MonitoredValue<T>& operator=(const ValueType& rhs) { set(rhs) ; return *this ; }
    // to interface with property service
    QString toString() const { return QVariant(m_value).toString() ; }
    // this one will be implemented by template specialization
    bool fromString( const QString& val ) ;
    // return the qvariant type
    QVariant::Type type() const { return QVariant{m_value}.type() ; }
  private:
    T m_value ;
  } ;
}

#endif
