#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

#include "MotionSystemSvc.h"
#include "MotionAxisWidget.h"
#include "MotionControllerWidget.h"

namespace PAP
{

  MotionControllerWidget::MotionControllerWidget(const MotionController& c, QWidget *parent ) :
    QWidget(parent), m_controller(&c)
  {
    //
    this->setObjectName(c.name().c_str()) ;
    this->setWindowTitle(c.name().c_str()) ;
    //resize(500,500) ;
    
    // add a widget for motors on/off (this should actually be done by controller)
    auto layout = new QHBoxLayout{} ;
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout) ;
    
    auto nameLabel = new QLabel(this);
    nameLabel->setObjectName(QStringLiteral("nameLabel"));
    nameLabel->setText( m_controller->name().c_str() ) ;
    layout->addWidget(nameLabel);
    
    m_motorsonbutton = new QPushButton("Motors On",this) ;
    m_motorsonbutton->setObjectName(QStringLiteral("motorsOnButton"));
    m_motorsonbutton->setCheckable(true) ;
    m_motorsonbutton->setChecked( m_controller->hasMotorsOn() ) ;
    layout->addWidget( m_motorsonbutton ) ;
    
    m_statuslabel = new QLabel(this) ;
    m_statuslabel->setText("status unknown") ;
    layout->addWidget( m_statuslabel) ;
    
    m_errorlabel = new QLabel(this) ;
    m_errorlabel->setText("error unknown") ;
    layout->addWidget( m_errorlabel) ;

    // FIXME: this should just work with signals!
    update() ;
    connect( m_controller, SIGNAL(statusChanged()), this, SLOT(update()));
    //QTimer *timer = new QTimer(this);
    //connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    //timer->start(500);
    
    QMetaObject::connectSlotsByName(this);
  }
  
  MotionControllerWidget::~MotionControllerWidget()
  {
  }
  
  void MotionControllerWidget::on_motorsOnButton_clicked()
  {
    qInfo() << "Switching motors: "
	    << m_motorsonbutton->isChecked() ;
    m_controller->switchMotorsOn( m_motorsonbutton->isChecked() ) ;
  } 
  
  void MotionControllerWidget::update()
  {
    qInfo() << "MotionControllerWidget::update "
	     << m_controller->id() << " "
	     << m_controller->status() << " "
	     << m_controller->hasMotorsOn() ;
    
    QString s ;
    s.sprintf("status = %02X",m_controller->status()) ;
    m_statuslabel->setText( s ) ;
    m_statuslabel->setAutoFillBackground(true); // IMPORTANT!
    QPalette pal = m_statuslabel->palette();
    pal.setColor(QPalette::Window, QColor(m_controller->hasMotorsOn() ? Qt::green : Qt::red));
    m_statuslabel->setPalette(pal);  
    m_errorlabel->setText( QString("error=") + m_controller->error() ) ;
    
    m_motorsonbutton->setText( m_controller->hasMotorsOn() ? "ON" : "OFF" ) ;
  }
}
