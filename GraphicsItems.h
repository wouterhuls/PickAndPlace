#ifndef GRAPHICSITEMS_H
#define GRAPHICSITEMS_H

#include "MarkerGraphics.h"
#include <QStaticText>
#include <cmath>
#include <iostream>

namespace PAP
{
  
  // Implements graphics item for Velopix marker. Center of marker is
  // center of system. Units are now in mm
  class VelopixMarker : public Marker
  {
  private:
    const float m_size = 0.1 ; // size in micron
  public:
    VelopixMarker(const FiducialDefinition& def, QGraphicsItem *parent = Q_NULLPTR)
      : Marker(def,parent)
    {
      setRotation(45) ;
    }
    
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
      //pen.setColor( QColor{230,100,25} ) ;
      pen.setColor( QColor{230,100,25} ) ; //Qt::blue ) ;
      pen.setWidthF(0.005); // in which units? this is shit ... rescaling does not work!
      painter->setPen(pen) ;
      //painter->setBrush(QColor(0, 255, 255, 127));
      // fraw here all the lines for a marker. perhaps 'polygon'?
      //painter->setBrush( QColor{230,100,25} ) ; //Qt::cyan );
      drawPoly(*painter, m_size ) ;
      // draw the cross
      pen.setStyle(Qt::DashLine);
      pen.setWidthF(0.002); // in which units? this is shit ... rescaling does not work!
      painter->setPen(pen) ;
      painter->drawLine(QPointF(-m_size/2,0),QPointF(+m_size/2,0)) ;
      painter->drawLine(QPointF(0,-m_size/2),QPointF(0,+m_size/2)) ;
    }

    inline void drawPoly(QPainter& painter, float scale, float t = 0.2) {
      const int N=8 ;
      // beware: points not centered around 0,0
      float l1x[] = {t,1-t,1,1,1-t,t,0,0,t} ;
      float l1y[] = {1,1,1-t,t,0,0,t,1-t,1} ;
      QPointF points[8] ;
      for(int i=0; i<N; ++i)
	points[i] = QPointF{ scale * ( l1x[i]-0.5 ), scale * ( l1y[i]-0.5 ) } ;
      painter.drawPolygon(points,8) ;
    }
    
  } ;

  class SensorPixelMarker : public Marker
  {
  private:
    const float m_size = 0.02 ; // size in micron
  public:
    SensorPixelMarker(const FiducialDefinition& def, QGraphicsItem *parent = Q_NULLPTR)
      : Marker(def,parent)
    {
      std::cout << "Sensor pixel marker: "
		<< def.name.toStdString() << " " << def.x << " " << def.x << std::endl ;
    }
    
    virtual QRectF boundingRect() const
    {
      //qreal penWidth = 1 ;
      qreal bound = 1.1 * m_size ;
      return QRectF{-0.5*bound,-0.5*bound,bound,bound} ;
    }

    virtual void paint(QPainter *painter,
		       const QStyleOptionGraphicsItem* /*option*/,
		       QWidget* /*widget*/)
    {
      QPen pen ;
      pen.setColor( QColor{230,100,25} ) ;
      pen.setWidthF(0.1*m_size); // in which units? this is shit ... rescaling does not work!
      painter->setPen(pen) ;
      QRectF rectangle{-0.5*m_size,-0.5*m_size,m_size,m_size};
      painter->drawEllipse( rectangle ) ;
      painter->drawLine(QPointF{-0.5*m_size,0},QPointF{0.5*m_size,0}) ;
      painter->drawLine(QPointF{0,-0.5*m_size},QPointF{0,+0.5*m_size}) ;
      //painter->drawRect( rectangle ) ;
    }
  } ;
  
  class Tile : public QGraphicsItemGroup
  {
  private:
    // for all dimensions, see https://edms.cern.ch/document/2086903
    const double markerdist = 42.514 ;
    const double width  = 43.470 ;
    const double height = 16.950 ;
    const double sensorwidth  = 43.470 ;
    const double sensorheight = 14.980 ;
    const double sensoredgetoasicmarker = 1.853 ;
    const double asicdist  = 0.105 ;
    const double asicsidetomarker = 0.058 ;
    const double asicbottomtomarker = 0.117 ;
    const double asicwidth  = (42.514 +2*asicsidetomarker- 2*asicdist)/3. ;
    const double asicheight = 16.600 ;
    const double pixelsize = 0.055 ;
    const double extrainterasicdist = 0.33 - 3*pixelsize ;
    const double distfirstlastpixelx = (3*256-1)*pixelsize + 2*extrainterasicdist ;
    const double distfirstlastpixely = 255*pixelsize ;
    const double sensormarkeredgedist = 0.090 ;
    
  public:
    Tile( const std::vector<FiducialDefinition>& markerdefs,
	  QGraphicsItem *parent = Q_NULLPTR) : QGraphicsItemGroup(parent)
    {
      // the position is the middle between the first and las velopix marker
      const auto& m1 = markerdefs.front() ;
      const auto& m2 = markerdefs.back() ;
      const auto dx = m2.x - m1.x ;
      const auto dy = m2.y - m1.y ;
      const auto angle = std::atan2( dy, dx ) ;
      const auto x0 = 0.5*(m2.x+m1.x) ;
      const auto y0 = 0.5*(m2.y+m1.y) ;
      setPos(x0,y0) ;
      setRotation( angle * 180.0 / M_PI ) ;
      setToolTip(QString("Tile ") + markerdefs.front().name.leftRef(3) ) ;
      qDebug() << "Tile average position: " << toolTip() << x0 << y0 ;
      
      // add a few markers for the sensor flatness measurements
      const double X0 = -(width-markerdist)/2 + 0.6 - x0 ;
      const double Y0 = +2.5 - y0 ;
      bool nside = markerdefs.front().name.contains("N") ;
      QPointF orderedcorners[4] ;
      for(int j=0; j<4; ++j) {
	int i = nside ? j : ( 2*(j/2) + (1 - j%2 ) ) ;
	const double x = X0 + ((i+i/2)%2) * (width-1.2) ;
	const double y = Y0 + (i/2) * (height-3.2) ;
	orderedcorners[j] = QPointF(x,y) ;
      }
            
      // I want more: let's go for 12, 4 lines of 4 in a snake pattern
      //std::vector< QPointF > points ;
      QString sensorname = QString{"Sensor"}+markerdefs.front().name.leftRef(3); 
      const int nX = 7 ;
      const int nY = 4 ;
      const float dY = orderedcorners[2].y() - orderedcorners[0].y() ;
      const float dX = orderedcorners[2].x() - orderedcorners[0].x() ;
      for(int iy=0; iy<nY; ++iy) {
	float y = orderedcorners[0].y() + (dY*iy)/(nY-1) ;
	for(int ix=0;ix<nX; ++ix) {
	  int jx = (iy%2) ? nX-1-ix : ix ;
	  float x = orderedcorners[0].x() + (dX*jx)/(nX-1) ;
	  QString refname = sensorname ;
	  refname += QString::number( jx+1 ) ;
	  refname += "_" ;
	  refname += QString::number( iy+1 ) ;
	  FiducialDefinition def{refname,x,y} ;
	  (static_cast<QGraphicsItemGroup*>(parent))->addToGroup( new ReferenceMarker{ def, this } ) ;
	}
      }
      
      // add the sensor circular markers

      // left edge
      for(int ipixel=10; ipixel<=256; ipixel+=10) {
	// I don't know the offset yet
	const double x = -0.5*sensorwidth + sensormarkeredgedist ;
	const double y = sensoredgetoasicmarker + 0.5*(sensorheight-distfirstlastpixely) + (ipixel-1)*pixelsize ;
	FiducialDefinition def{sensorname + QString{"_R"} + QString::number(ipixel),x,y} ;
	(static_cast<QGraphicsItemGroup*>(parent))->addToGroup( new SensorPixelMarker{def,this} ) ;
      }
      // right edge
      for(int ipixel=10; ipixel<=256; ipixel+=10) {
	// I don't know the offset yet
	const double x = 0.5*sensorwidth - sensormarkeredgedist ;
	const double y = sensoredgetoasicmarker + 0.5*(sensorheight-distfirstlastpixely) + (ipixel-1)*pixelsize ;
	FiducialDefinition def{sensorname + QString{"_L"} + QString::number(ipixel),x,y} ;
	(static_cast<QGraphicsItemGroup*>(parent))->addToGroup( new SensorPixelMarker{def,this} ) ;
      }
      // top edge
      for(int ipixel=10; ipixel<=3*256; ipixel+=10) {
	//
	const auto iasic = (ipixel-1)/256 ;
	// I don't know the offset yet
	const double y = sensoredgetoasicmarker + sensorheight - sensormarkeredgedist ;
	//const double x = -0.5*distfirstlastpixelx + (ipixel-1)*pixelsize + iasic*extrainterasicdist ;
	const double x = 0.5*distfirstlastpixelx - (ipixel-1)*pixelsize - iasic*extrainterasicdist ;
	FiducialDefinition def{sensorname + QString{"_T"} + QString::number(ipixel),x,y} ;
	(static_cast<QGraphicsItemGroup*>(parent))->addToGroup( new SensorPixelMarker{def,this} ) ;
      }
    }
  
    // we'll draw just a 
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem* /*option*/,
		       QWidget* /*widget*/)
    {
      // It will be just a rectangle. The origin is at the position of the first marker.
      // Let's first just draw a thin line that connects the first and last marker
      QPen pen ;
      pen.setColor( QColor{230,100,25} ) ;//Qt::yellow ) ;
      pen.setWidthF(1.0);
      pen.setCosmetic(true) ;
      painter->setPen(pen) ;
      painter->drawLine( QPointF{-0.5*markerdist,0},QPointF{+0.5*markerdist,0}) ;
            
      // draw the sensor
      {
	pen.setColor( Qt::blue ) ;
	painter->setPen(pen) ;
	painter->drawRect( QRectF{ -sensorwidth/2, sensoredgetoasicmarker, sensorwidth, sensorheight} ) ;
	// draw a line through the outer pixels
	pen.setColor( Qt::red ) ;
	painter->setPen(pen) ;
	painter->drawRect(
			  QRectF{ -distfirstlastpixelx/2,
			      sensoredgetoasicmarker + 0.5*(sensorheight-distfirstlastpixely),
			      distfirstlastpixelx , distfirstlastpixely} ) ;
      }
	
      // draw the three ASICs
      {
	// outline
	double x0       = -0.5*markerdist-asicsidetomarker;
	const double y0 = -asicbottomtomarker ;
	pen.setColor( Qt::green ) ;
	painter->setPen(pen) ;
	for(int i=0; i<3; ++i) {
	  // outline
	  painter->drawRect(QRectF{x0,y0,asicwidth,asicheight}) ;
	  x0 += asicwidth + asicdist ;
	}
      }
    }
    
    virtual QRectF boundingRect() const
    { 
      QRectF rectangle{-(width+1),-0.9,width+1,height+1.5} ;
      return rectangle ;
    }
  } ;
  
 class Substrate : public QGraphicsItem
  {
  private:
    const double Lx = 111.37 ;
    const double Ly = 116.5 ;
    const double Px = -Lx + 89.82  ;
    const double Py = -46.025 ;
    
  public:
    Substrate(QGraphicsItem *parent = Q_NULLPTR) : QGraphicsItem(parent) {
      setPos(0,0) ;
      setToolTip("Substrate") ;
    } ;
      
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem* /*option*/,
		       QWidget* /*widget*/)
    {
      const QPointF points[] = {
        { Px + Lx - 51.42, Py },
	{ Px + Lx - 10.00, Py },
	{ Px + Lx        , Py + 10 },
	{ Px + Lx        , Py + 83 },
	{ Px + Lx - 33.50, Py + Ly },
	{ Px + Lx - 77.01, Py + Ly },
	{ Px             , Py + 83 },     // top point
	{ Px + 36.575    , Py + 46.425 }, // closest to IP
	//{0,0},
	//{ Px + 36.575    , Py + 46.425 }, // closest to IP
	{ Px + Lx - 86.82, Py + 34.40 }
      } ;
      QPen pen ;
      //      pen.setColor( QColor{65,80,244} ) ;
      pen.setCosmetic(true) ;
      pen.setWidthF(1);
      //pen.setStyle(Qt::DotLine) ;
      painter->setPen(pen) ;
      painter->drawPolygon( points, 9 ) ;
    }
     virtual QRectF boundingRect() const
     { 
       return QRectF{Px,Py,Lx,Ly} ;
     }
  } ;
  
  // Implements graphics item for a marker of the Jig.
  class JigMarker : public Marker
  {
  private:
    const float m_size = 10.0 ; // size in mm
  public:
    JigMarker(const FiducialDefinition& def, QGraphicsItem *parent = Q_NULLPTR)
      : Marker(def,parent) {}
    
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
      pen.setColor( QColor{65,80,244} ) ;
      
      // let's draw some thin lines near the center
      pen.setWidthF(0.002); // in which units? this is shit ... rescaling does not work!
      painter->setPen(pen) ;
      painter->drawLine(QPointF(0,-0.200),QPointF(0,0.200)) ;
      painter->drawLine(QPointF(-0.200,0),QPointF(0.200,0)) ;
      // let's draw as a circle with some lines inside
      pen.setWidthF(0.005*m_size); // in which units? this is shit ... rescaling does not work!      
      painter->setPen(pen) ;
      QRectF rectangle{-0.5*m_size,-0.5*m_size,m_size,m_size};
      painter->drawRect( rectangle ) ;
      painter->drawEllipse( rectangle ) ;
      painter->drawLine(QPointF(0,-0.200),QPointF(0,-0.5*m_size)) ;
      painter->drawLine(QPointF(0,+0.200),QPointF(0,+0.5*m_size)) ;
      painter->drawLine(QPointF(-0.200,0),QPointF(-0.5*m_size,0)) ;
      painter->drawLine(QPointF(+0.200,0),QPointF(+0.5*m_size,0)) ;
      
    }    
  } ;


  // Implements graphics item for Velopix marker. Center of marker is
  // center of system. Units are in microns!!!
  class SightMarker : public QGraphicsItem
  {
  private:
    float m_size = 10.0 ; // size in mm
  public:
    SightMarker(const FiducialDefinition& def,
		float thesize,
		QGraphicsItem *parent = Q_NULLPTR) : QGraphicsItem(parent), m_size(thesize)
    {
      setPos(def.x,def.y) ;  // convert from mm to micron
      setToolTip(def.name) ;
    } ;
    
    virtual QRectF boundingRect() const
    {
      //qreal penWidth = 1 ;
      qreal bound = 1.01 * m_size ;
      return QRectF{-0.5*bound,-0.5*bound,bound,bound} ;
    }

    virtual void paint(QPainter *painter,
		       const QStyleOptionGraphicsItem* /*option*/,
		       QWidget* /*widget*/)
    {
      // semitransparent yellow??
      QPen pen ;
      pen.setColor( QColor{65,80,244} ) ;
      
      // let's draw some thin lines near the center
      //pen.setWidthF(0.0002*m_size); // in which units? this is shit ... rescaling does not work!
      pen.setWidthF(1.0);
      pen.setCosmetic(true) ;
      painter->setPen(pen) ;
      const double L = 0.02*m_size ;
      painter->drawLine(QPointF(0,-L),QPointF(0,L)) ;
      painter->drawLine(QPointF(-L,0),QPointF(L,0)) ;
      // let's draw as a circle with some lines inside
      pen.setWidthF(0.01*m_size); // in which units? this is shit ... rescaling does not work!
      pen.setWidthF(2.0);
      pen.setCosmetic(true) ;
      painter->setPen(pen) ;
      QRectF rectangle{-0.5*m_size,-0.5*m_size,m_size,m_size};
      painter->drawRect( rectangle ) ;
      painter->drawEllipse( rectangle ) ;
      painter->drawLine(QPointF(0,-L),QPointF(0,-0.5*m_size)) ;
      painter->drawLine(QPointF(0,+L),QPointF(0,+0.5*m_size)) ;
      painter->drawLine(QPointF(-L,0),QPointF(-0.5*m_size,0)) ;
      painter->drawLine(QPointF(+L,0),QPointF(+0.5*m_size,0)) ;
      
    }    
  } ;


  
   // Implements graphics item for stack axis marker. Center of marker is
  // center of system. Units are in microns!!!
  class StackAxisMarker : public QGraphicsItem
  {
  private:
    const double m_size = 2.0 ; // size in mm
  public:
    StackAxisMarker(QGraphicsItem *parent = Q_NULLPTR) : QGraphicsItem(parent)
    {
      setToolTip("StackAxis") ;
    } ;
      
    virtual QRectF boundingRect() const
    {
      qreal bound = 1.01 * m_size ;
      return QRectF{-0.5*bound,-0.5*bound,bound,bound} ;
    }

    virtual void paint(QPainter *painter,
		       const QStyleOptionGraphicsItem* /*option*/,
		       QWidget* /*widget*/)
    {
      // just a simple cross
      QPen pen ;
      pen.setColor( QColor{232, 93, 13} ) ;
      pen.setWidthF( 0.5 ) ;
      painter->setPen(pen) ;
      const double L = m_size ;
      painter->drawLine(QPointF(0,-L),QPointF(0,L)) ;
      painter->drawLine(QPointF(-L,0),QPointF(L,0)) ;
    }    
  } ;

  
  

}
#endif

