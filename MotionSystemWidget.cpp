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
	mssvc->focusAxis().moveTo( double(20.0) ) ;
      } ) ;
    extrabuttonlayout->addWidget(camerasafebutton,0,1) ;
    
    auto calibrationdialog = new MotionSystemCalibration{this} ;
    auto calibratebutton = new QPushButton("Calibrate",this) ;
    connect(calibratebutton,&QPushButton::clicked,[=](){ calibrationdialog->show() ; } ) ;
    extrabuttonlayout->addWidget( calibratebutton,1,0) ;
			   
    setLayout( layout ) ;
    
  }
  
  MotionSystemWidget::~MotionSystemWidget()
  {
  }
}
