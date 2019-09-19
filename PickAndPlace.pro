#-------------------------------------------------
#
# Project created by QtCreator 2017-08-16T20:24:35
#
#-------------------------------------------------

QT       += core gui serialport multimedia multimediawidgets testlib charts


QMAKE_MAC_SDK = macosx10.14
CONFIG+=sdk_no_version_check

#INCLUDEPATH+=$HOME/surfdrive/QtProjects
INCLUDEPATH+=/usr/local/include

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PickAndPlace
TEMPLATE = app

PKGCONFIG += opencv4
CONFIG += link_pkgconfig

SOURCES += main.cpp \
    MotionSystemSerialPort.cpp \
    MotionControllerWidget.cpp \
    MotionSystemSvc.cpp \
    MotionAxis.cpp \
    MotionAxisWidget.cpp \
    MotionAxisSettingsDialog.cpp \
    MotionController.cpp \
    MotionSystemWidget.cpp \
    MotionAxisParameter.cpp \
    Console.cpp \
    SerialPortSettingsDialog.cpp \
    PropertySvc.cpp \
    CameraView.cpp \
    CameraWindow.cpp \
    AutoFocus.cpp \
    NamedValueInputWidget.cpp \
    GeometrySvc.cpp \
    MonitoredValue.cpp \
    AlignPages.cpp \
    MotionSystemCalibration.cpp \
    StackCalibration.cpp \
    CameraImageProcessingDialog.cpp \
    MetrologyPages.cpp

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
    MotionSystemTypes.h \
    MotionAxisParameter.h \
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
    AutoFocus.h \
    AlignPages.h\
    MotionSystemCalibration.h \
    StackCalibration.h \
    MonitoredValueLabel.h \
    MetrologyReport.h \
    MetrologyPages.h 

FORMS    += SerialPortSettingsDialog.ui

#mainwindow.ui \
#    MotionControllerWidget.ui

RESOURCES += \
    PickAndPlace.qrc

