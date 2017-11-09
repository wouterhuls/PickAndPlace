#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QInputDialog>

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
    
    layout->addWidget( new NamedValueInputWidget(m_axis->stepsize(),this) ) ;
    layout->addWidget( new NamedValueInputWidget(m_axis->position(),this) ) ;
    
    // search home
    auto searchHomeButton = new QPushButton(this);
    searchHomeButton->setObjectName(QStringLiteral("searchHomeButton"));
    searchHomeButton->setText("Search Home") ;
    layout->addWidget(searchHomeButton);
    
    // set the current position as 'zero'
    auto setZeroButton = new QPushButton(this);
    setZeroButton->setObjectName(QStringLiteral("setZeroButton"));
    setZeroButton->setText("Set Zero") ;
    layout->addWidget(setZeroButton);
    
    QMetaObject::connectSlotsByName(this);
  }
  
  void MotionAxisSettingsDialog::on_searchHomeButton_clicked()
  {
    m_axis->searchHome() ;
  }
}
