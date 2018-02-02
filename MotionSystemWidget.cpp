#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLCDNumber>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>

#include "MotionSystemSvc.h"
#include "MotionAxisWidget.h"
#include "MotionControllerWidget.h"
#include "MotionSystemWidget.h"
#include "MotionSystemCalibration.h"

namespace PAP
{

  MotionSystemWidget::MotionSystemWidget( QWidget *parent ) :
    QWidget(parent)
  {
    //
    this->setWindowTitle("Motion System Controls") ;
    this->setObjectName("MotionSystemWidget") ;
    resize(600,500) ;
    
    // add a vertical layout
    auto layout = new QVBoxLayout{} ;//(widget);
    layout->setObjectName(QStringLiteral("layout"));
    layout->setContentsMargins(0, 0, 0, 0);
    
    // add widgets for the controllers
    auto mssvc = MotionSystemSvc::instance() ;
    for( auto& c : mssvc->controllers() ) {
      auto cw = new MotionControllerWidget( *c.second, this ) ;
      layout->addWidget( cw ) ;
    }
    
    // now add the widgets for all the axes
    layout->addWidget( new MotionAxisWidget{ mssvc->focusAxis(),this } ) ;
    layout->addWidget( new MotionAxisWidget{ mssvc->mainXAxis(),this } ) ;
    layout->addWidget( new MotionAxisWidget{ mssvc->mainYAxis(),this } ) ;
    layout->addWidget( new MotionAxisWidget{ mssvc->stackXAxis(),this } ) ;
    layout->addWidget( new MotionAxisWidget{ mssvc->stackYAxis(),this } ) ;
    layout->addWidget( new MotionAxisWidget{ mssvc->stackRAxis(),this } ) ;

    auto extrabuttonlayout = new QGridLayout{} ;
    layout->addLayout( extrabuttonlayout ) ;

    auto parkbutton = new QPushButton("Park position",this) ;
    connect(parkbutton,&QPushButton::clicked,[=](){
	mssvc->mainXAxis().moveTo( double(mssvc->mainXAxis().leftTravelLimit()+1.0) ) ;
	mssvc->mainYAxis().moveTo( double(mssvc->mainYAxis().leftTravelLimit()+1.0) ) ;
      } ) ;
    extrabuttonlayout->addWidget(parkbutton,0,0) ;
    
    auto camerasafebutton = new QPushButton("Camera safe position",this) ;
    connect(camerasafebutton,&QPushButton::clicked,[=](){
	mssvc->focusAxis().moveTo( double(18.0) ) ;
      } ) ;
    extrabuttonlayout->addWidget(camerasafebutton,0,1) ;
    
    auto calibrationdialog = new MotionSystemCalibration{this} ;
    auto calibratebutton = new QPushButton("Calibrate",this) ;
    //calibratebutton->setObjectName(QStringLiteral("calibrateButton"));
    connect(calibratebutton,&QPushButton::clicked,[=](){ calibrationdialog->show() ; } ) ;
    //layout->addWidget( calibratebutton ) ;
    extrabuttonlayout->addWidget( calibratebutton,1,0) ;
			   
    auto quitbutton = new QPushButton("Quit",this) ;
    quitbutton->setObjectName(QStringLiteral("quitButton"));
    extrabuttonlayout->addWidget( quitbutton,1,1) ;

    setLayout( layout ) ;
    
    QMetaObject::connectSlotsByName(this);
  }
  
  MotionSystemWidget::~MotionSystemWidget()
  {
  }
  
  void MotionSystemWidget::on_quitButton_clicked()
  {
    QCoreApplication::quit();
  }
  
  void MotionSystemWidget::on_calibrateButton_clicked()
  {
    auto dialog = new QDialog{ this } ;
    dialog->resize(500,200) ;
    auto label = new QLabel{ dialog } ;
    label->setText("a. place the glass plate on the machine\n"
		   "b. take a number of measurements of markers\n"
		   "   - move to a marker \n"
		   "   - record the coordinates by pressing right mouse button and 'record' \n"
		   "   - manually insert the coordinates (X,y in mm) written on the glass plate \n"
		   "   - press 'accept'\n"
		   "c. press the calibrate button\n"
		   "   - this will update the constants stored in GeomSvc\n"
		   "d. test the result on a number of markers\n") ;
    label->setWordWrap(true);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
  }
}
