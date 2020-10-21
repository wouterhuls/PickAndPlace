#ifndef STACKCALIBRATION_H
#define STACKCALIBRATION_H

#include <QWidget>
#include "Coordinates.h"

#include <Eigen/Dense>

class QTableWidget ;

namespace PAP
{
  class StackCalibration : public QWidget
  {
  private:
    QPointF m_taggedCameraInStack ;
    std::vector<MSCoordinates> m_measurements ;
    enum { DIM = 9 } ; // 9 parameters, including marker position
    Eigen::Matrix<double,1,DIM>   m_pars ;
    Eigen::Matrix<double,DIM,DIM> m_cov  ;
    QTableWidget* m_table ; // table to show the current value of the parameters
    const double m_sigma2 = 0.01*0.01 ; // covariance for a measured coordinate
  public:
    StackCalibration( QWidget* parent=0 ) ;
    void tag() ;
    void track() const ;
    void calibrate() ;
    void filter() ;
    void initialize() ;
    void update() ;
    void updateTable() ;
    void exportToFile() const ;
    void importFromFile() ;
    
  } ;
}
#endif
