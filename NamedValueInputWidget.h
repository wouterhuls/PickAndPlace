#ifndef NamedValueInputWidget_H
#define NamedValueInputWidget_H

#include <QWidget>
#include <QLabel>
#include "NamedValue.h"

namespace PAP
{
  class NamedValueInputWidget : public QWidget
  {
    Q_OBJECT  
  public:
    NamedValueInputWidget(NamedValue& v, QWidget *parent = 0) ;
    virtual ~NamedValueInputWidget() {}
  public slots:
    void updateLabel() ;
    void buttonpressed() ;  
  private:
    NamedValue* m_v ;
    QLabel m_label ;
  } ;
}


#endif
  
