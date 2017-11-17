#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLCDNumber>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>

#include "MotionSystemSvc.h"
#include "MotionAxisWidget.h"
#include "MotionControllerWidget.h"
#include "MotionSystemWidget.h"

namespace PAP
{

  MotionSystemWidget::MotionSystemWidget( QWidget *parent ) :
    QWidget(parent)
  {
    //
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

    auto calibratebutton = new QPushButton("Calibrate",this) ;
    calibratebutton->setObjectName(QStringLiteral("calibrateButton"));
    layout->addWidget( calibratebutton ) ;

    
    auto quitbutton = new QPushButton("Quit",this) ;
    quitbutton->setObjectName(QStringLiteral("quitButton"));
    layout->addWidget( quitbutton ) ;
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
