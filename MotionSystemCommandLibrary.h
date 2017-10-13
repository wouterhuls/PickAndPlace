#ifndef MotionSystemCommandLibrary_H
#define MotionSystemCommandLibrary_H

namespace PAP
{
  namespace MSCommandLibrary
  {
    struct ParameterDefinition
    {
    ParameterDefinition(const char* _name,
			const char* _getcmd,
			const char* _setcmd,
			QVariant::Type _type,
			bool _configurable=false,
			double _minvalue = 0,
			double _maxvalue = 1,
			int _decimals = 1) :
      name(_name),getcmd(_getcmd),setcmd(_setcmd),type(_type),configurable(_configurable),
	minvalue(_minvalue),maxvalue(_maxvalue),decimals(_decimals) {}
      const char* name ;
      const char* getcmd ;
      const char* setcmd ;
      QVariant::Type type ;
      bool configurable ;
      double minvalue ;
      double maxvalue ;
      int decimals ;
    } ;
    
    static const std::vector<ParameterDefinition> Parameters {
      { "ManualVelocity","DM","MH", QVariant::Double,true,0.0,1.0,3 },
      { "LeftTravelLimit","TL","SL", QVariant::Double,true,0.0,1.0,3},
      { "RightTravelLimit","TR","SR", QVariant::Double,true,0.0,1.0,3 },
      { "Units","TN","SN",QVariant::String,true},
      { "Position","TP","PA",QVariant::Double,false,-200,200,3},	  
    } ;
    
    inline const ParameterDefinition* findParDef( const char* name )
    {
      const ParameterDefinition* rc(0) ;
      for( const auto& ipar : Parameters )
	if( strcmp(name,ipar.name)==0 ) {
	  rc = &ipar ;
	  break ;
	}
      return rc ;
    }

    template<class T> const ParameterDefinition* findParDef( const T& name )
    {
      const ParameterDefinition* rc(0) ;
      for( const auto& ipar : Parameters )
	if( name==T{ipar.name} ) {
	  rc = &ipar ;
	  break ;
	}
      return rc ;
    }

    inline const ParameterDefinition* findParDef( const QStringRef& name )
    {
      return findParDef( name.toString() ) ;
    }
  }
}


#endif
