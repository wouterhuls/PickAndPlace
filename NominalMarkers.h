#ifndef NOMINALMARKERS_H
#define NOMINALMARKERS_H

#include "Coordinates.h"

namespace PAP
{
  // Names and position of markers in the LHCb coordinate system.
  
  // Only if we look from the Nside then X and Y have the nominal
  // orientation. If we look from the Cside all x coordinates change
  // sign.

  namespace Markers {

    inline std::vector<FiducialDefinition> velopixCLI()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"CLI_VP00_Fid1",-11.0695,41.4332},
	    {"CLI_VP00_Fid2", -1.1523,31.5161},
	    {"CLI_VP01_Fid1", -0.9967,31.3605},
	    {"CLI_VP01_Fid2",  8.9205,21.4433},
	    {"CLI_VP02_Fid1",  9.0760,21.2878},
            {"CLI_VP02_Fid2", 18.9932,11.3706},
	  } ;
      }

    inline std::vector<FiducialDefinition> velopixCSO()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"CSO_VP30_Fid1", 51.2726,  1.4634},
            {"CSO_VP30_Fid2", 41.3554, -8.4538},
            {"CSO_VP31_Fid1", 41.1999, -8.6093},
            {"CSO_VP31_Fid2", 31.2827,-18.5265},
            {"CSO_VP32_Fid1", 31.1271,-18.6821},
	    {"CSO_VP32_Fid2", 21.2100,-28.5992},
	  } ;
      }

    inline std::vector<FiducialDefinition> velopixNSI()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"NSI_VP20_Fid1", 11.4873,-18.8765},
            {"NSI_VP20_Fid2", 21.4044, -8.9593},
            {"NSI_VP21_Fid1", 21.5600, -8.8038},
            {"NSI_VP21_Fid2", 31.4772,  1.1134},
            {"NSI_VP22_Fid1", 31.6327,  1.2690},
            {"NSI_VP22_Fid2", 41.5499, 11.1861}
	  } ;
      }

    inline std::vector<FiducialDefinition> velopixNLO()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"NLO_VP10_Fid1", 27.7436, 22.0656},
	    {"NLO_VP10_Fid2", 17.8265, 31.9827},
            {"NLO_VP11_Fid1", 17.6709, 32.1383},
            {"NLO_VP11_Fid2",  7.7537, 42.0555},
            {"NLO_VP12_Fid1",  7.5982, 42.2110},
            {"NLO_VP12_Fid2", -2.3190, 52.1282}
	  } ;
      }

    inline std::vector<FiducialDefinition> velopixCLISensor()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"CLI_Sensor_Fid1",-22.7936,29.4793},
	    {"CLI_Sensor_Fid2",-22.7936,30.2571},
	    {"CLI_Sensor_T40",+5.6374,1.0483},
	    {"CSO_Sensor_Fid3",19.1732,-27.1105},
	    {"CSO_Sensor_L230",10.3821,-18.3194}
	  } ;
      }
    
    inline std::vector<FiducialDefinition> velopixNSISensor()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"NSI_Sensor_Fid1",-0.2369,-6.9226},
	    {"NSI_Sensor_Fid2",-0.2369,-7.7004},
	    {"NSI_Sensor_T610",5.7930,-0.8927},
	    {"NSI_Sensor_R30",8.4376,-16.3748}
	  } ;
      }

    // Martijn's measurements 10/01/2018:
    //  X Value_Marker 1 (ViScan 1x)    142.306
    //  Y Value_Marker 1 (ViScan 1x)    165.154
    //  X Value_Marker 2 (ViScan 1x)    37.117
    //  Y Value_Marker 2 (ViScan 1x)    325.040
    // X and Y are swapped wrt LHCb frame. Measurements are These are
    // wrt to the pen on the foot. This pen is at y=97 x=275 in the
    // LHCb frame.
    // See my root macro "markermeasurements.cxx"
    
    inline std::vector<FiducialDefinition> turnJigA()
      {
	return std::vector<FiducialDefinition>
	  {
	    //{"ModuleJigMarker1",-81.0,  35.0},
	    //{"ModuleJigMarker2",+62.5,-146.3}
	    //{"MainJigMarker1", -35.0,-81.0},
	    //{"MainJigMarker2",+146.3,+62.5}
	    // {"MainJigMarker1", -50.0,-60.0}, // NOMINAL
	    // {"MainJigMarker2",+110.0,+45.0}  // NOMINAL
	    //{"MainJigMarker1", -50.040,-59.883}, // Measured 10/01/2018
	    //{"MainJigMarker2",+109.846,+45.306}  //  Measured 10/01/2018
	    //{"MainJigMarker1NSide",-50.0128, -59.8926}, // Measured November 2018
	    //{"MainJigMarker2NSide",109.857, 45.2993},
	    //{"MainJigMarker1CSide",-50.0215,-59.9006},
	    //{"MainJigMarker2CSide",109.852, 45.2918}
	    {"MainJigMarker1",-50.017, -59.897}, // Averaged over N and C side
	    {"MainJigMarker2",109.854, 45.296}   // idem 
	  } ;
      }

    // Measured by Martijn in September 2020.
    // (This is acually for the third jig, but we'll never use he 2nd.)
    // Measurements were updated in November 2020. After that the dowelpin was still adjusted.
    inline std::vector<FiducialDefinition> turnJigB()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"MainJigMarker1", -50.418, 60.426},
	    {"MainJigMarker2",  19.632, -70.501}
	  } ;
      }
    // Measured by Martijn in November 2018, then translated into the
    // module frame. For one of these two sets, I need to change the
    // sign of the y-coordinate. Note that the Z-coordinate is always
    // positive. That's something we may want to change at some point.
    inline std::vector<FiducialDefinition> jigSurfaceCSide()
    {
      return std::vector<FiducialDefinition>{
	//{"JigSurfaceCSidePoint1", -49.6, +57.2, 12.371},
	//{"JigSurfaceCSidePoint2", 109.7, +56.2, 12.439},
	//{"JigSurfaceCSidePoint3", 110.6, -54.7, 12.444},
	//{"JigSurfaceCSidePoint4", -49.9, -49.3, 12.325}
	// Corrected by adaptive fitting a plane through all 8 (nside+cside) measurements
	{"JigSurfaceCSidePoint1", -49.6, +57.2, 12.3645},
	{"JigSurfaceCSidePoint2", 109.7, +56.2, 12.4391},
	{"JigSurfaceCSidePoint3", 110.6, -54.7, 12.4492},
	{"JigSurfaceCSidePoint4", -49.9, -49.3, 12.3737}
      } ;
    } ;
    
 
    
    inline std::vector<FiducialDefinition> jigSurfaceNSide()
    {
      return std::vector<FiducialDefinition>{
	//{"JigSurfaceNSidePoint1",	-49.6,	 57.2,	12.698},
	//{"JigSurfaceNSidePoint2",	109.7,	 56.2,	12.624},
	//{"JigSurfaceNSidePoint3",	110.6,	-54.7,	12.594},
	//{"JigSurfaceNSidePoint4",	-49.9,	-49.3,	12.690}
	// Corrected by adaptive fitting a plane through all 8 (nside+cside) measurements
	{"JigSurfaceNSidePoint1",	-49.6,	 57.2,	12.6955},
	{"JigSurfaceNSidePoint2",	109.7,	 56.2,	12.6209},
	{"JigSurfaceNSidePoint3",	110.6,	-54.7,	12.6108},
	{"JigSurfaceNSidePoint4",	-49.9,	-49.3,	12.6863}};
    } ;

    inline std::vector<FiducialDefinition> jigBSurfaceCSide()
    {
      return std::vector<FiducialDefinition>{
        // these were measured with a caliper assuming that the Nside surface is nominal (-12.5)
	// they suggest a level of precision that is not there :-)
	//{"JigBSurfaceCSidePoint1",	-50,	 73,	12.468},
	//{"JigBSurfaceCSidePoint2",	-50,	-73,	12.462},
	//{"JigBSurfaceCSidePoint3",	 32,	-70.5,	-0.114},
        //{"JigBSurfaceCSidePoint4",	 6,	-58,	-0.127},
        //{"JigBSurfaceCSidePoint5",	 -32,	64,	-0.176},
	//{"JigBSurfaceCSidePoint6",	 -50,	45,	-0.194},
	//{"JigBSurfaceCSidePoint7",	110,	 65,	-10.578},
	//{"JigBSurfaceCSidePoint8",	110,	-65,	-10.564},
	//{"JigBSurfaceCSidePoint9",	 35,	-65,	-10.610}
	{"JigBSurfaceCSidePoint1",	-50,	 73,	12.460}, // first three I trust. is three enough?
	{"JigBSurfaceCSidePoint2",	-50,	-73,	12.460},
	{"JigBSurfaceCSidePoint3",	  7,	-73,	12.460},
	//{"JigBSurfaceCSidePoint4",	 32,	-70.5,	-0.126}, // These four are nice, but I don't think they help.
        //{"JigBSurfaceCSidePoint5",	 6,	-58,	-0.138},
        //{"JigBSurfaceCSidePoint6",	 -32,	64,	-0.187},
	//{"JigBSurfaceCSidePoint7",	 -50,	45,	-0.195},
	{"JigBSurfaceCSidePoint8",	110,	 67,	-10.590},
	{"JigBSurfaceCSidePoint9",	110,	-67,	-10.567}, // this one comes out really bad: is jig bent ?
	{"JigBSurfaceCSidePoint10",	 50,	-73,	-10.620}
      } ;
    } ;
    
    inline std::vector<FiducialDefinition> jigBSurfaceNSide()
    {
      return std::vector<FiducialDefinition>{
	{"JigBSurfaceNSidePoint1",	-50,	 73,	12.5},
	{"JigBSurfaceNSidePoint2",	155,	 73,	12.5},
	{"JigBSurfaceNSidePoint3",	155,	-73,	12.5},
	{"JigBSurfaceNSidePoint4",	-50,	-73,	12.5}
      } ;
    } ;
    
    // Measured on NRD007
    inline std::vector<FiducialDefinition> microchannelNSide()
      {
	return std::vector<FiducialDefinition> {
	  {"MC_N19_NLO_Fid1",28.18,   22.04},
	  {"MC_N16_NLO_Fid2",-2.35,   52.57},
	  {"MC_N17_NLO_Fid3",-15.31,   39.66},
	  {"MC_N18_NLO_Fid4", 14.64,    9.70},
	  {"MC_N21_NSI_Fid1",11.38,  -19.08},
	  {"MC_N20_NSI_Fid2",41.91,   11.45},
	  {"MC_NSI_Fid3",10.1,5.34},
	  {"MC_N12",   38.58,  -44.50},
	  {"MC_N13",   80.00,  -44.50},
	  {"MC_N14",   56.50,   68.00},
	  {"MC_N15",   12.99,   68.00},
	  {"MC_N_B1",   88.10,   14.05}, // not sure these are visible: otherwise we should remove them
	  {"MC_N_B2",   79.90,   14.05},
	  {"MC_N_B3",   79.90,  -14.05},
	  {"MC_N_B4",   88.10,  -14.05}
	} ;
      }
    
    // Measured on NRD007
    inline std::vector<FiducialDefinition> microchannelCSide()
      {
	return std::vector<FiducialDefinition> {
	  {"MC_C04_CLI_Fid1",-11.41, 41.87},
	  {"MC_C07_CLI_Fid2", 19.12, 11.34},
	  {"MC_C06_CLI_Fid3", 13.25,  2.44},
	  {"MC_C05_CLI_Fid4",-19.95, 35.64},
	  {"MC_C09_CSO_Fid1", 51.79,  1.57},
	  {"MC_C11_CSO_Fid2", 21.26,-28.96},
	  {"MC_C10_CSO_Fid3",  8.83,-15.34},
	  {"MC_C08_CSO_Fid4", 41.09, 14.09},
	  {"MC_C00",  38.58, -44.50},
	  {"MC_C01",  80.00, -44.50},
	  {"MC_C02",  56.50,  68.00},
	  {"MC_C03",  12.99,  68.00},
	  {"MC_C_B1", 88.10, -14.05},  // actually B4
	  {"MC_C_B2", 79.90, -14.05},  // actually B3
	  {"MC_C_B3", 79.95, 14.00},   // actually B2
	  {"MC_C_B4", 88.10, 14.05},   // atcually B1
	} ;
      }
  }
}

#endif
