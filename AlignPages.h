#ifndef ALIGNPAGES_H
#define ALIGNPAGES_H

#include "CoordinateMeasurement.h"
#include <QWidget>
class QLabel ;
class QPlainTextEdit ;
class QTableWidget ;
class QVideoFrame ;
class QGraphicsItem ;

namespace PAP
{
  class CameraView ;
  class CameraWindow ;
  
  class MarkerRecorderWidget : public QWidget
  {
    Q_OBJECT
  public:
    enum Status { Uninitialized=0, Active, Recorded, Calibrated} ;
    MarkerRecorderWidget(const QString& markername,
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
    void findMarker( const QVideoFrame& frame ) ;
  private:
    bool m_triggerMarkerFinder{false} ;
    QGraphicsItem* m_measuredmarker{0} ;
  } ;
  
  // helper class for page for alignment of the tiles
  class AlignTilePage : public QWidget
  {
     Q_OBJECT
  public:
     AlignTilePage(PAP::CameraView* camview,const TileInfo& tileinfo) ;
  private:
    MarkerRecorderWidget* m_marker1recorder ;
    MarkerRecorderWidget* m_marker2recorder ;
    CameraView* m_cameraview ;
    QString m_tilename ;
    QPlainTextEdit* m_textbox ;
  public slots:
    void updateAlignment() const ;
  } ;

  QWidget* makeAlignMainJigZPage(ViewDirection view, CameraWindow& camerasvc) ;
}




#endif
