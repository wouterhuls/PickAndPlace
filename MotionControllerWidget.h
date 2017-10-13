#ifndef MOTIONCONTROLLERWIDGET_H
#define MOTIONCONTROLLERWIDGET_H

#include <QWidget>

class QAbstractButton ;
class QLabel ;

namespace PAP
{
  class MotionController ;
  
  class MotionControllerWidget : public QWidget
  {
    Q_OBJECT
    
  public:
    explicit MotionControllerWidget(const MotionController& controller,
				    QWidget *parent = 0) ;
    ~MotionControllerWidget();
    
    private slots:
      //void on_leftButton_clicked();
      //void on_homeButton_clicked();
      //void on_rightButton_clicked();
      void on_motorsOnButton_clicked() ;
      void update() ;
      //void showPosition() ;
  private:
      const MotionController* m_controller ;
      QAbstractButton* m_motorsonbutton ;
      QLabel* m_statuslabel ;
      QLabel* m_errorlabel ;
  };
}

#endif // MOTIONCONTROLLERWIDGET_H
