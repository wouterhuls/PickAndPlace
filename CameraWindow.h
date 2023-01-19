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

  // Definition of CameraWindow class
  class CameraWindow : public QMainWindow
  {
    Q_OBJECT
    
  public:
    explicit CameraWindow(QWidget *parent = 0);
    ~CameraWindow() {}
    CameraView* cameraview() { return m_cameraview ; }
    AutoFocus* autofocus() ;
    //MetrologyReport* measurementreport() { return m_measurementreport ; }

    QString moduleName() const { return m_moduleName.value(); }
    QString moduleDataDir() const {
      return QString{"/home/velouser/Documents/PickAndPlaceData/"} + moduleName() + "/" ; }
  signals:
    void viewToggled( int view ) ;
    
  public slots:
    void on_focusButton_clicked() ;
    //void on_quitButton_clicked();
    void on_stopButton_clicked();
    void processFrame( const QVideoFrame& frame ) ;
    void moveToMarker() ;
    void moveToPositionInModuleFrame() ;
    void moveCameraToPointInModule( ModuleCoordinates modulepoint ) ;
    
  private:
    void createMenus() ;
  private:
    CameraView* m_cameraview{0} ;
    QDialog*   m_motionsystemdialog{0} ;
    //MetrologyReportPage* m_metrologyreport{0} ;
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
