#ifndef WH_CAMERAWINDOW_H
#define WH_CAMERAWINDOW_H

#include <QWidget>
#include <QMainWindow>

class QLabel;

namespace PAP
{
  class CameraView ;
  
  class CameraWindow : public QMainWindow
  {
    Q_OBJECT
    
  public:
    explicit CameraWindow(QWidget *parent = 0);
    ~CameraWindow() {}

  private:
    CameraView* m_cameraview ;
    // some labels that tell where the cursor position is
    QLabel* m_cursorposition ;
  } ;
}

#endif // MAINWINDOW_H
