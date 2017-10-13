#ifndef NamedValueInputWidget_H
#define NamedValueInputWidget_H

#include <QPushButton>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QWidget>
#include <QLabel>
#include <QDebug>

#include "NamedValue.h"
#include "MotionSystemCommandLibrary.h"

namespace PAP
{
  
  class NamedValueInputWidget : public QWidget
  {
    Q_OBJECT  
  public:
    NamedValueInputWidget(NamedValue& v, QWidget *parent = 0)
      : QWidget(parent), m_v(&v)
      {
	auto layout = new QHBoxLayout{} ;
	// a a button that will trigger the input dialog
	auto button = new QPushButton{m_v->shortname().toString(),this} ;
	//button->setObjectName(QStringLiteral("setButton"));
	layout->addWidget( button ) ;
	connect(button, &QAbstractButton::clicked, this, &NamedValueInputWidget::buttonpressed);
	m_label = new QLabel(this) ;
	updateLabel() ;
	connect(m_v,&NamedValue::valueChanged,this,&NamedValueInputWidget::updateLabel);
	layout->addWidget(m_label) ;
	this->setLayout( layout ) ;
      } ;
    virtual ~NamedValueInputWidget() {}
    public slots:
      void updateLabel()
      {
	m_label->setText( m_v->value().toString() ) ;
      }
      
      void buttonpressed()
      {
	bool ok;
	// get the current velocity
	if(       m_v->type() == QVariant::Double ) {
	  // let's see if we can find it:
	  double xmin(-2147483647),xmax(2147483647);
	  int decimals = 3 ;
	  auto par = MSCommandLibrary::findParDef( m_v->shortname().toString() ) ;
	  if(par) {
	    xmin = par->minvalue ;
	    xmax = par->maxvalue ;
	    decimals = par->decimals ;
	  }
	  double d = QInputDialog::getDouble(this,
					     m_v->name(),
					     m_v->name(),
					     m_v->value().toDouble(),
					     xmin,xmax,decimals,
					     &ok) ;
	  if(ok) m_v->setValue( d ) ;
	} else if( m_v->type() == QVariant::Int ) {
	  int d = QInputDialog::getInt(this,
				       m_v->name(),
				       m_v->name(),
				       -64000,
				       64000,
				       1,
				       m_v->value().toInt(), &ok) ;
	  if(ok) m_v->setValue( d ) ;
	} else if( m_v->type() == QVariant::String ) {
	  QString d = QInputDialog::getText(this,
					    m_v->name(),
					    m_v->name(),
					    QLineEdit::Normal,
					    m_v->value().toString(), &ok) ;
	  if(ok) m_v->setValue( d ) ;
	} else {
	  qDebug() << "NamedValueInputQWidget: do not know this type: "
		   << m_v->type() ; 
	}
      }
      
  private:
      NamedValue* m_v ;
      QLabel* m_label ;
  } ;
}


#endif
  
