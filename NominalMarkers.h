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
	    {"CLI_Sensor_Fid1",-22.85,29.479},
	    {"CLI_Sensor_Fid2",-22.85,30.257}
	  } ;
      }
    
    inline std::vector<FiducialDefinition> velopixNSISensor()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"NSI_Sensor_Fid1",-0.294,-6.923},
	      {"NSI_Sensor_Fid2",-0.294,-7.700}
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
    
    inline std::vector<FiducialDefinition> jigNCSide()
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
    
    // Measured on NRD006
    inline std::vector<FiducialDefinition> microchannelNSide()
      {
	return std::vector<FiducialDefinition> {
	  {"MC_NLO_Fid1",28.17,22.39},
	  {"MC_N19",     28.20,21.95},
	  {"MC_N16_Fid2",-2.47,52.81},
	  {"MC_N16",     -2.33,52.48},
	  {"MC_N17_NLO_Fid3",-15.38,39.86},
	  {"MC_N18_NLO_Fid4", 14.67,10.01},
	  {"MC_N21_NSI_Fid1",11.78,-18.93},
	  {"MC_N20_NSI_Fid2",41.93,11.85},
	  {"MC_NSI_Fid3",10.1,5.34},
	  {"MC_N14",56.32,68.46},
	  {"MC_N15",12.81,68.30},
	  {"MC_N12",38.80,-44.11},
	  {"MC_N13",80.19,-44.02}
	} ;
      }
    
    // Measured on NRD006
    inline std::vector<FiducialDefinition> microchannelCSide()
      {
	return std::vector<FiducialDefinition> {
	  {"MC_C04_CLI_Fid1",-11.39, 41.78},
	  {"MC_C07_CLI_Fid2", 19.14, 11.25},
	  {"MC_C06_CLI_Fid3", 13.27,  2.34},
	  {"MC_C05_CLI_Fid4",-19.94, 35.55},
	  {"MC_C09_CSO_Fid1", 51.81,  1.49},
	  {"MC_C11_CSO_Fid2", 21.29,-29.05},
	  {"MC_C10_CSO_Fid3",  8.86,-15.43},
	  {"MC_C08_CSO_Fid4", 41.11, 14.00},
	  {"MC_C00", 38.62,-44.58},
	  {"MC_C01", 80.03,-44.58},
	  {"MC_C_B1",  88.12,-14.13},
	  {"MC_C_B2",  79.93,-14.13},
	  {"MC_C_B3",  79.92,+13.96},
	  {"MC_C_B4",  88.12,+13.97}
	} ;
      }


    
  }
}

#endif
