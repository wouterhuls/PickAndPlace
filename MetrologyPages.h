#ifndef METROLOGYPAGES_H
#define METROLOGYPAGES_H

#include "Coordinates.h"
#include <QWidget>

class QTabWidget ;

namespace PAP
{

  // Annoying: I need a metrology page base classjust to introduce signals outside the cpp file
  
  class MarkerMetrologyPageBase : public QWidget
  {
    Q_OBJECT
  public:
    using QWidget::QWidget ;
    virtual ~MarkerMetrologyPageBase() {}
  signals:
    void autoready() ;
  } ;
  
  
  class CameraWindow ;
  QWidget* createTileMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
  QWidget* createSensorSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir, QString tilename) ;
  QWidget* createSubstrateSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
  QWidget* createGenericSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
  QTabWidget* createSideMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;

  QWidget* createPhotoBoothPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
  
}

#endif
