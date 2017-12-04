#include <QPushButton>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDebug>
#include "NamedValue.h"
#include "MotionSystemCommandLibrary.h"
#include "NamedValueInputWidget.h"

namespace PAP
{
  template<typename T>
  NamedValueInputWidget<T>::NamedValueInputWidget(NamedValue<T>& v,
						  const T& min,
						  const T& max,
						  int decimals,
						  QWidget *parent)
    : QWidget(parent), m_v(&v), m_min(min), m_max(max), m_decimals(decimals),m_label(this)
  {
    auto layout = new QHBoxLayout{} ;
    // a a button that will trigger the input dialog
    auto button = new QPushButton{m_v->shortname().toString(),this} ;
    //button->setObjectName(QStringLiteral("setButton"));
    layout->addWidget( button ) ;
    connect(button, &QAbstractButton::clicked, this, &NamedValueInputWidget::buttonpressed);
    updateLabel() ;
    connect(m_v,&NamedValueBase::valueChanged,this,&NamedValueInputWidget::updateLabel);
    layout->addWidget(&m_label) ;
    this->setLayout( layout ) ;
  }

  template<typename T>
  void NamedValueInputWidget<T>::updateLabel()
  {
    m_label.setText( m_v->toString() ) ;
  }

  template<>
  void NamedValueInputWidget<QVariant>::buttonpressed()
  {
    bool ok;
    if(       m_v->type() == QVariant::Double ) {
      double d = QInputDialog::getDouble(this,m_v->name(),m_v->name(),
					 m_v->value().toDouble(),
					 m_min.toDouble(),
					 m_max.toDouble(),
					 m_decimals,&ok) ;
      if(ok) m_v->setValue( d ) ;
    } else if( m_v->type() == QVariant::Int ) {
      int d = QInputDialog::getInt(this,m_v->name(),m_v->name(),
				   m_min.toInt(),
				   m_max.toInt(),
				   1,
				   m_v->value().toInt(), &ok) ;
      if(ok) m_v->setValue( d ) ;
    } else if( m_v->type() == QVariant::String ) {
      QString d = QInputDialog::getText(this,m_v->name(),m_v->name(),
					QLineEdit::Normal,
					m_v->value().toString(), &ok) ;
      if(ok) m_v->setValue( d ) ;
    } else {
      qDebug() << "NamedValueInputWidget: do not know this type: "
	       << m_v->type() ; 
    }
  }

  template<>
  void NamedValueInputWidget<double>::buttonpressed()
  {
    bool ok;
    double d = QInputDialog::getDouble(this,m_v->name(),m_v->name(),
				       m_v->value(),
				       m_min,
				       m_max,
				       m_decimals,
				       &ok) ;
    if(ok) m_v->setValue( d ) ;
  }

  // explicit instantiation
  template class NamedValueInputWidget<QVariant>;
  template class NamedValueInputWidget<double>;
}
