#ifndef PAP_MONITOREDVALUELABEL
#define PAP_MONITOREDVALUELABEL

#include <QLabel>
#include "MonitoredValue.h"

namespace PAP
{
  class MonitoredValueLabel : public QLabel
  {
  public:
  MonitoredValueLabel(MonitoredValueBase& value, const QString& name="",
		      QWidget *parent=nullptr ) : QLabel{parent}, m_value{&value}, m_name{name}
    {
      const QString prefix = m_name.isEmpty() ? "" : m_name + ": " ;
      setText( prefix + m_value->toString() ) ;
      connect(m_value,&MonitoredValueBase::valueChanged,
	      [this,prefix]{
		//qDebug() << prefix << "value: " << m_value->toString() ;
		this->setText( prefix + m_value->toString() ) ; } ) ;
    }
  private:
    MonitoredValueBase* m_value{nullptr} ;
    QString m_name ;
  } ;
  
}

#endif
