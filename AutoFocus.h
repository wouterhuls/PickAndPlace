#ifndef __AUTOFOCUS_HH__
#define __AUTOFOCUS_HH__

#include <QDialog>
class QImage ;
class QVideoFrame ;
class QLabel ;

namespace PAP
{
  class CameraView ;
  
  class AutoFocus : public QDialog
  {

    Q_OBJECT
    
  public:
    AutoFocus(CameraView*, QWidget* parent );
    virtual ~AutoFocus() ;
    const QImage* focusImage() const { return m_focusImage ; }
    double focusMeasure() const { return m_focusMeasure ; }
    double computeContrast( const QVideoFrame& frame ) ;

    // temporary, before we put this int he dialog
    QLabel* focusView() { return m_focusView ; }

    void startFocusSequence() ;
    
  signals:
    void focusMeasureUpdated() ;
    
  public slots:      
    void processFrame( const QVideoFrame& frame ) ;
    
  private:
    QImage* m_focusImage ;

    QLabel* m_focusView ;
   
    // current value of picture contrast or entropy or whatever
    double m_focusMeasure ;

    //
    bool m_isFocussing ;
    
  } ;

} ;


#endif
