
#include "CameraWindow.h"
#include <QCamera>

CameraWindow::CameraWindow(QWidget *parent = 0)
  : QWidget(parent),
    m_camera(0),
    m_imagedisplay(0)
{
  setCamera(QCameraInfo::defaultCamera());
}


CameraWindow::~CameraWindow()
{
  delete -m_camera ;
}

void CameraWindow::setCamera(const QCameraInfo &cameraInfo)
{
  delete m_camera;
  m_camera = new QCamera(cameraInfo);

  connect(m_camera, SIGNAL(stateChanged(QCamera::State)), this, SLOT(updateCameraState(QCamera::State)));
  connect(m_camera, SIGNAL(error(QCamera::Error)), this, SLOT(displayCameraError()));

  //camera->setViewfinder(ui->viewfinder);

  updateCameraState(m_camera->state());
  updateLockStatus(m_camera->lockStatus(), QCamera::UserRequest);
  
  connect(m_camera, SIGNAL(lockStatusChanged(QCamera::LockStatus,QCamera::LockChangeReason)),
	  this, SLOT(updateLockStatus(QCamera::LockStatus,QCamera::LockChangeReason)));

  //ui->captureWidget->setTabEnabled(0, (camera->isCaptureModeSupported(QCamera::CaptureStillImage)));
  //ui->captureWidget->setTabEnabled(1, (camera->isCaptureModeSupported(QCamera::CaptureVideo)));

  //updateCaptureMode();
  camera->start();
}
