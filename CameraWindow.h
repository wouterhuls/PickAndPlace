#ifndef WH_CAMERAWINDOW_H
#define WH_CAMERAWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include "CoordinateMeasurement.h"
#include "Singleton.h"
#include "NamedValue.h"

class QLabel;
class QCheckBox ;
class QVideoFrame;
class QPushButton ;

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
    CameraView* m_cameraview{0} ;
    AutoFocus* m_autofocus{0} ;
    MeasurementReportPage* m_measurementreport{0} ;
    // some labels that tell where the cursor position is
    NamedValue<QString> m_moduleName ;
    QPushButton* m_moduleNameButton{0} ;
    QLabel* m_cursorposition{0} ;
    QCheckBox* m_showNSideTiles{0} ;
    QCheckBox* m_showCSideTiles{0} ;
    
    // boolean to trigger taking a still image
    bool m_stillImageTriggered{false} ;
  } ;

}

#endif // MAINWINDOW_H
