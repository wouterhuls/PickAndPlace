#ifndef WH_CAMERAWINDOW_H
#define WH_CAMERAWINDOW_H

#include <QWidget>
#include <QMainWindow>

class QLabel;

namespace PAP
{
  class CameraView ;
  class AutoFocus ;
  
  class CameraWindow : public QMainWindow
  {
    Q_OBJECT
    
  public:
    explicit CameraWindow(QWidget *parent = 0);
    ~CameraWindow() {}

  public slots:
    void on_focusButton_clicked() ;
    void on_quitButton_clicked();
  private:
    CameraView* m_cameraview ;
    AutoFocus* m_autofocus ;
    // some labels that tell where the cursor position is
    QLabel* m_cursorposition ;
    bool m_isFocussing ; // flag to disable focus button while focussing
  } ;
}

#endif // MAINWINDOW_H
