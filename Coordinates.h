#ifndef PAP_COORDINATES_H
#define PAP_COORDINATES_H

#include <QString>
#include <QPointF>
#include <QVector3D>

namespace PAP
{
  enum ViewDirection { NSideView = 0 , CSideView=1, NumViews=2 } ;
  enum TileType { ShortSide=0, LongSide=1, NumTiles=2} ;
  
  template<typename T>
    struct BoundingRange
    {
      T min{std::numeric_limits<T>::max()} ;
      T max{std::numeric_limits<T>::min()} ;
      void add(const T& x) {
        if(min>x)      min=x ;
        else if(max<x) max=x ;
      }
    } ;
  
  template<typename T=double>
    struct BoundingBox
    {
      BoundingRange<T> x, y ;
      void add( double ax, double ay ) {
        x.add(ax) ;
        y.add(ay) ;
      }
    } ;
  
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

  using Coordinates3D = QVector3D ;
  
  struct MSMainCoordinates
  {
    MSMainCoordinates( double _x=0, double _y=0 ) : x(_x),y(_y) {}
    double x{0} ;
    double y{0} ;
  } ;

  struct MSStackCoordinates
  {
    MSStackCoordinates(double _x=0, double _y=0, double _phi=0)
    : x{_x},y{_y},phi{_phi}{}
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

  using ModuleCoordinates = Coordinates3D ;

  struct FiducialDefinition
  {
  FiducialDefinition( const QString& _name, double _x, double _y, double _z=0)
    : name{_name},x{_x},y{_y},z{_z} {}
    QString name ;
    double x ;
    double y ;
    double z ;
  } ;

  struct TileInfo
  {
  TileInfo( const char* n, const char* m1, const char* m2)
  : name{n},marker1{m1},marker2{m2}{}
    QString name ;
    QString marker1 ;
    QString marker2 ;
  } ;
  TileInfo getTileInfo( ViewDirection view, TileType tile ) ;
}

#endif
