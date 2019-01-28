#ifndef METROLOGYPAGES_H
#define METROLOGYPAGES_H

#include "Coordinates.h"

class QWidget ;
class QTabWidget ;

namespace PAP
{
  class CameraWindow ;
  QWidget* createTileMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
  QWidget* createSensorSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir, QString tilename) ;
  QWidget* createSubstrateSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
  QWidget* createGenericSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
  QTabWidget* createSideMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
}

#endif
