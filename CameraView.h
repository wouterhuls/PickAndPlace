#ifndef WH_CAMERAVIEW_H
#define WH_CAMERAVIEW_H

#include <QWidget>
#include <QMainWindow>
#include <QGraphicsView>
#include "NamedValue.h"

class QGraphicsScene ;
class QGraphicsView ;
class QGraphicsVideoItem ;
class QCamera ;
class QCameraInfo ;
class QCameraViewfinder ;
class QVideoProbe ;
class QVideoFrame ;

namespace PAP
{

  class CameraView : public QGraphicsView
  {
    Q_OBJECT
    
  public:
    explicit CameraView(QWidget *parent = 0);
    ~CameraView();
    void setCamera(const QCameraInfo &cameraInfo) ;

    double pixelSizeX() const { return m_chipPixelSize / m_magnification ; }
    double pixelSizeY() const { return m_chipPixelSize / m_magnification ; }

    double computeContrast( QVideoFrame& frame ) const ;
  public slots:
    virtual void wheelEvent ( QWheelEvent * event ) ;
    void scalingTime(qreal x) ;
    void animFinished() ;
    virtual void mousePressEvent( QMouseEvent* event) ;
    void processFrame( QVideoFrame& frame ) ;
    void moveCameraTo( QPointF localpoint ) const ;
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
        
    //QCameraViewfinder* m_viewfinder ;
    //QGraphicsTextItem m_cursorpos ;

    // some info on camera
    QPointF m_localOrigin ;
    int m_numPixelsX ;
    int m_numPixelsY ;
    const double m_chipPixelSize ;
    NamedDouble m_magnification ;
    NamedDouble m_rotation ;
    
    // needed for the smooth zoom function
    int m_numScheduledScalings ;
    
  };
}

#endif // MAINWINDOW_H
