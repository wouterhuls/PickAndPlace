#ifndef PAP_COORDINATES_H
#define PAP_COORDINATES_H

namespace PAP
{
  struct Coordinates2D
  {
    Coordinates2D( double _x=0, double _y=0 ) : x(_x),y(_y) {}
    double x ;
    double y ;
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
