#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLCDNumber>
#include <QTimer>
#include <QStyle>
#include <QIcon>

#include "MotionAxisWidget.h"
#include "MotionAxis.h"
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
    
    auto downMoveButton = new QPushButton(widget);
    downMoveButton->setObjectName(QStringLiteral("moveDownButton"));
    //downMoveButton->setText("Down") ;
    downMoveButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward)) ;
    horizontalLayout->addWidget(downMoveButton);
    
    auto downStepButton = new QPushButton(widget);
    downStepButton->setObjectName(QStringLiteral("stepDownButton"));
    downStepButton->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft)) ;
    horizontalLayout->addWidget(downStepButton);
    
    auto upStepButton = new QPushButton(widget);
    upStepButton->setObjectName(QStringLiteral("stepUpButton"));
    upStepButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay)) ;
    horizontalLayout->addWidget(upStepButton);
    
    auto moveUpButton = new QPushButton(widget);
    moveUpButton->setObjectName(QStringLiteral("moveUpButton"));
    moveUpButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward)) ;
    horizontalLayout->addWidget(moveUpButton);
    
    m_positionLabel = new QLCDNumber(widget);
    m_positionLabel->setObjectName(QStringLiteral("positionLabel"));
    horizontalLayout->addWidget(m_positionLabel);
    
    //QTimer *timer = new QTimer(this);
    //connect(timer, SIGNAL(timeout()), this, SLOT(showPosition()));
    //timer->start(1000);
    connect(&(m_axis->position()),&NamedValue::valueChanged,this,&MotionAxisWidget::showPosition) ;
    showPosition() ;
    
    auto settingsButton = new QPushButton(QIcon("images/settings.png"),"settings",widget) ;
    //settingsButton->setIcon( QIcon("images/settings.png") ) ;
    settingsButton->setObjectName(QStringLiteral("settingsButton"));
    horizontalLayout->addWidget(settingsButton);
    
    QMetaObject::connectSlotsByName(this);
  }
  
  MotionAxisWidget::~MotionAxisWidget()
  {
  }
  
  void MotionAxisWidget::on_stepDownButton_clicked()
  {
    m_axis->step(Down) ;
  }
  
  void MotionAxisWidget::on_moveDownButton_pressed()
  {
    m_axis->move(Down) ;
  }
  
  void MotionAxisWidget::on_moveDownButton_released()
  {
    m_axis->stop() ;
  }
  
  void MotionAxisWidget::on_stepUpButton_clicked()
  {
    m_axis->step(Up) ; // still need to set the stepsize!
  }
  
  void MotionAxisWidget::on_moveUpButton_pressed()
  {
    m_axis->move(Up) ;
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
    auto pos = m_axis->position() ;
    m_positionLabel->display( pos.value().toFloat() ) ;
    // change color if moving?
    QPalette pal = m_positionLabel->palette();
    if( m_axis->isMoving() ) {
      m_positionLabel->setAutoFillBackground(true); // IMPORTANT!
      pal.setColor(QPalette::Window, QColor(Qt::red));
    } else {
      m_positionLabel->setAutoFillBackground(true); // IMPORTANT!
      pal.setColor(QPalette::Window, QColor(Qt::green));
    }
    m_positionLabel->setPalette(pal);   
  }
}
