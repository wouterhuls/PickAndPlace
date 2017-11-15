#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLCDNumber>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCoreApplication>
#include <QDebug>

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

}
