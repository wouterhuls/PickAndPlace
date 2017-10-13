#ifndef MOTIONAXISWIDGET_H
#define MOTIONAXISWIDGET_H

#include <QWidget>
class QLCDNumber ;

namespace PAP
{

  class MotionAxis ;

  
  class MotionAxisWidget : public QWidget
  {
    Q_OBJECT
    
  public:
    explicit MotionAxisWidget(MotionAxis& axis, QWidget *parent = 0) ;
    ~MotionAxisWidget();
    
    private slots:
      void on_stepDownButton_clicked();
      void on_moveDownButton_pressed();
      void on_moveDownButton_released();
      
      void on_stepUpButton_clicked();
      void on_moveUpButton_pressed();
      void on_moveUpButton_released();
      void on_settingsButton_clicked();  
      
      void showPosition() ;
      
  private:
      MotionAxis* m_axis ;
      QLCDNumber* m_positionLabel ;
  };
}

#endif // MOTIONAXISWIDGET_H
