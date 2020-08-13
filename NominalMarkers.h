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
            {"CLI_VP02_Fid2", 18.993,11.371},
	    {"CLI_VP00_Fid2", -1.152,31.516},
	    {"CLI_VP01_Fid1", -0.997,31.361},
	    {"CLI_VP01_Fid2",  8.920,21.444},
	    {"CLI_VP02_Fid1",  9.076,21.288},
	    {"CLI_VP00_Fid1",-11.069,41.433}
	  } ;
      }

    inline std::vector<FiducialDefinition> velopixCSO()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"CSO_VP32_Fid2", 21.210,-28.599},
            {"CSO_VP30_Fid2", 41.355, -8.454},
            {"CSO_VP31_Fid1", 41.200, -8.609},
            {"CSO_VP31_Fid2", 31.283,-18.526},
            {"CSO_VP32_Fid1", 31.127,-18.682},
	    {"CSO_VP30_Fid1", 51.273,  1.464}
	  } ;
      }

    inline std::vector<FiducialDefinition> velopixNSI()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"NSI_VP20_Fid1", 11.487,-18.876},
            {"NSI_VP20_Fid2", 21.404, -8.959},
            {"NSI_VP21_Fid1", 21.560, -8.804},
            {"NSI_VP21_Fid2", 31.477,  1.113},
            {"NSI_VP22_Fid1", 31.633,  1.269},
            {"NSI_VP22_Fid2", 41.550, 11.186}
	  } ;
      }

    inline std::vector<FiducialDefinition> velopixNLO()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"NLO_VP10_Fid1", 27.744, 22.066},
	    {"NLO_VP10_Fid2", 17.826, 31.983},
            {"NLO_VP11_Fid1", 17.671, 32.138},
            {"NLO_VP11_Fid2",  7.754, 42.055},
            {"NLO_VP12_Fid1",  7.598, 42.211},
            {"NLO_VP12_Fid2", -2.319, 52.128}
	  } ;
      }

    inline std::vector<FiducialDefinition> velopixCLISensor()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"CLI_Sensor_Fid1",-22.794,29.479},
	    {"CLI_Sensor_Fid2",-22.794,30.257},
	    {"CLI_Sensor_T40",+5.6374,1.0483},//+5.676,1.009},
	    {"CSO_Sensor_Fid3",19.200,-27.115},
	    {"CSO_Sensor_L230",10.382,-18.319}
	  } ;
      }
    
    inline std::vector<FiducialDefinition> velopixNSISensor()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"NSI_Sensor_Fid1",-0.237,-6.923},
	    {"NSI_Sensor_Fid2",-0.237,-7.700},
	    {"NSI_Sensor_T610",5.793,-0.893},
	    {"NSI_Sensor_R30",8.437,-16.374}
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

    // Measured by Martijn in July 2020.
    inline std::vector<FiducialDefinition> turnJigB()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"MainJigMarker1", -50.466, 60.219},
	    {"MainJigMarker2",  19.853, -70.455}
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
    
    // Measured on NRD007
    inline std::vector<FiducialDefinition> microchannelNSide()
      {
	return std::vector<FiducialDefinition> {
	  {"MC_N19_NLO_Fid1",28.284, 22.061},
	  {"MC_N16_NLO_Fid2",-2.237, 52.600},
	  {"MC_N17_NLO_Fid3",-15.202, 39.696},
	  {"MC_N18_NLO_Fid4", 14.741, 9.732},
	  {"MC_N21_NSI_Fid1",11.474, -19.055},
	  {"MC_N20_NSI_Fid2",42.007, 11.470},
	  {"MC_NSI_Fid3",10.1,5.34},
	  {"MC_N14",56.615, 68.018},
	  {"MC_N15",13.104, 68.032},
	  {"MC_N12",38.665, -44.478},
	  {"MC_N13",80.084, -44.489}
	} ;
      }
    
    // Measured on NRD007
    inline std::vector<FiducialDefinition> microchannelCSide()
      {
	return std::vector<FiducialDefinition> {
	  {"MC_C04_CLI_Fid1",-11.304, 41.907},
	  {"MC_C07_CLI_Fid2", 19.222, 11.371},
	  {"MC_C06_CLI_Fid3", 13.350,  2.466},
	  {"MC_C05_CLI_Fid4",-19.851, 35.682},
	  {"MC_C09_CSO_Fid1", 51.885,  1.592},
	  {"MC_C11_CSO_Fid2", 21.348,-28.934},
	  {"MC_C10_CSO_Fid3",  8.926, -15.310},
	  {"MC_C08_CSO_Fid4", 41.191, 14.114},
	  {"MC_C00",  38.666, -44.476},
	  {"MC_C01",  80.086, -44.487},
	  {"MC_C_B1", 88.192, -14.040},
	  {"MC_C_B2", 79.991, -14.036},
	  {"MC_C_B3", 80.050, 14.012},
	  {"MC_C_B4", 88.198, 14.059}
	} ;
      }


    
  }
}

#endif
