#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QCheckBox>

#include "MotionAxis.h"
#include "MotionAxisSettingsDialog.h"
#include "NamedValueInputWidget.h"


namespace PAP
{
  
  MotionAxisSettingsDialog::MotionAxisSettingsDialog( MotionAxis& axis, QWidget *parent )
    : QDialog(parent),m_axis(&axis)
  {
    this->setObjectName( axis.name() ) ;
    this->setWindowTitle( axis.name() ) ;
    
    //auto layout = new QGridLayout{} ;
    auto layout = new QVBoxLayout{} ;
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout) ;
    
    // add some buttons
    for( auto& par : axis.parameters() ) {
      auto w = new NamedValueInputWidget(par,this) ;
      layout->addWidget( w ) ;
      // enable the call back only now
      QObject::connect( &par, &NamedValueBase::valueChanged,
			&axis, &MotionAxis::handleParameterUpdate ) ;
    }
    
    layout->addWidget( new NamedValueInputWidget{m_axis->stepsize(),this} ) ;
    layout->addWidget( new NamedValueInputWidget{m_axis->position(),this} ) ;

    m_allowpasstravellimitbox = new QCheckBox{"Allow pass travel limit", this} ;
    m_allowpasstravellimitbox->setCheckState( m_axis->allowPassTravelLimit() ? Qt::Checked : Qt::Unchecked) ;
    connect( m_allowpasstravellimitbox, &QCheckBox::stateChanged,
	     this, &MotionAxisSettingsDialog::allowPassTravelLimitChanged) ;
    layout->addWidget( m_allowpasstravellimitbox ) ;
    
    // search home
    auto searchHomeButton = new QPushButton{this};
    searchHomeButton->setObjectName(QStringLiteral("searchHomeButton"));
    searchHomeButton->setText("Search Home") ;
    layout->addWidget(searchHomeButton);
    
    // set the current position as 'zero'
    auto setZeroButton = new QPushButton{this};
    setZeroButton->setObjectName(QStringLiteral("zeroButton"));
    setZeroButton->setText("Set Zero") ;
    layout->addWidget(setZeroButton);
    
    QMetaObject::connectSlotsByName(this);
  }
  
  void MotionAxisSettingsDialog::on_searchHomeButton_clicked()
  {
    m_axis->searchHome() ;
  }

  void MotionAxisSettingsDialog::on_zeroButton_clicked()
  {
    if( !m_axis->isMoving() )
      m_axis->setZero() ;
    else {
      QDialog dialog(this) ;
      QLabel label("Cannot zet zero: axis still moving",&dialog) ;
      dialog.exec() ;
    }
  }


  
  void MotionAxisSettingsDialog::allowPassTravelLimitChanged()
  {
    m_axis->setAllowPassTravelLimit( m_allowpasstravellimitbox->isChecked() ) ;
  }
}
