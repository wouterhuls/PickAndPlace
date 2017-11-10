#-------------------------------------------------
#
# Project created by QtCreator 2017-08-16T20:24:35
#
#-------------------------------------------------

QT       += core gui serialport multimedia multimediawidgets testlib

#INCLUDEPATH+=$HOME/surfdrive/QtProjects
INCLUDEPATH+=/usr/local/include

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PickAndPlace
TEMPLATE = app

SOURCES += main.cpp \
    MotionSystemSerialPort.cpp \
    MotionControllerWidget.cpp \
    MotionSystemSvc.cpp \
    MotionAxis.cpp \
    MotionAxisWidget.cpp \
    MotionAxisSettingsDialog.cpp \
    MotionController.cpp \
    MotionSystemWidget.cpp \
    Console.cpp \
    SerialPortSettingsDialog.cpp \
    PropertySvc.cpp \
    CameraView.cpp \
    CameraWindow.cpp \
    AutoFocus.cpp \
    NamedValueInputWidget.cpp \
    GeometrySvc.cpp \
    NamedValue.cpp 

HEADERS  += \
    NominalMarkers.h \
    GraphicsItems.h \
    MotionSystemSerialPort.h \
    MonitoredValue.h \
    NamedValue.h \
    NamedValueInputWidget.h \
    MotionControllerWidget.h \
    MotionSystemSvc.h \
    MotionSystemCommandLibrary.h \
    MotionAxisParameters.h \
    MotionAxis.h \
    MotionAxisWidget.h \
    MotionAxisSettingsDialog.h \
    MotionController.h \
    MotionSystemWidget.h \
    Console.h \
    SerialPortSettingsDialog.h \
    Singleton.h \
    PropertySvc.h \
    CameraWindow.h \
    CameraView.h \
    AutoFocus.h

FORMS    += SerialPortSettingsDialog.ui

#mainwindow.ui \
#    MotionControllerWidget.ui

RESOURCES += \
    PickAndPlace.qrc
