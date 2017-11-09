#ifndef NOMINALMARKERS_H
#define NOMINALMARKERS_H

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
    
    std::vector<FiducialDefinition> jigNCSide()
      {
	return std::vector<FiducialDefinition>
	  {
	    //{"ModuleJigMarker1",-81.0,  35.0},
	    //{"ModuleJigMarker2",+62.5,-146.3}
	    {"ModuleJigMarker1", -35.0,-81.0},
	    {"ModuleJigMarker2",+146.3,+62.5}
	  } ;
      }
  }
}

#endif