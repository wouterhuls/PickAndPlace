#ifndef METROLOGYPAGES_H
#define METROLOGYPAGES_H

#include "Coordinates.h"
#include <QWidget>
#include <QTabWidget>

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
  class SideMetrologyPage : public QTabWidget
  {
  public:
    SideMetrologyPage( CameraWindow& camerasvc, ViewDirection view ) ;
    void reset() ;
  } ;
  
  QWidget* createPhotoBoothPage(CameraWindow& camerasvc, ViewDirection viewdir) ; 
}

#endif
