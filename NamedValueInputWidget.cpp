#include <QPushButton>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDebug>
#include "NamedValue.h"
#include "MotionSystemCommandLibrary.h"
#include "NamedValueInputWidget.h"

namespace PAP
{

  NamedValueInputWidget::NamedValueInputWidget(NamedValueBase& v, QWidget *parent)
    : QWidget(parent), m_v(&v), m_label(this)
  {
    auto layout = new QHBoxLayout{} ;
    // a a button that will trigger the input dialog
    auto button = new QPushButton{m_v->shortname().toString(),this} ;
    //button->setObjectName(QStringLiteral("setButton"));
    layout->addWidget( button ) ;
    connect(button, &QAbstractButton::clicked, this, &NamedValueInputWidget::buttonpressed);
    updateLabel() ;
    connect(m_v,&NamedValue::valueChanged,this,&NamedValueInputWidget::updateLabel);
    layout->addWidget(&m_label) ;
    this->setLayout( layout ) ;
  }
    
  void NamedValueInputWidget::updateLabel()
  {
    m_label.setText( m_v->toString() ) ;
  }
  
  void NamedValueInputWidget::buttonpressed()
  {
    bool ok;
    NamedValue* v ;
    NamedDouble* vd ;
    // FIXME: still need to find a better solution for this!
    int decimals=3 ;
    auto par = MSCommandLibrary::findParDef( m_v->shortname().toString() ) ;
    if(par) decimals = par->decimals ;

    if( (v = dynamic_cast<NamedValue*>(m_v) ) ) {
      if(       v->type() == QVariant::Double ) {
	double d = QInputDialog::getDouble(this,v->name(),v->name(),
					   v->value().toDouble(),
					   v->min().toDouble(),
					   v->max().toDouble(),
					   decimals,
					   &ok) ;
	if(ok) v->setValue( d ) ;
      } else if( v->type() == QVariant::Int ) {
	int d = QInputDialog::getInt(this,v->name(),v->name(),
				     -64000,
				     64000,
				     1,
				     v->value().toInt(), &ok) ;
	if(ok) v->setValue( d ) ;
      } else if( v->type() == QVariant::String ) {
	QString d = QInputDialog::getText(this,v->name(),v->name(),
					  QLineEdit::Normal,
					  v->value().toString(), &ok) ;
	if(ok) v->setValue( d ) ;
      } else {
	qDebug() << "NamedValueInputQWidget: do not know this type: "
		 << v->type() ; 
      }
    } else if( (vd = dynamic_cast<NamedDouble*>(m_v) ) ) {
      double d = QInputDialog::getDouble(this,vd->name(),vd->name(),
					 vd->value(),
					 vd->min(),
					 vd->max(),
					 decimals,
					 &ok) ;
      if(ok) vd->setValue( d ) ;
    } else {
      qWarning() << "NamedValueInputQWidget: cannot deduce type of NamedValue." ;
    }
  }
}
