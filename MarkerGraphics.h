#pragma once
#include <QGraphicsItem>
#include <cmath>

namespace PAP
{
  // Interface class to markers. Markers have a name and position
  class Marker : public QGraphicsItem
  {
  public:
    // constructor
    Marker( const QString& name, double x, double y, QGraphicsItem *parent )
      : QGraphicsItem(parent)
    {
      setToolTip( name ) ;
      setPos( x, y ) ;
    } ;
    Marker( const FiducialDefinition & def,QGraphicsItem *parent)
      : QGraphicsItem(parent)
    {
      setToolTip( def.name ) ;
      setPos( def.x, def.y ) ;
    } ;
    QString name() const { return toolTip() ; }
  } ;

  
  class ReferenceMarker : public Marker
  {
    
  private:
    const float m_size = 0.05 ; // size in micron
  public:
    ReferenceMarker(const FiducialDefinition& def, QGraphicsItem *parent = Q_NULLPTR)
      : Marker(def,parent){}
    virtual QRectF boundingRect() const
    {
      //qreal penWidth = 1 ;
      qreal bound = 1.02 * m_size ;
      return QRectF{-0.5*bound,-0.5*bound,bound,bound} ;
    }
    virtual void paint(QPainter *painter,
		       const QStyleOptionGraphicsItem* /*option*/,
		       QWidget* /*widget*/)
    {
      // semitransparent yellow??
      QPen pen ;
      pen.setColor( Qt::yellow ) ;
      // let's draw as a circle with some lines inside
      pen.setWidthF(0.01*m_size); // in which units? this is shit ... rescaling does not work!      
      painter->setPen(pen) ;
      QRectF rectangle{-0.5*m_size,-0.5*m_size,m_size,m_size};
      painter->drawRect( rectangle ) ;
      painter->drawEllipse( rectangle ) ;
    }
  } ;

  class SubstrateMarker : public Marker
  {
  private:
    const float m_size = 0.3 ; // size in micron
  public:
    SubstrateMarker(const FiducialDefinition& def, QGraphicsItem *parent = Q_NULLPTR)
      : Marker(def,parent){}
    virtual QRectF boundingRect() const
    {
      //qreal penWidth = 1 ;
      qreal bound = 1.02 * m_size ;
      return QRectF{-0.5*bound,-0.5*bound,bound,bound} ;
    }
    virtual void paint(QPainter *painter,
		       const QStyleOptionGraphicsItem* /*option*/,
		       QWidget* /*widget*/)
    {
      // let's draw a double circle. outer diameter is 300 micron. inner is 250 micron.
      // semitransparent yellow??
      QPen pen ;
      pen.setColor( Qt::yellow ) ;
      pen.setWidthF(0.02*m_size); // in which units?
      painter->setPen(pen) ;
      for( auto d : {0.3,0.25} )
	painter->drawEllipse( QRectF{-d/2,-d/2,d,d}) ;
    }
  } ;
  
  
  // This is the actual jig marker to scale.
  class MeasuredJigMarker : public Marker
  {
  private:
    const float m_circleradius = 0.02 ;
    const float m_circledist   = 0.025*std::sqrt(2.) ;
  public:
  MeasuredJigMarker(const QString& name, QGraphicsItem *parent = Q_NULLPTR)
    : Marker{name,0,0, parent} {}
    
    virtual QRectF boundingRect() const
    {
      //qreal penWidth = 1 ;
      qreal bound = 2*(m_circledist + m_circleradius) ;
      return QRectF{-0.5*bound,-0.5*bound,bound,bound} ;
    }

    virtual void paint(QPainter *painter,
		       const QStyleOptionGraphicsItem* /*option*/,
		       QWidget* /*widget*/)
    {
      // semitransparent yellow??
      QPen pen ;
      pen.setColor( QColor{65,80,244} ) ;
      pen.setWidthF(0.1);
      painter->setPen(pen) ;
      QRectF rectangle{-0.5*m_circleradius,-0.5*m_circleradius,m_circleradius,m_circleradius} ;
      painter->drawEllipse(QPointF(+m_circledist,0), m_circleradius,m_circleradius) ;
      painter->drawEllipse(QPointF(0,+m_circledist), m_circleradius,m_circleradius) ;
      painter->drawEllipse(QPointF(-m_circledist,0), m_circleradius,m_circleradius) ;
      painter->drawEllipse(QPointF(0,-m_circledist), m_circleradius,m_circleradius) ;
    }
  } ;
  

}
