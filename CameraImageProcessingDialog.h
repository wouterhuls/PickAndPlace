#ifndef CAMERA_IMAGE_PROCESSING_WIDGET
#define CAMERA_IMAGE_PROCESSING_WIDGET

#include <QDialog>
class QCameraImageProcessing ;

namespace PAP
{
  
  class CameraImageProcessingDialog : public QDialog
  {
  private:
    QCameraImageProcessing* m_ip ;
  public:
    CameraImageProcessingDialog( QCameraImageProcessing& ip,
				 QWidget* parent) ;
  } ;
}

#endif
