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
  
    std::vector<FiducialDefinition> velopixCLI()
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

    std::vector<FiducialDefinition> velopixCSO()
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

    std::vector<FiducialDefinition> velopixNSI()
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

    std::vector<FiducialDefinition> velopixNLO()
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

    std::vector<FiducialDefinition> velopixCLISensor()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"CLI_Sensor_Fid1",-22.85,29.479},
	    {"CLI_Sensor_Fid2",-22.85,30.257}
	  } ;
      }
    
    std::vector<FiducialDefinition> velopixNSISensor()
      {
	return std::vector<FiducialDefinition>
	  {
	    {"NSI_Sensor_Fid1",-0.294,-6.923},
	      {"NSI_Sensor_Fid2",-0.294,-7.700}
	  } ;
      }
    
    std::vector<FiducialDefinition> jigNCSide()
      {
	return std::vector<FiducialDefinition>
	  {
	    //{"ModuleJigMarker1",-81.0,  35.0},
	    //{"ModuleJigMarker2",+62.5,-146.3}
	    //{"MainJigMarker1", -35.0,-81.0},
	    //{"MainJigMarker2",+146.3,+62.5}
	    // {"MainJigMarker1", -50.0,-60.0}, // NOMINAL
	    // {"MainJigMarker2",+110.0,+45.0}  // NOMINAL
	    {"MainJigMarker1", -50.040,-59.883}, // Measured 10/01/2018
	    {"MainJigMarker2",+109.846,+45.306}  //  Measured 10/01/2018
	  } ;
	
      }

    std::vector<FiducialDefinition> microchannelNSide()
      {
	return std::vector<FiducialDefinition> {
	  {"MC_NLO_Fid1",28.16,22.39},
	  {"MC_NLO_Fid2",-2.51,52.79},
	  {"MC_NLO_Fid3",-15.41,39.82},
	  {"MC_NLO_Fid4", 14.68,10.00},
	  {"MC_NSI_Fid1",11.78,-18.93},
	  {"MC_NSI_Fid2",41.94,11.79},
	  {"MC_NSI_Fid3",10.1,5.34}
	} ;
      }
    
    std::vector<FiducialDefinition> microchannelCSide()
      {
	return std::vector<FiducialDefinition> {
	  {"MC_CLI_Fid1",-11.42, 41.78},
	  {"MC_CLI_Fid2", 19.60, 11.2},
	  {"MC_CLI_Fid3", 13.25,  2.35},
	  {"MC_CLI_Fid4",-19.97, 35.54},
	  {"MC_CSO_Fid1", 51.79,  1.49},
	  {"MC_CSO_Fid2", 21.27,-29.05},
	  {"MC_CSO_Fid3",  8.84,-15.43},
	  {"MC_CSO_Fid4", 41.09, 14.01}
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
    
    
  }
}

#endif
