#ifndef MOTIONAXISSETTINGSDIALOG_H
#define MOTIONAXISSETTINGSDIALOG_H

#include <QDialog>
#include "MotionAxis.h"

namespace PAP
{
  
  class MotionAxisSettingsDialog : public QDialog
  {
    Q_OBJECT
    
  public:
    explicit MotionAxisSettingsDialog(MotionAxis& axis, QWidget *parent = 0);
    virtual ~MotionAxisSettingsDialog() {}
    
    // need to think if you want to directly touch the parameters, or
    // rather have a local copy with an 'apply' button.
    
    // to show the widget, call the function show( thingy ) in the parent
    // to hide the widget, call the 'hide()' function from your apply/cancel button
    // for the setting of numbers, use the 'DoubleSpinBox'
    
    // we want buttons to
    // - search home
    // - set 'zero' (?)
    // - set the step size
    // - set the velocity
    // before we can set them, we need to be able to read them from the device
    
    private slots:
      void on_searchHomeButton_clicked();
      
  private:
      MotionAxis* m_axis ;
  };

}


#endif
