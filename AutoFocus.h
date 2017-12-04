#ifndef __AUTOFOCUS_HH__
#define __AUTOFOCUS_HH__

#include <QDialog>
class QImage ;
class QVideoFrame ;
class QLabel ;
namespace QtCharts
{
  class QChart ;
  class QLineSeries ;
};

namespace PAP
{
  class CameraView ;
  class MotionAxis ;
  struct FocusMeasurement
  {
    FocusMeasurement( double _z=0, double _I=0) : z(_z),I(_I) {}
    double z ;
    double I ;
    bool operator<(const FocusMeasurement& rhs) const { return z < rhs.z ; }
  } ;
    
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
    
  signals:
    void focusMeasureUpdated() ;
    
  public slots:      
    void processFrame( const QVideoFrame& frame ) ;
  private:
    FocusMeasurement takeMeasurement( MotionAxis& axis, double zpos ) ;
  
  private:
    QImage* m_focusImage ;

    QLabel* m_focusView ;
   
    // series with focus data
    QtCharts::QLineSeries* m_focusmeasurements ;
    QtCharts::QChart* m_focuschart ;
    
    //
    bool m_isFocussing ;
    bool m_focusTriggered ;
    // the last focus measurement. at some point we can put this in a signal.
    FocusMeasurement m_focusMeasurement ;
  } ;

} ;


#endif
