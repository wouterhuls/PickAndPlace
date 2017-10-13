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
    for( auto& c : MotionSystemSvc::instance()->controllers() ) {
      auto cw = new MotionControllerWidget( *c.second, this ) ;
      layout->addWidget( cw ) ;
    }
    
    // now add the widgets for all the axis
    for( auto& axis : MotionSystemSvc::instance()->axes() ) {
      auto mcw = new MotionAxisWidget( *axis.second, this ) ;
      qDebug() << "Adding widget for axis: "
	       << axis.second->id().controller << " "
	       << axis.second->id().axis ;
      layout->addWidget( mcw ) ;
    }
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
