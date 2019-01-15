#ifndef METROLOGYPAGES_H
#define METROLOGYPAGES_H

#include "Coordinates.h"

class QWidget ;

namespace PAP
{
  class CameraWindow ;
  QWidget* createTileMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
  QWidget* createSensorSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir, const char* tilename) ;
  QWidget* createSubstrateSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
  QWidget* createGenericSurfaceMetrologyPage(CameraWindow& camerasvc, ViewDirection viewdir) ;
}

#endif
