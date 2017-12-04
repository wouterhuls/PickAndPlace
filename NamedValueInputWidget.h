#ifndef NamedValueInputWidget_H
#define NamedValueInputWidget_H

#include <QWidget>
#include <QLabel>
#include "NamedValue.h"

namespace PAP
{
  template<typename T>
  class NamedValueInputWidget : public QWidget
  {
    //Q_OBJECT  
  public:
    NamedValueInputWidget(NamedValue<T>& v,
			  const T& min,
			  const T& max,
			  int decimals,
			  QWidget *parent = 0) ;
    virtual ~NamedValueInputWidget() {}
  public:
    void updateLabel() ;
    void buttonpressed() ;  
  private:
    NamedValue<T>* m_v ;
    T m_min ;
    T m_max ;
    int m_decimals ;
    QLabel m_label ;
  } ;

  // specializations need to be done before instantiations
  template<> void NamedValueInputWidget<double>::buttonpressed() ;
  template<> void NamedValueInputWidget<QVariant>::buttonpressed() ;
  extern template class NamedValueInputWidget<QVariant>;
  extern template class NamedValueInputWidget<double>;
}


#endif
  
