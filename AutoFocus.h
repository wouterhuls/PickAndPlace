#ifndef __AUTOFOCUS_HH__
#define __AUTOFOCUS_HH__

#include <QDialog>
#include "NamedValue.h"
#include "MotionAxis.h"
class QImage ;
class QVideoFrame ;
class QLabel ;
namespace QtCharts
{
  class QChart ;
  class QXYSeries ;
  class QChartView ;
};

namespace PAP
{
  class CameraView ;
  class MotionAxis ;
  struct AutoFocusSettingsWidget ;
  
  struct FocusMeasurement
  {
    FocusMeasurement( double _z=0, double _I=0) : z(_z),I(_I) {}
    MotionAxis::Direction zdir ;
    double z ;
    double I ;
    bool operator<(const FocusMeasurement& rhs) const { return z < rhs.z ; }
  } ;

  enum FocusSeriesType { None, Rising=1, Falling=2, Both=3 } ;

  class AutoFocus : public QDialog
  {

    Q_OBJECT
    
  public:
    AutoFocus(CameraView*, QWidget* parent );
    virtual ~AutoFocus() ;
    const QImage* focusImage() const { return m_focusImage ; }
    double computeContrast( const QVideoFrame& frame ) ;

    // temporary, before we put this int he dialog
    QLabel* focusView() { return m_focusView ; }

    void startFocusSequence() ;
    void startNearFocusSequence() ;
    void startFastFocusSequence() ;
    void startFastFocusSequenceSimple() ;
    void moveFocusTo( double z ) const ;
   
  signals:
    void focusMeasureUpdated() ;
    void focusMeasurement(PAP::FocusMeasurement result) ;
    void focussed() ;
    
  public slots:      
    void processFrame( const QVideoFrame& frame ) ;
    void storeMarkerFocus() ;
    void applyMarkerFocus() const ;
    void analyseFastFocus( PAP::FocusMeasurement result ) ;
    void analyseSlowFocus( PAP::FocusMeasurement result ) ;
  private:
    FocusMeasurement takeMeasurement( MotionAxis& axis, double zpos ) ;
  
  private:
    CameraView* m_cameraView{0} ;
    QImage* m_focusImage{0} ;
    QLabel* m_focusView{0} ;

    AutoFocusSettingsWidget* m_settings{0} ;
    
    // series with focus data
    QtCharts::QXYSeries* m_focusmeasurements{nullptr} ;
    QtCharts::QChart* m_focuschart{nullptr} ;
    QtCharts::QChartView* m_chartview{nullptr} ;
    
    //
    MotionAxis* m_zaxis{0} ;
    bool m_isFocussing{false} ;
    bool m_focusTriggered{false} ;
    // some data members that I need to communicate between slots
    std::vector<FocusMeasurement> m_fastfocusmeasurements ;
    FocusSeriesType m_focusseriestype ;
    FocusMeasurement m_bestfocus ;

    // map that stores markers with focus point
    std::map< QString,NamedDouble> m_markerfocuspoints[2] ;
  } ;

} ;

Q_DECLARE_METATYPE(PAP::FocusMeasurement) ;

#endif
