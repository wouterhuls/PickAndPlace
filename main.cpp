#include <QApplication>
#include <QDebug>

#include <QSerialPort>
#include <QSerialPortInfo>

#include <QCameraInfo>

#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>

#include "MotionSystemSvc.h"
#include "MotionSystemWidget.h"
#include "PropertySvc.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    // This part I do not understand yet: I do not understand what you would to do in the UI and what in the C++.

    //MainWindow w;
    //w.show();



    // list the serial ports
    QSerialPort gpibserial{} ;

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
            qDebug() << "Name : " << info.portName();
            qDebug() << "Description : " << info.description();
            qDebug() << "Manufacturer: " << info.manufacturer();
            qDebug() << "Boud rates:" << info.standardBaudRates();

            // Example use QSerialPort
            if( info.portName()=="cu.usbserial-PX9I7SZ6") {
                gpibserial.setPort(info)  ;
            }
            //QSerialPort serial;
            //serial.setPort(info);
            //if (serial.open(QIODevice::ReadWrite))
            //    serial.close();
        }

    if( gpibserial.portName().size()>0 ) {
        qDebug() << "Trying to open port" ;
        if (gpibserial.open(QIODevice::ReadWrite)) {
            qDebug() << "Opened the gpib port" ;
            gpibserial.write("++ver\n") ;
            QByteArray readData = gpibserial.readAll();
            while (gpibserial.waitForReadyRead(500))
                readData.append(gpibserial.readAll());
            // it seems that it never receives a 'waitForReadyRead' signal. maybe that's a matter of configuring the thing right.
            qDebug() << "Error: " << gpibserial.errorString() ;
            qDebug() << readData ;
            gpibserial.close();
        }
    }

    // list the cameras
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, cameras) {
        qDebug() << "camera description: "
                 << cameraInfo.description() ;
        qDebug() << "camera name: "
                 << cameraInfo.deviceName() ;
        //if (cameraInfo.deviceName() == "mycamera")
        //    camera = new QCamera(cameraInfo);
    }

    // let's create a mainwindow, with vertical layout, for all axis widgets
    QMainWindow mainwindow ;
    mainwindow.resize(600,500) ;
    /* auto centralWidget =*/
    new PAP::MotionSystemWidget(&mainwindow);

    //mainwindow.setLayout( layout ) ;
    mainwindow.show() ;

    auto rc = a.exec() ;

    PAP::PropertySvc::instance()->write("papconfig_last.txt") ;
    
    return rc;
}
