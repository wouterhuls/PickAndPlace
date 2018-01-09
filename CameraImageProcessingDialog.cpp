#include "CameraImageProcessingDialog.h"

#include <QSlider>
#include <QCameraImageProcessing>
#include <QVBoxLayout>

namespace PAP
{
  
  class IPSlider : public QSlider
  {
  private:
    
  public:
  IPSlider(QCameraImageProcessing& ip,
	   void (QCameraImageProcessing::*setter)(qreal),
	   qreal (QCameraImageProcessing::*getter) () const,
	   double minval,
	   double maxval,
	   int numbins)
    : QSlider(Qt::Horizontal)
      {
	setTickPosition( QSlider::TicksBelow ) ;
	setMinimum(0) ;
	setMaximum(numbins) ;
	double offset = minval ;
	double delta  = (maxval-minval)/numbins ;
	double curr = (ip.*getter)() ;
	setSliderPosition( (curr-offset)/delta ) ;
	connect(this,&QAbstractSlider::sliderMoved,
		[&]() { (ip.*setter)(this->sliderPosition() * delta + offset) ; } ) ;
      }
  } ;
  
  CameraImageProcessingDialog::CameraImageProcessingDialog( QCameraImageProcessing& ip,
							    QWidget* parent)
    : QDialog(parent), m_ip(&ip)
  {
    resize(600,600) ;
    setWindowTitle("CameraSettings") ;
    
    auto layout = new QVBoxLayout{} ;
    this->setLayout( layout ) ;
    
    //QStyleOptionSlider sliderstyle ;
    auto contrastslider = new IPSlider{*m_ip,
				       &QCameraImageProcessing::setContrast,
				       &QCameraImageProcessing::contrast,
				       -1.0,1.0,100} ;
    layout->addWidget( contrastslider ) ;
    
  }
}
