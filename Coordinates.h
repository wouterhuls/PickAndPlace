#ifndef PAP_COORDINATES_H
#define PAP_COORDINATES_H

#include <QString>
#include <QPointF>

namespace PAP
{
  enum ViewDirection { NSideView = 0 , CSideView=1 } ;
  
  class Coordinates2D : public QPointF
  {
  public:
    Coordinates2D( double x=0, double y=0, double phi=0 ) : QPointF{x,y},m_phi{phi} {}
    Coordinates2D( const QPointF& p ) : QPointF{p},m_phi{0} {}
    qreal phi() const { return m_phi; }
    qreal x() const { return QPointF::x(); }
    qreal y() const { return QPointF::y(); }
    qreal& phi() { return m_phi; }
    qreal& x() { return rx(); }
    qreal& y() { return ry(); }
    
  private:
    qreal m_phi ;
  } ;
  
  struct MSMainCoordinates
  {
    MSMainCoordinates( double _x=0, double _y=0 ) : x(_x),y(_y) {}
    double x ;
    double y ;
  } ;

  struct MSStackCoordinates
  {
    double x ;
    double y ;
    double phi ;
  } ;
  
  struct MSCoordinates
  {
    MSMainCoordinates main ;
    MSStackCoordinates stack ;
    double focus ;
  } ;

  struct FiducialDefinition
  {
    FiducialDefinition( const QString& _name, double _x, double _y)
    : name(_name),x(_x),y(_y) {}
    QString name ;
    double x ;
    double y ;
  } ;
}

#endif
