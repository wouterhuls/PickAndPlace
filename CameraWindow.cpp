#include "CameraWindow.h"
#include "CameraView.h"

namespace PAP
{
  CameraWindow::CameraWindow(QWidget *parent)
    : QMainWindow(parent)
  {
    resize(700,700);
    m_cameraview = new CameraView{this} ;
    m_cameraview->show() ;
  }
}
