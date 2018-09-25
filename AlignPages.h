#ifndef ALIGNPAGES_H
#define ALIGNPAGES_H

#include "CoordinateMeasurement.h"
#include <QWidget>
class QLabel ;
class QPlainTextEdit ;
class QTableWidget ;

namespace PAP
{
  class CameraView ;
  class CameraWindow ;
  
  class MarkerRecorderWidget : public QWidget
  {
    Q_OBJECT
  public:
    enum Status { Uninitialized=0, Active, Recorded, Calibrated} ;
    MarkerRecorderWidget(const char* markername,
			 const PAP::CameraView* camview,
			 QWidget* parent=0) ;
    const CoordinateMeasurement& measurement() const { return m_measurement ; }
    const QPointF& markerposition() const { return m_markerposition; }
    double dx() const { return m_markerposition.x() - m_measurement.globalcoordinates.x(); }
    double dy() const { return m_markerposition.y() - m_measurement.globalcoordinates.y(); }
    Status status() const { return m_status ; }
    void reset() { setStatus(Uninitialized) ; }
    void setStatus( Status s ) ;
    //signals:
    //void ready() ;
  public slots:
    void record( const CoordinateMeasurement& m) ;
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

  class AlignMainJigPage : public QWidget
  {
    Q_OBJECT
    
  private:
    ViewDirection m_viewdirection ;
    CameraView* m_cameraview ;
    MarkerRecorderWidget* m_marker1recorder ;
    MarkerRecorderWidget* m_marker2recorder ;
    QPlainTextEdit* m_textbox ;
  public:
    AlignMainJigPage(ViewDirection dir, PAP::CameraView* camview) ;
  public slots:
    void updateAlignment() const ;
  } ;
  
  // helper class for page for alignment of the tiles
  class AlignTilePage : public QWidget
  {
     Q_OBJECT
  public:
    AlignTilePage(PAP::CameraView* camview,
		  const char* tilename,
		  const char* marker1, const char* marker2) ;
  private:
    MarkerRecorderWidget* m_marker1recorder ;
    MarkerRecorderWidget* m_marker2recorder ;
    CameraView* m_cameraview ;
    QString m_tilename ;
    QPlainTextEdit* m_textbox ;
  public slots:
    void updateAlignment() const ;
  } ;

  class AlignMainJigZPage : public QWidget
  {
  private:
    enum Status { Inactive, Active, Calibrated } ;
    ViewDirection m_viewdirection ;
    CameraWindow* m_camerasvc{0} ;
    Status m_status{Inactive} ;
    const std::vector<MSMainCoordinates> refcoordinates{ {-85,-10},{-85,120},{80,120},{80,-10} } ;
    std::vector<MSCoordinates> m_measurements ;
    std::vector<QMetaObject::Connection> m_conns ;
    QTableWidget* m_measurementtable{0} ;
  public:
    AlignMainJigZPage(ViewDirection view, CameraWindow& camerasvc) ;
  private:
    void disconnectsignals() ;
    void connectsignals() ;
    void move() ;
    void focus() ;
    void measure() ;
    void calibrate() ;
  } ;
  
}




#endif
