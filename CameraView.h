#ifndef WH_CAMERAVIEW_H
#define WH_CAMERAVIEW_H

#include <QWidget>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsItemGroup>
#include "NamedValue.h"
#include "Coordinates.h"

class QGraphicsScene ;
class QGraphicsView ;
class QGraphicsVideoItem ;
class QCamera ;
class QCameraInfo ;
class QCameraViewfinder ;
class QVideoProbe ;
class QVideoFrame ;
class QGraphicsRectItem ;
class QGraphicsSimpleTextItem ;

namespace PAP
{
  class CoordinateMeasurement ;
  class Marker ;
  
  class CameraView : public QGraphicsView
  {
    Q_OBJECT
    
  public:
    explicit CameraView(QWidget *parent = 0);
    ~CameraView();
    enum MovementType { AbsolutePosition, RelativePosition } ;
    void setCamera(const QCameraInfo &cameraInfo) ;

    // return the pixelsize = microns
    double pixelSize() const { return m_chipPixelSize / m_magnification ; }
    double pixelSizeX() const { return pixelSize() ; }
    double pixelSizeY() const { return pixelSize() ; }
    
    MonitoredQPointF& cameraCentreInModuleFrame() { return m_cameraCentreInModuleFrame ; }
    
    QVideoProbe* videoProbe() { return m_videoProbe ; }

    QGraphicsItemGroup* nsidemarkers() { return m_nsidemarkers; }
    QGraphicsItemGroup* csidemarkers() { return m_nsidemarkers; }

    void updateGeometryView() ;
    void updateStackAxisView() ;
    void resetCamera() ;
    //void lockWhiteBalance( bool lock = true ) ;

    QTransform fromCameraToPixel() const
    {
      // Also here, take into account that we apply operators from left to right, rather than vice versa:
      QTransform T ;
      T.translate( m_localOrigin.x(), m_localOrigin.y() ) ;
      QTransform S ;
      S.scale( 1.0/pixelSizeX(), -1.0/pixelSizeY() ) ;
      return  S * T;
      // The following solution may be a bit faster:
      // QTransform T ;
      // T.scale( 1.0/pixelSizeX(), -1.0/pixelSizeY() ) ;
      // T.translate( m_localOrigin.x()*pixelSizeX(), -m_localOrigin.y()*pixelSizeY() ) ;
      // return T ;
    }

    ViewDirection currentViewDirection() const { return m_currentViewDirection ; }

    const QPointF& localOrigin() const { return m_localOrigin ; }

    QCamera* camera() { return m_camera ; }

    const PAP::Marker* closestMarker( const QPointF& localpoint ) const ;
    const PAP::Marker* closestMarker() const { return closestMarker(localOrigin()) ; }
    QString closestMarkerName() const ;
    QStringList visibleMarkers() const ;
    /// Move to a particular marker
    void moveCameraTo( const QString& name ) const ;
  signals:
    void recording( const CoordinateMeasurement& ) const ;
    
  public slots:
    virtual void wheelEvent ( QWheelEvent * event ) ;
    void scalingTime(qreal x) ;
    void animFinished() ;
    virtual void mousePressEvent( QMouseEvent* event) ;
    void moveCameraTo( QPointF localpoint, MovementType mode) const ;
    
    /// Return the position of a marker in global coordinates
    QPointF globalPosition( const QString& name ) const ;
    
    void record( QPointF localpoint ) const ;
    void zoomReset() ;
    void zoomOut() ;
    void setViewDirection( ViewDirection view ) ;

    void showNSideMarkers( int state ) { m_nsidemarkers->setVisible( state>0 ) ; }
    void showCSideMarkers( int state ) { m_csidemarkers->setVisible( state>0 ) ; }
    
  private:
    QCamera* m_camera ;
    QGraphicsScene* m_scene ; // a graphics scene that
    // displayes the image and a
    // cross. if I understand it
    // well, then it can also tell
    // where the cursor is and do
    // things like zoom. this is how
    // I want to solve it but you
    // cannot put a viewfinder in a
    // graphicsscene, so that
    // requires some development.
    //QGraphicsView* m_view ;
    QGraphicsVideoItem* m_viewfinder ;
    QVideoProbe* m_videoProbe ;
    QGraphicsRectItem* m_viewfinderborder ;
        
    //QCameraViewfinder* m_viewfinder ;
    //QGraphicsTextItem m_cursorpos ;
    
    // some info on camera view
    QPointF m_localOrigin ;
    int m_numPixelsX ;
    int m_numPixelsY ;
    const double m_chipPixelSize ;
    NamedDouble m_magnification ;
    NamedDouble m_rotation ;
    double m_nominalscale ;

    // some info on the view direction
    ViewDirection m_currentViewDirection ;
    QGraphicsItemGroup* m_detectorgeometry ;
    QGraphicsItemGroup* m_nsidemarkers ;
    QGraphicsItemGroup* m_csidemarkers ;
    QGraphicsItem* m_stackaxis ;
    
    // needed for the smooth zoom function
    int m_numScheduledScalings ;

    // text with name of closest marker
    QGraphicsSimpleTextItem* m_markertext ;
    
    // derived parameters for the camera centre in the module frame
    MonitoredQPointF m_cameraCentreInModuleFrame ;
  };
}

#endif // MAINWINDOW_H
