#ifndef MOTIONSYSTEMWIDGET_H
#define MOTIONSYSTEMWIDGET_H

#include <QWidget>
class QLCDNumber ;

namespace PAP
{
  class MotionAxis ;
  
  class MotionSystemWidget : public QWidget
  {
    Q_OBJECT
    
  public:
    explicit MotionSystemWidget(QWidget *parent = 0) ;
    ~MotionSystemWidget();
    
    private slots:
      void on_quitButton_clicked() ;
      void on_calibrateButton_clicked() ;
  private:
  };
}

#endif // MOTIONCONTROLLERWIDGET_H
