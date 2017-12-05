#ifndef PAP_COORDINATES_H
#define PAP_COORDINATES_H

#include <QString>

namespace PAP
{
  enum ViewDirection { NSideView = 0 , CSideView=1 } ;
  
  struct Coordinates2D
  {
  Coordinates2D( double _x=0, double _y=0, double _phi=0 ) : x(_x),y(_y),phi(_phi) {}
    double x ;
    double y ;
    double phi ;
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
    FiducialDefinition( QString _name, double _x, double _y)
    : name(_name),x(_x),y(_y) {}
    QString name ;
    double x ;
    double y ;
  } ;
}

#endif
