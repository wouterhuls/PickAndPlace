#include "CameraImageProcessingDialog.h"

#include <QSlider>
#include <QCameraImageProcessing>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace PAP
{
  
  class IPSlider : public QSlider
  {
  private:
    QCameraImageProcessing* m_ip ;
  public:
  IPSlider(QCameraImageProcessing& ip,
	   void (QCameraImageProcessing::*setter)(qreal),
	   qreal (QCameraImageProcessing::*getter) () const,
	   double minval,
	   double maxval,
	   int numbins)
    : QSlider(Qt::Horizontal),
      m_ip(&ip)
      {
	setTickPosition( QSlider::TicksBelow ) ;
	setMinimum(0) ;
	setMaximum(numbins) ;
	double offset = minval ;
	double delta  = (maxval-minval)/numbins ;
	double curr = (ip.*getter)() ;
	setSliderPosition( (curr-offset)/delta ) ;
	connect(this,&QAbstractSlider::sliderMoved,
		[=]() { (m_ip->*setter)(this->sliderPosition() * delta + offset) ; } ) ;
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
    
    auto tmptext1 = new QLabel{this} ;
    tmptext1->setText("Unfortunately the camera does not support imageprocessing parameters.") ;
    tmptext1->setWordWrap(true);
    layout->addWidget( tmptext1 ) ;
        
 
    auto wbbutton = new QPushButton{"Auto white Balance"} ;
    wbbutton->setCheckable(true) ;
    connect(wbbutton,&QPushButton::clicked,
	    [=]() {
	      m_ip->setWhiteBalanceMode( wbbutton->isChecked()
					 ? QCameraImageProcessing::WhiteBalanceAuto
					 : QCameraImageProcessing::WhiteBalanceManual ) ; } ) ;
    layout->addWidget( wbbutton ) ;
    
    m_ip->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceManual) ;
    

    
    layout->addWidget(new IPSlider{*m_ip,
	  &QCameraImageProcessing::setManualWhiteBalance,
	  &QCameraImageProcessing::manualWhiteBalance,
	  0,5000,1000}) ;

       //QStyleOptionSlider sliderstyle ;
    layout->addWidget( new IPSlider{*m_ip,
	  &QCameraImageProcessing::setContrast,
	  &QCameraImageProcessing::contrast,
	  -1.0,1.0,100} ) ;
    
  }
}
