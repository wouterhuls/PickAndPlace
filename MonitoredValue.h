#ifndef MONITOREDVALUE_H
#define MONITOREDVALUE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>

namespace PAP
{

  /*
  class ValueChangedEmitter : public QObject
  {
    Q_OBJECT
  signals:
    void valueChanged() ;
  public:
    virtual ~ValueChangedEmitter() {}
  } ;
  */

  class MonitoredValueBase : public QObject
  {
    Q_OBJECT
  public:
    MonitoredValueBase() = default ;
    MonitoredValueBase( const MonitoredValueBase&) : QObject{} {}
    MonitoredValueBase( MonitoredValueBase&& ) : QObject{} {}
    virtual ~MonitoredValueBase() {}
    // to interface with property service
    virtual QString toString() const = 0 ;
    // return false in case of failure
    virtual bool fromString( const QString& val ) = 0 ;
  signals:
    void valueChanged() ;
  } ;

  template<class T>
    class MonitoredValue : virtual public MonitoredValueBase
  {
  public:
    using ValueType = T ;
    MonitoredValue() = default ;
    MonitoredValue( const ValueType& v) :  MonitoredValueBase{}, m_value{v} {}
    MonitoredValue( const MonitoredValue& rhs) = default ;//: m_value{rhs.m_value} {}
    MonitoredValue( MonitoredValue&& rhs) = default ;//: m_value{std::move(rhs.value)} {} ;
    // we also want automatic type conversion
    const ValueType& value() const { return m_value; }
    operator ValueType() const { return m_value ; }
    void setValue(const ValueType& value) {
      //qDebug() << "MonitoredValue: set" << this << value ;
      if (value != m_value) {
	m_value = value;
	emit valueChanged() ;
      }
    }
    void setWithoutSignal( const ValueType& value) { m_value = value; }
    // conversion
    MonitoredValue<T>& operator=(const ValueType& rhs) { set(rhs) ; return *this ; }
    // to interface with property service
    QString toString() const { return QVariant{m_value}.toString() ; }
    // this one will be implemented by template specialization
    bool fromString( const QString& val ) ;
    // return the qvariant type
    QVariant::Type type() const { return QVariant{m_value}.type() ; }
  private:
    T m_value ;
  } ;

  using MonitoredDouble = MonitoredValue<double> ;
  using MonitoredFloat  = MonitoredValue<float> ;
  using MonitoredInteger = MonitoredValue<int> ;
  using MonitoredString  = MonitoredValue<QString> ;
  using MonitoredQPointF = MonitoredValue<QPointF> ;

}

#endif
