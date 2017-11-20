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

  class MarkerRecorderWidget : public QWidget
  {
    Q_OBJECT
  public:
    enum Status { Uninitialized=0, Active=1, Ready=2 } ;
    MarkerRecorderWidget(const char* markername,
			 const PAP::CameraView* camview,
			 QWidget* parent=0) ;
    const CoordinateMeasurement& measurement() const { return m_measurement ; }
    const QPointF& markerposition() const { return m_markerposition; }
    double dx() const { return m_markerposition.x() - m_measurement.globalcoordinates.x; }
    double dy() const { return m_markerposition.y() - m_measurement.globalcoordinates.y; }
    Status status() const { return m_status ; }
    void reset() { setStatus(Uninitialized) ; }
    void setStatus( Status s ) ;
  signals:
    void ready() ;
  public slots:
    void record( CoordinateMeasurement m) ;
    void on_recordbutton_toggled(bool checked) {
      if( checked ) setStatus(Active) ;
      else setStatus(Uninitialized) ;
    }
  private:
    const PAP::CameraView* m_cameraview ;
    Status m_status ;
    QLabel* m_statuslabel ;
    CoordinateMeasurement m_measurement ;
    QPointF m_markerposition ;
  } ;

  
}

#endif // MAINWINDOW_H
