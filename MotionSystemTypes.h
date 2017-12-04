#ifndef MOTIONSYSTEMTYPES_H
#define MOTIONSYSTEMTYPES_H

namespace PAP
{
  
  using MotionControllerID = int ;
  
  
  struct MotionAxisID
  {
  MotionAxisID(int c=1, int a=1) : controller(c),axis(a) {}
    int controller ;
    int axis ;
    bool operator<(const MotionAxisID& rhs) const {
      return controller < rhs.controller ||
	( controller == rhs.controller && axis < rhs.axis ) ;
    }
  };

  /*
  class MSParameter : public NamedVariant
  {
  public:
    MSParameter( const QString& name, QVariant::Type type=QVariant::Invalid ) : NamedValue(name,type) {}
    MSParameter( const QString& name, const ValueType& v ) : NamedValue(name,v) {}
    MSParameter( const QString& name, const ValueType& v, const ValueType& min, const ValueType& max)
      : NamedValue(name,v,min,max) {}
    virtual ~MSParameter() {}
  } ;
  */

  /*
  struct MotionControllerData
  {
    int status ;
    int error ;
  } ;
  */

}


#endif // MOTIONSYSTEMTYPES_H
