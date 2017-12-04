#ifndef MOTIONAXISPARAMETER
#define MOTIONAXISPARAMETER

#include "NamedValue.h"

namespace PAP
{
  class MotionAxis ;
  namespace MSCommandLibrary {
    struct ParameterDefinition ;
  }
  
  //template<typename T>
  class MotionAxisParameter : public QObject
  {
    Q_OBJECT
  public:
    using SetValueType = NamedVariant ;
    using GetValueType = MonitoredValue<QVariant> ;
    MotionAxisParameter(MotionAxis& axis,
			const MSCommandLibrary::ParameterDefinition& pardef) ;

    SetValueType& setValue() { return m_setValue ; }
    GetValueType& getValue() { return m_getValue ; }
    const SetValueType& setValue() const { return m_setValue ; }
    const GetValueType& getValue() const { return m_getValue ; }
    const MSCommandLibrary::ParameterDefinition& pardef() const { return *m_pardef ; }
    
    // this bypasses the other set method of NamedValue. it
    // immediately sends an update command to the motion system
    //void setParameter( const ValueType& value, bool autoupdate = true ) ;
    // explicitly called 
    //void getParameter() ;

  public slots:
    void read() ;
    void write() ;
    void initSetValue() ;
  private:
    const MotionAxis* m_axis ;
    const MSCommandLibrary::ParameterDefinition* m_pardef ;
    // this is the value that we send to the motion system
    SetValueType m_setValue ;
    // this is the value that we read back from the motion system
    GetValueType m_getValue ;
    // is the set value actually initialized?
    bool m_initialized ;
  } ;
}


#endif // MOTIONAXISPARAMETERS_H
