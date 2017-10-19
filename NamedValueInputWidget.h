#ifndef NamedValueInputWidget_H
#define NamedValueInputWidget_H

#include <QWidget>
#include <QLabel>

namespace PAP
{
  class NamedValue ;
  
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
  
