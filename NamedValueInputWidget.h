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
    NamedValueInputWidget(NamedValueBase& v, QWidget *parent = 0) ;
    virtual ~NamedValueInputWidget() {}
  public slots:
    void updateLabel() ;
    void buttonpressed() ;  
  private:
    NamedValueBase* m_v ;
    QLabel m_label ;
  } ;
}


#endif
  
