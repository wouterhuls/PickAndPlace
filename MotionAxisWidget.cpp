#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLCDNumber>
#include <QTimer>
#include <QStyle>
#include <QIcon>

#include <cmath>

#include "MotionAxisWidget.h"
#include "MotionAxis.h"
#include "MotionController.h"
#include "MotionAxisSettingsDialog.h"

namespace PAP
{

  MotionAxisWidget::MotionAxisWidget(MotionAxis& axis, QWidget *parent) :
    QWidget(parent), m_axis(&axis)
  {
    //
    this->setObjectName( m_axis->name() ) ;
    
    //resize(400, 50);
    setGeometry(QRect(10, 10, 400, 50));
    auto widget = this ;
    auto horizontalLayout = new QHBoxLayout(widget);
    horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    auto nameLabel = new QLabel(widget);
    nameLabel->setObjectName(QStringLiteral("nameLabel"));
    nameLabel->setText( m_axis->name() ) ;
    horizontalLayout->addWidget(nameLabel);
    
    auto moveDownButton = new QPushButton(widget);
    moveDownButton->setObjectName(QStringLiteral("moveDownButton"));
    //moveDownButton->setText("Down") ;
    moveDownButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward)) ;
    horizontalLayout->addWidget(moveDownButton);
    
    auto stepDownButton = new QPushButton(widget);
    stepDownButton->setObjectName(QStringLiteral("stepDownButton"));
    stepDownButton->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft)) ;
    horizontalLayout->addWidget(stepDownButton);

    auto stopButton = new QPushButton(widget);
    stopButton->setObjectName(QStringLiteral("stopButton"));
    stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop)) ;
    connect(stopButton,&QPushButton::clicked,[=](){ m_axis->stop(); }) ;
    horizontalLayout->addWidget(stopButton);
    
    auto stepUpButton = new QPushButton(widget);
    stepUpButton->setObjectName(QStringLiteral("stepUpButton"));
    stepUpButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay)) ;
    horizontalLayout->addWidget(stepUpButton);
    
    auto moveUpButton = new QPushButton(widget);
    moveUpButton->setObjectName(QStringLiteral("moveUpButton"));
    moveUpButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward)) ;
    horizontalLayout->addWidget(moveUpButton);

    // auto ahButton = new QPushButton("AH",widget);
    // connect(ahButton,&QPushButton::clicked,[=](){ m_axis->applyAntiHysteresisStep(); }) ;
    // horizontalLayout->addWidget(ahButton);
    
    m_positionLabel = new QLCDNumber(widget);
    m_positionLabel->setObjectName(QStringLiteral("positionLabel"));
    m_positionLabel->setDigitCount(7) ;
    horizontalLayout->addWidget(m_positionLabel);
    
    //QTimer *timer = new QTimer(this);
    //connect(timer, SIGNAL(timeout()), this, SLOT(showPosition()));
    //timer->start(1000);
    connect(&(m_axis->position()),&NamedValueBase::valueChanged,this,&MotionAxisWidget::showPosition) ;
    connect(m_axis,&MotionAxis::movementStarted,this,&MotionAxisWidget::showPosition) ;
    connect(m_axis,&MotionAxis::movementStopped,this,&MotionAxisWidget::showPosition) ;
    // this one we should not need, but for some reason the started and stopped signals are not enough:-(
    //connect(&(m_axis->controller()),&MotionController::statusChanged,this,&MotionAxisWidget::showPosition) ;
    showPosition() ;
    
    auto settingsButton = new QPushButton(QIcon(":/images/settings.png"),"",widget) ;
    settingsButton->setObjectName(QStringLiteral("settingsButton"));
    horizontalLayout->addWidget(settingsButton);
    
    QMetaObject::connectSlotsByName(this);
  }
  
  MotionAxisWidget::~MotionAxisWidget()
  {
  }
  
  void MotionAxisWidget::on_stepDownButton_clicked()
  {
    m_axis->step(MotionAxis::Down) ;
  }
  
  void MotionAxisWidget::on_moveDownButton_pressed()
  {
    m_axis->move(MotionAxis::Down) ;
  }
  
  void MotionAxisWidget::on_moveDownButton_released()
  {
    m_axis->stop() ;
  }
  
  void MotionAxisWidget::on_stepUpButton_clicked()
  {
    m_axis->step(MotionAxis::Up) ; // still need to set the stepsize!
  }
  
  void MotionAxisWidget::on_moveUpButton_pressed()
  {
    m_axis->move(MotionAxis::Up) ;
  }
  
  void MotionAxisWidget::on_moveUpButton_released()
  {
    m_axis->stop() ;
  }

  void MotionAxisWidget::on_settingsButton_clicked()
  {
    auto window = new MotionAxisSettingsDialog( *m_axis, this ) ;
    window->show() ;
  }
  
  void MotionAxisWidget::showPosition()
  {
    //auto pos = dynamic_cast<const NamedValue*>(sender()) ;
    double pos = m_axis->position() ;
    double setpos = m_axis->setPosition() ;
    m_positionLabel->display( pos ) ;
    // change color if moving?
    QPalette pal = m_positionLabel->palette();
    // qDebug() << "MotionAxisWidget: "
    // 	     << m_axis->name() << " ismoving: " << m_axis->isMoving() ;
    if( pos == 0 ) {
      pal.setColor(QPalette::Window, QColor(Qt::gray));
    } else if( m_axis->isMoving() ) {
      m_positionLabel->setAutoFillBackground(true); // IMPORTANT!
      pal.setColor(QPalette::Window, QColor(Qt::yellow));
    } else {
      m_positionLabel->setAutoFillBackground(true); // IMPORTANT!
      if( std::abs( pos-setpos ) <= 0.0015 )
	pal.setColor(QPalette::Window, QColor(Qt::green));
      else
	pal.setColor(QPalette::Window, QColor(Qt::red));
    }
    m_positionLabel->setPalette(pal);   
  }
}
