#include <QApplication>
#include <QDebug>

#include <QSerialPort>
#include <QSerialPortInfo>

#include <QCameraInfo>

#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>

#include <QProcessEnvironment>

#include "MotionSystemSvc.h"
#include "MotionSystemWidget.h"
#include "PropertySvc.h"
#include "CameraWindow.h"

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  QByteArray localMsg = msg.toLocal8Bit();
  switch (type) {
  case QtDebugMsg:
    //fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    fprintf(stderr, "Debug: %s\n", localMsg.constData()) ;
    break;
  case QtInfoMsg:
    fprintf(stderr, "Info: %s\n", localMsg.constData()) ;
    break;
  case QtWarningMsg:
    fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    break;
  case QtCriticalMsg:
    fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    break;
  case QtFatalMsg:
    fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    abort();
  }
}

int main(int argc, char *argv[])
{
  qInstallMessageHandler(myMessageOutput);
  QApplication a(argc, argv);
  
  
  // This part I do not understand yet: I do not understand what you would to do in the UI and what in the C++.
  
  //MainWindow w;
  //w.show();
  
  
  
  // let's create a mainwindow, with vertical layout, for all axis widgets
  // QMainWindow mainwindow;
  // mainwindow.resize(600,500) ;
  // mainwindow.move(50,0) ;
  // mainwindow.setWindowIcon( QIcon(":/images/VeloUpgradeLogoSmall.png") ) ;
  // mainwindow.setWindowTitle("Motion System Window") ;
    
  // // auto centralWidget =
  // new PAP::MotionSystemWidget(&mainwindow);
  
  // //mainwindow.setLayout( layout ) ;
  // mainwindow.show() ;
  
  // For now we put the camera window in a separate main window. In
  // the end, we'll always want the camera window, but the MS
  // controls we only need on demand.
  
  PAP::CameraWindow camerawindow ;
  camerawindow.move(900,0) ;
  camerawindow.show() ;
  camerawindow.setWindowIcon( QIcon(":/images/VeloUpgradeLogoSmall.png") ) ;
  
  // load the setup file
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment() ;
  QString filename = env.value("HOME") + "/.papconfig" ;
  PAP::PropertySvc::instance()->read(filename.toLatin1()) ;
  auto rc = a.exec() ;
  PAP::PropertySvc::instance()->write("papconfig_last.txt") ;
  
  return rc;
}
