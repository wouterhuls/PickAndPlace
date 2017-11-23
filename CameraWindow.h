#ifndef WH_CAMERAWINDOW_H
#define WH_CAMERAWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include "CoordinateMeasurement.h"

class QLabel;
class QCheckBox ;

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
    void toggleView(int view) ;
  private:
    CameraView* m_cameraview ;
    AutoFocus* m_autofocus ;
    // some labels that tell where the cursor position is
    QLabel* m_cursorposition ;
    QCheckBox* m_showNSideTiles ;
    QCheckBox* m_showCSideTiles ;       
  } ;

}

#endif // MAINWINDOW_H
