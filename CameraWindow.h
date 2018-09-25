#ifndef WH_CAMERAWINDOW_H
#define WH_CAMERAWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include "CoordinateMeasurement.h"
#include "Singleton.h"

class QLabel;
class QCheckBox ;
class QVideoFrame;

namespace PAP
{
  class CameraView ;
  class AutoFocus ;
  class MeasurementReportPage ;
  
  class CameraWindow : public QMainWindow
  {
    Q_OBJECT
    
  public:
    explicit CameraWindow(QWidget *parent = 0);
    ~CameraWindow() {}
    CameraView* cameraview() { return m_cameraview ; }
    AutoFocus* autofocus() { return m_autofocus ; }
    MeasurementReportPage* measurementreport() { return m_measurementreport ; }

  public slots:
    void on_focusButton_clicked() ;
    //void on_quitButton_clicked();
    void on_stopButton_clicked();
    void toggleView(int view) ;
    void processFrame( const QVideoFrame& frame ) ;
    void moveToMarker() ;
  private:
    CameraView* m_cameraview ;
    AutoFocus* m_autofocus ;
    MeasurementReportPage* m_measurementreport ;
    // some labels that tell where the cursor position is
    QLabel* m_cursorposition ;
    QCheckBox* m_showNSideTiles ;
    QCheckBox* m_showCSideTiles ;

    // boolean to trigger taking a still image
    bool m_stillImageTriggered ;
  } ;

}

#endif // MAINWINDOW_H
