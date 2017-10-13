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
    this->setObjectName( axis.name().c_str() ) ;
    this->setWindowTitle( axis.name().c_str()) ;
    
    //auto layout = new QGridLayout{} ;
    auto layout = new QVBoxLayout{} ;
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout) ;
    
    // add some buttons
    for( auto& par : axis.parameters() ) {
      auto w = new NamedValueInputWidget(par,this) ;
      layout->addWidget( w ) ;
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
