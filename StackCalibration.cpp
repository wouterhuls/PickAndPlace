#include "StackCalibration.h"
#include "MotionSystemSvc.h"
#include "GeometrySvc.h"

#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QFileDialog>
#include <QStringList>

#include <cmath>
#include <Eigen/Dense>
#include <iostream>
#include <fstream>
#include <set>

namespace PAP
{
  // Functionality that we need:
  // - some way to make a list of measurements. each measurement just records the position of all axes
  // - a way to display these measurements such that you can also remove those that were a mistake
  // - a fit

  StackCalibration::StackCalibration( QWidget* parent )
    : QWidget( parent )
  {
    auto hlayout = new QVBoxLayout{} ;
    this->setLayout(hlayout) ;
    
    auto tmptext1 = new QLabel{this} ;
    tmptext1->setText("The idea is that by taking a number of different measurements at different positions,"
		      "we should be able to calibrate the small stage such that we can follow a marker with the main stage"
		      "while arbitrarily moving the marker with the stack") ;
    tmptext1->setWordWrap(true);
    hlayout->addWidget( tmptext1 ) ;

    auto tagbutton = new QPushButton{"Tag"} ;
    hlayout->addWidget(tagbutton) ;
    connect(tagbutton,&QPushButton::clicked,[=]{ this->tag() ; } ) ;
    
    auto trackbutton = new QPushButton{"Track"} ;
    hlayout->addWidget(trackbutton) ;
    connect(trackbutton,&QPushButton::clicked,
	    [=]{ this->track() ; } ) ;

    auto recordbutton = new QPushButton{"Record"} ;
    hlayout->addWidget(recordbutton) ;
    connect(recordbutton,&QPushButton::clicked,
	    [=]{ this->m_measurements.push_back( MotionSystemSvc::instance()->coordinates() ) ; } ) ;
    
    auto clearbutton = new QPushButton{"Clear"} ;
    hlayout->addWidget(clearbutton) ;
    connect(clearbutton,&QPushButton::clicked,
	    [=]{ this->m_measurements.clear() ; } ) ;
    
    auto calibratebutton = new QPushButton{"Calibrate"} ;
    hlayout->addWidget(calibratebutton) ;
    connect(calibratebutton,&QPushButton::clicked,[=]{ this->calibrate() ; } ) ;
    
    auto initbutton = new QPushButton{"Initialize"} ;
    hlayout->addWidget(initbutton) ;
    connect(initbutton,&QPushButton::clicked,[=]{ this->initialize() ; } ) ;
    

    auto filterbutton = new QPushButton{"Filter"} ;
    hlayout->addWidget(filterbutton) ;
    connect(filterbutton,&QPushButton::clicked,[=]{ this->filter() ; } ) ;
    
    auto updatebutton = new QPushButton{"Update geometry"} ;
    hlayout->addWidget(updatebutton) ;
    connect(filterbutton,&QPushButton::clicked,[=]{ this->update() ; } ) ;
    
    {
      auto filelayout = new QHBoxLayout{} ;
      auto exportbutton = new QPushButton{"Export"} ;
      filelayout->addWidget(exportbutton) ;
      connect(exportbutton,&QPushButton::clicked,[=]{ this->exportToFile() ; } ) ;
      auto importbutton = new QPushButton{"Import"} ;
      filelayout->addWidget(importbutton) ;
      connect(importbutton,&QPushButton::clicked,[=]{ this->importFromFile() ; } ) ;
      hlayout->addLayout( filelayout ) ;
    }
    
    m_table = new QTableWidget{9,2,this} ;
    m_table->setHorizontalHeaderLabels( { "value","error"} ) ;
    m_table->setVerticalHeaderLabels( { "tagX","tagY","X0","XA","XB","Y0","YA","YB","Phi0"} ) ;
    hlayout->addWidget(m_table) ;

    initialize() ;
        
    {
      // make a few buttons for predefined positions, such that this
      // takes me less time. we need a least 4 points.
      std::vector<MSStackCoordinates> refpoints = { { 0,  0, -1.5},
						    { 0,  25, -1.5},
						    { 25, 25, -1.5},
						    { 12.5, 12.5, -1.5},
						    { 12.5, 12.5, 0},
						    { 12.5, 12.5, 0.9} } ;
      auto stepbuttonlayout = new QHBoxLayout{} ;
      hlayout->addLayout(stepbuttonlayout) ;
      for(size_t i=0; i<refpoints.size(); ++i) {
	auto p = new QPushButton{QString{"P"} + QString::number(i) } ;
	stepbuttonlayout->addWidget(p) ;
	connect(p,&QPushButton::clicked,
		[=]{
		  auto mssvc = MotionSystemSvc::instance() ; 
		  mssvc->stackXAxis().moveTo( refpoints[i].x ) ;
		  mssvc->stackYAxis().moveTo( refpoints[i].y ) ;
		  mssvc->stackRAxis().moveTo( refpoints[i].phi ) ;
		} ) ;
      }
    }

  }

  void StackCalibration::updateTable()
  {
    for(int i=0; i<DIM; ++i) {
      if( !m_table->item(i,0) ) m_table->setItem(i,0,new  QTableWidgetItem{} ) ;
      if( !m_table->item(i,1) ) m_table->setItem(i,1,new  QTableWidgetItem{} ) ;
      m_table->item(i,0)->setText( QString::number( m_pars(i) ) ) ;
      m_table->item(i,1)->setText( QString::number( std::sqrt(m_cov(i,i)) ) ) ;
    }
  }
  
  // calibrate from a number of measurements
  
  void StackCalibration::calibrate()
  {
    //std::stringstream text ;
    auto& text = std::cout ;
    text << "Number of measurements: " << m_measurements.size() << std::endl ;
    // below we use transforms to go from one frame to another. but
    // to compute the parameters we need to explicitly model the
    // transforms, compute derivatives etc. in that process, we
    // hopefully also find the mistake in the transforms.
    
    // This is the transformation 'geomsvc::fromCameratoGlobal. We
    // do not touch that in this calibration step. Let's assume it is right.
    //  QTransform rc ;
    //  MSMainCoordinates main = MotionSystemSvc::instance()->maincoordinates() ;
    //  Coordinates2D offset = toGlobal( main ) ;
    //  rc.rotateRadians( m_cameraPhi ) ;
    //  rc.translate( offset.x(), offset.y() ) ;
    
    // This computes the transform for the stack axis.
    /*
      Coordinates2D GeometrySvc::stackAxisInGlobal( const MSStackCoordinates& c ) const
      {
      return Coordinates2D{
      m_stackX0 + m_stackXA * c.x + m_stackXB * c.y,
      m_stackY0 + m_stackYA * c.x + m_stackYB * c.y,
      m_stackPhi0 + c.phi
      } ; 
      }
      QTransform GeometrySvc::fromStackToGlobal() const
      {
      Coordinates2D coord = stackAxisInGlobal() ;
      QTransform rc ;
      rc.translate( coord.x(), coord.y() ) ;
      rc.rotateRadians( -coord.phi() ) ; // I do not know why I need the minus sign here :-(
      return rc ;
      }
      
    */
    
    // In the following we need compute stackX{0,A,B},
    // stackY{0,A,B}. These are 6 parameters in total, plus two
    // parameters for the position of the marker in the stack frame.
    
    auto geomsvc = GeometrySvc::instance() ;
    if(m_measurements.size()>0) {
      // for every measurement we
      // - compute the global coordinates from MSMain coordinates
      // - compute the prediction of this by using the transforms
      // - compute the residuals to the transform parameters
           
      // the position of the marker in the stack frame is also
      // unknown: so these are two extra parameters. we need to
      // initialize them with something, so we do that with the first
      // measurement.
      
      // compute the markerposition in the stack frame from the first measurement.
      QPointF markerposstackframe ;
      {
	const auto m0 =  m_measurements.front() ;
	const Coordinates2D markerglobalpos = geomsvc->toGlobal( m0.main ) ;
	const Coordinates2D stackaxis = geomsvc->stackAxisInGlobal( m0.stack ) ;
	const double dx = markerglobalpos.x() - stackaxis.x() ;
	const double dy = markerglobalpos.y() - stackaxis.y() ;
	// now just rotate in the stack frame: need to get the sign of the angle right:-)
	const double sinphi = std::sin(stackaxis.phi()) ;
	const double cosphi = std::cos(stackaxis.phi()) ;
	double dxprime = cosphi*dx - sinphi*dy ; 
	double dyprime = sinphi*dx + cosphi*dy ; 
	markerposstackframe = QPointF{dxprime,dyprime} ;
	text << "Approximate position of marker in stack frame: " << markerposstackframe.x() << "," << markerposstackframe.y() << std::endl ;
      }
      // the parameters are:
      // 0: marker pos stack frame x
      // 1: marker pos stack frame y
      // 2: m_stackX0
      // 3: m_stackY0
      // 4. m_stackXA
      // 5. m_stackYA 
      // 6. m_stackXB
      // 6: m_stackYB

      std::vector<std::string> parnames = {
	"tag X", "tag Y",
	"stack X0", "stack XA", "stack XB",
	"stack Y0", "stack YA", "stack YB" } ;
      enum { DIM = 8 } ;
      using VectorNd = Eigen::Matrix<double,DIM,1> ;
      using MatrixNd = Eigen::Matrix<double,DIM,DIM> ;
      VectorNd pars ;
      pars(0) = markerposstackframe.x() ;
      pars(1) = markerposstackframe.y() ;
      pars(2) = geomsvc->stackX0() ; 
      pars(3) = geomsvc->stackXA() ;
      pars(4) = geomsvc->stackXB() ;
      pars(5) = geomsvc->stackY0() ;
      pars(6) = geomsvc->stackYA() ;
      pars(7) = geomsvc->stackYB() ;
            
      const int maxnsteps = 5 ; 
      for( int istep=0; istep<maxnsteps; ++istep) {
	
	VectorNd halfdchi2dpar   = VectorNd::Zero() ;
	MatrixNd halfd2chi2dpar2 = MatrixNd::Zero() ;
	const double sigma2 = 0.005*0.005 ; // sigma = 5 micron
	
	for( const auto& m : m_measurements) {
	  // now we do the same, but with inverse transformations, compute residual, etc
	  Coordinates2D markerglobalposref = geomsvc->toGlobal( m.main ) ;
	  const Coordinates2D stackaxis = {
	    pars(2) + pars(3) * m.stack.x + pars(4) * m.stack.y,
	    pars(5) + pars(6) * m.stack.x + pars(7) * m.stack.y,
	    m.stack.phi } ;
	  const double sinphi = std::sin(stackaxis.phi()) ;
	  const double cosphi = std::cos(stackaxis.phi()) ;
	  const double x = stackaxis.x() + pars(0) * cosphi + pars(1) * sinphi ;
	  const double y = stackaxis.y() - pars(0) * sinphi + pars(1) * cosphi ;
	  
	  const double residualX = x - markerglobalposref.x() ;
	  const double residualY = y - markerglobalposref.y() ;
	  
	  // x and y factorise, so just do them one by one
	  VectorNd deriv ;
	  {
	    // derivatives of residual X
	    deriv(0) = cosphi ;
	    deriv(1) = sinphi ;
	    deriv(2) = 1 ;
	    deriv(3) = m.stack.x ;
	    deriv(4) = m.stack.y ;
	    deriv(5) = 0 ;
	    deriv(6) = 0 ;
	    deriv(7) = 0 ;
	    halfdchi2dpar += residualX/sigma2 * deriv ;
	    for(int irow=0; irow<DIM; ++irow)
	      for(int icol=0; icol<DIM; ++icol)
		halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol)  / sigma2;
	  }
	  {
	    // derivatives of residual Y
	    deriv(0) = -sinphi ;
	    deriv(1) =  cosphi ;
	    deriv(2) = 0 ;
	    deriv(3) = 0 ;
	    deriv(4) = 0 ;
	    deriv(5) = 1 ;
	    deriv(6) = m.stack.x ;
	    deriv(7) = m.stack.y ;
	    halfdchi2dpar += residualY/sigma2* deriv ;
	    for(int irow=0; irow<DIM; ++irow)
	      for(int icol=0; icol<DIM; ++icol)
		halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol)  / sigma2;
	  }
	}
	
	text << "1st derivative: " << halfdchi2dpar << std::endl ;
	text << "2nd derivative: " << halfd2chi2dpar2 << std::endl ;
	//we need to build in something to 'protect' the problem if
	//parameters are unconstrained. If all measurements have the
	//same stack angle, then we cannot determine the stackX0,
	//stackY0 and stackphi0, so we need to leave them
	//out. Likewise, if all measurements have the same stack X
	//position, then we cannot determine thetaX, and if all have
	//the same stackY, we cannot determine thetaY. So, let's first
	//just test that. We need to add something about 'sufficiently
	//different', so I'll multiply with some number, then round.
	std::set<int> phivalues, xvalues, yvalues ;
	for( const auto& m : m_measurements) {
	  phivalues.insert( m.stack.phi*100) ;
	  xvalues.insert(m.stack.x*1) ;
	  yvalues.insert(m.stack.y*1) ;
	}
	std::cout << "xvalues: " << xvalues.size() << std::endl
		  << "yvalues: " << yvalues.size() << std::endl
		  << "phivalues: " << phivalues.size() << std::endl ;
	// now create the projection matrix.
	std::vector<bool> isactive(DIM,true) ;
	isactive[3] = isactive[6] = xvalues.size()>1 ;
	isactive[4] = isactive[7] = yvalues.size()>1 ;
	isactive[0] = isactive[1] = isactive[2] = isactive[5] = phivalues.size()>1 ;
	auto numactive = std::count_if(isactive.begin(),isactive.end(),[=](const auto& b) { return b; }) ;
	Eigen::Matrix<double,Eigen::Dynamic,DIM> projmatrix = Eigen::Matrix<double,Eigen::Dynamic,DIM>::Zero(numactive,DIM) ;
	int iactive{0} ;
	for(int i=0; i<DIM;++i) {
	  if( isactive[i] ) {
	    projmatrix(iactive,i) = 1.0 ;
	    ++iactive ;
	  }
	}
	text << "Projection matrix: " << projmatrix << std::endl ;
	Eigen::Matrix<double,Eigen::Dynamic,1> halfdchi2dparsub    = projmatrix * halfdchi2dpar ;
	text << "Sub first: " << std::endl << halfdchi2dparsub << std::endl ;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> halfd2chi2dpar2sub = projmatrix * halfd2chi2dpar2 * projmatrix.transpose() ;
	text << "Sub second: " << std::endl<< halfd2chi2dpar2sub << std::endl ;
	text << "determinant: " << halfd2chi2dpar2sub.determinant() << std::endl ;
	Eigen::Matrix<double,Eigen::Dynamic,1> deltasub = halfd2chi2dpar2sub.ldlt().solve(halfdchi2dparsub) ;
	text << "Sub delta: " << std::endl << deltasub << std::endl ;
	const VectorNd delta = projmatrix.transpose() * deltasub ;
	text << "Delta: " << delta << std::endl ;
	const double dchi2 = delta.dot(halfdchi2dpar) ;
	
	text << "deltachi2 (module factor 2): " << istep << " " << delta.dot(halfdchi2dpar) << std::endl ;
	for(int ipar = 0; ipar<DIM; ++ipar )
	  text << parnames[ipar] << " : " << pars[ipar] << " " << pars[ipar] - delta[ipar] << std::endl ;
	pars -= delta ;
	if(dchi2<0.01) break ;
      }
      text << "Updating stack calibration" << std::endl ;
      geomsvc->updateStackCalibration( pars(2), pars(3) , pars(4),
				       pars(5), pars(6) , pars(7) ,
				       geomsvc->stackPhi0() ) ;
    }
  }
  
  // // void StackCalibration::calibrateold()
  // // {
  // //   //std::stringstream text ;
  // //   auto& text = std::cout ;
  // //   text << "Number of measurements: " << m_measurements.size() << std::endl ;
  // //   // below we use transforms to go from one frame to another. but
  // //   // to compute the parameters we need to explicitly model the
  // //   // transforms, compute derivatives etc. in that process, we
  // //   // hopefully also find the mistake in the transforms.
    
  // //   // This is the transformation 'geomsvc::fromCameratoGlobal. We
  // //   // do not touch that in this calibration step. Let's assume it is right.
  // //   //  QTransform rc ;
  // //   //  MSMainCoordinates main = MotionSystemSvc::instance()->maincoordinates() ;
  // //   //  Coordinates2D offset = toGlobal( main ) ;
  // //   //  rc.rotateRadians( m_cameraPhi ) ;
  // //   //  rc.translate( offset.x(), offset.y() ) ;
    
  // //   // This computes the transform for the stack axis.
  // //   /*
  // //     Coordinates2D GeometrySvc::stackAxisInGlobal( const MSStackCoordinates& c ) const
  // //     {
  // //     return Coordinates2D{
  // //     m_stackX0 + m_stackXA * c.x + m_stackXB * c.y,
  // //     m_stackY0 + m_stackYA * c.x + m_stackYB * c.y,
  // //     m_stackPhi0 + c.phi
  // //     } ; 
  // //     }
  // //     QTransform GeometrySvc::fromStackToGlobal() const
  // //     {
  // //     Coordinates2D coord = stackAxisInGlobal() ;
  // //     QTransform rc ;
  // //     rc.translate( coord.x(), coord.y() ) ;
  // //     rc.rotateRadians( -coord.phi() ) ; // I do not know why I need the minus sign here :-(
  // //     return rc ;
  // //     }
      
  // //   */
    
  // //   // In the following we need to compute stackX{0,A,B},
  // //   // stackY{0,A,B} and stackPhi0. These are 7 parameters in
  // //   // total. However, we assume that the X and Y axis scale are well
  // //   // calibrated, so that we only need to calibrate the angles of
  // //   // both axis with the global system axes. That means that there
  // //   // are 5 parameters left.
    
  // //   auto geomsvc = GeometrySvc::instance() ;
  // //   if(m_measurements.size()>0) {
  // //     // for every measurement we
  // //     // - compute the global coordinates from MSMain coordinates
  // //     // - compute the prediction of this by using the transforms
  // //     // - compute the residuals to the transform parameters
           
  // //     // the position of the marker in the stack frame is also
  // //     // unknown: so these are two extra parameters. we need to
  // //     // initialize them with something, so we do that with the first
  // //     // measurement.
      
  // //     // compute the markerposition in the stack frame from the first measurement.
  // //     QPointF markerposstackframe ;
  // //     {
  // // 	const auto m0 =  m_measurements.front() ;
  // // 	const Coordinates2D markerglobalpos = geomsvc->toGlobal( m0.main ) ;
  // // 	const Coordinates2D stackaxis = geomsvc->stackAxisInGlobal( m0.stack ) ;
  // // 	const double dx = markerglobalpos.x() - stackaxis.x() ;
  // // 	const double dy = markerglobalpos.y() - stackaxis.y() ;
  // // 	// now just rotate in the stack frame: need to get the sign of the angle right:-)
  // // 	const double sinphi = std::sin(stackaxis.phi()) ;
  // // 	const double cosphi = std::cos(stackaxis.phi()) ;
  // // 	double dxprime = cosphi*dx - sinphi*dy ; 
  // // 	double dyprime = sinphi*dx + cosphi*dy ; 
  // // 	markerposstackframe = QPointF{dxprime,dyprime} ;
  // // 	text << "Approximate position of marker in stack frame: " << markerposstackframe.x() << "," << markerposstackframe.y() << std::endl ;
  // //     }
  // //     // the parameters are:
  // //     // 0: marker pos stack frame x
  // //     // 1: marker pos stack frame y
  // //     // 2: m_stackX0
  // //     // 3: m_stackY0
  // //     // 4. thetaX: m_stackXA = cos(thetaX); m_stackYA = sin(thetaX) 
  // //     // 5. thetaY: m_stackXB = sin(thetaY); m_stackYB = cos(thetaY) 
  // //     // 6: m_stackPhi0

  // //     std::vector<std::string> parnames = { "tag X", "tag Y", "stack X0", "stack Y0", "stack theta X", "stack theta Y", "stack phi0" } ;
      
  // //     using Vector7d = Eigen::Matrix<double,7,1> ;
  // //     using MatrixNd = Eigen::Matrix<double,7,7> ;
  // //     Vector7d pars ;
  // //     pars(0) = markerposstackframe.x() ;
  // //     pars(1) = markerposstackframe.y() ;
  // //     pars(2) = geomsvc->stackX0() ;
  // //     pars(3) = geomsvc->stackY0() ;
  // //     pars(4) = std::atan2(geomsvc->stackYA(),geomsvc->stackXA()) ;
  // //     pars(5) = std::atan2(geomsvc->stackXB(),geomsvc->stackYB()) ;
  // //     pars(6) = geomsvc->stackPhi0() ;
      
  // //     const int maxnsteps = 5 ; 
  // //     for( int istep=0; istep<maxnsteps; ++istep) {
	
  // // 	Vector7d halfdchi2dpar   = Vector7d::Zero() ;
  // // 	MatrixNd halfd2chi2dpar2 = MatrixNd::Zero() ;
  // // 	const double sigma2 = 0.005*0.005 ; // sigma = 5 micron
	
  // // 	for( const auto& m : m_measurements) {
  // // 	  // now we do the same, but with inverse transformations, compute residual, etc
  // // 	  Coordinates2D markerglobalposref = geomsvc->toGlobal( m.main ) ;
	  
  // // 	  //const Coordinates2D stackaxisorig = geomsvc->stackAxisInGlobal( m.stack ) ;
  // // 	  const Coordinates2D stackaxis = {
  // // 	    pars(2) + std::cos( pars(4) ) * m.stack.x + std::sin( pars(5) ) * m.stack.y,
  // // 	    pars(3) + std::sin( pars(4) ) * m.stack.x + std::cos( pars(5) ) * m.stack.y,
  // // 	    pars(6) + m.stack.phi } ;
  // // 	  // qDebug() << "stackaxis orig: "
  // // 	  // 	   << stackaxisorig.x() << stackaxisorig.y() <<  stackaxisorig.phi() ;
  // // 	  // qDebug() << "stackaxis new : "
  // // 	  // 	   << stackaxis.x() << stackaxis.y() <<  stackaxis.phi() ;
	  
	  
  // // 	  const double sinphi = std::sin(stackaxis.phi()) ;
  // // 	  const double cosphi = std::cos(stackaxis.phi()) ;
  // // 	  double x = stackaxis.x() + pars(0) * cosphi + pars(1) * sinphi ;
  // // 	  double y = stackaxis.y() - pars(0) * sinphi + pars(1) * cosphi ;
	  
  // // 	  double residualX = x - markerglobalposref.x() ;
  // // 	  double residualY = y - markerglobalposref.y() ;
  // // 	  //qDebug() << "Residual: " << residualX << residualY ;
	  
  // // 	  // x and y factorise, so just do them one by one
  // // 	  Vector7d deriv ;
  // // 	  {
  // // 	    // derivatives of residual X
  // // 	    deriv(0) = cosphi ;
  // // 	    deriv(1) = sinphi ;
  // // 	    deriv(2) = 1 ;
  // // 	    deriv(3) = 0 ;
  // // 	    deriv(4) = -1.0 * std::sin( pars(4) ) * m.stack.x ;
  // // 	    deriv(5) = +1.0 * std::cos( pars(5) ) * m.stack.y ;
  // // 	    deriv(6) = pars(0) * -1.0 * sinphi + pars(1) * +1.0 * cosphi ;
  // // 	    halfdchi2dpar += residualX/sigma2 * deriv ;
  // // 	    for(int irow=0; irow<7; ++irow)
  // // 	      for(int icol=0; icol<7; ++icol)
  // // 		halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol)  / sigma2;
  // // 	  }
  // // 	  {
  // // 	    // derivatives of residual Y
  // // 	    deriv(0) = -sinphi ;
  // // 	    deriv(1) =  cosphi ;
  // // 	    deriv(2) = 0 ;
  // // 	    deriv(3) = 1 ;
  // // 	    deriv(4) = +1.0 * std::cos( pars(4) ) * m.stack.x ;
  // // 	    deriv(5) = -1.0 * std::sin( pars(5) ) * m.stack.y ;
  // // 	    deriv(6) = pars(0) * -1.0 * cosphi + pars(1) * -1.0 * sinphi ;
  // // 	    halfdchi2dpar += residualY/sigma2* deriv ;
  // // 	    for(int irow=0; irow<7; ++irow)
  // // 	      for(int icol=0; icol<7; ++icol)
  // // 		halfd2chi2dpar2(irow,icol) += deriv(irow)*deriv(icol)  / sigma2;
  // // 	  }
  // // 	}
	
  // // 	text << "1st derivative: " << halfdchi2dpar << std::endl ;
  // // 	text << "2nd derivative: " << halfd2chi2dpar2 << std::endl ;

  // // 	text << "determinant: " << halfd2chi2dpar2.determinant() << std::endl ;

  // // 	//we need to build in something to 'protect' the problem if
  // // 	//parameters are unconstrained. If all measurements have the
  // // 	//same stack angle, then we cannot determine the stackX0,
  // // 	//stackY0 and stackphi0, so we need to leave them
  // // 	//out. Likewise, if all measurements have the same stack X
  // // 	//position, then we cannot determine thetaX, and if all have
  // // 	//the same stackY, we cannot determine thetaY. So, let's first
  // // 	//just test that. We need to add something about 'sufficiently
  // // 	//different', so I'll multiply with some number, then round.
  // // 	std::set<int> phivalues, xvalues, yvalues ;
  // // 	for( const auto& m : m_measurements) {
  // // 	  phivalues.insert( m.stack.phi*100) ;
  // // 	  xvalues.insert(m.stack.x*1) ;
  // // 	  yvalues.insert(m.stack.y*1) ;
  // // 	}
  // // 	std::cout << "xvalues: " << xvalues.size() << std::endl
  // // 		  << "yvalues: " << yvalues.size() << std::endl
  // // 		  << "phivalues: " << phivalues.size() << std::endl ;
  // // 	// now create the projection matrix.
  // // 	std::vector<bool> isactive(7,true) ;
  // // 	isactive[4] = xvalues.size()>1 ;
  // // 	isactive[5] = yvalues.size()>1 ;
  // // 	isactive[0] = isactive[1] = isactive[2] = isactive[3] = isactive[6] = phivalues.size()>1 ;
  // // 	isactive[6] = false ; // I realized that stackPhi0 is perhaps not a parameter in this problem :-(
  // // 	auto numactive = std::count_if(isactive.begin(),isactive.end(),[=](const auto& b) { return b; }) ;
  // // 	Eigen::Matrix<double,Eigen::Dynamic,7> projmatrix = Eigen::Matrix<double,Eigen::Dynamic,7>::Zero(numactive,7) ;
  // // 	int iactive{0} ;
  // // 	for(int i=0; i<7;++i) {
  // // 	  if( isactive[i] ) {
  // // 	    projmatrix(iactive,i) = 1.0 ;
  // // 	    ++iactive ;
  // // 	  }
  // // 	}
  // // 	text << "Projection matrix: " << projmatrix << std::endl ;

  // // 	Eigen::Matrix<double,Eigen::Dynamic,1> 
  // // 	  /*const auto*/ halfdchi2dparsub    = projmatrix * halfdchi2dpar ;
  // // 	text << "Sub first: " << std::endl << halfdchi2dparsub << std::endl ;
  // // 	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic>
  // // 	  /*const auto*/ halfd2chi2dpar2sub = projmatrix * halfd2chi2dpar2 * projmatrix.transpose() ;
  // // 	text << "Sub second: " << std::endl<< halfd2chi2dpar2sub << std::endl ;
  // // 	Eigen::Matrix<double,Eigen::Dynamic,1> 
  // // 	  /*const auto*/ deltasub = halfd2chi2dpar2sub.ldlt().solve(halfdchi2dparsub) ;
  // // 	text << "Sub delta: " << std::endl << deltasub << std::endl ;
  // // 	const Vector7d delta = projmatrix.transpose() * deltasub ;
  // // 	//Vector7d delta = halfd2chi2dpar2.ldlt().solve(halfdchi2dpar) ;
  // // 	text << "Delta: " << delta << std::endl ;
  // // 	const double dchi2 = delta.dot(halfdchi2dpar) ;
	
  // // 	text << "deltachi2 (module factor 2): " << istep << " " << delta.dot(halfdchi2dpar) << std::endl ;
  // // 	for(int ipar = 0; ipar<7; ++ipar )
  // // 	  text << parnames[ipar] << " : " << pars[ipar] << " " << pars[ipar] - delta[ipar] << std::endl ;
  // // 	pars -= delta ;
  // // 	if(dchi2<0.01) break ;
	
  // //     }
  // //     text << "Updating stack calibration" << std::endl ;
  // //     geomsvc->updateStackCalibration( pars(2), std::cos( pars(4) ), std::sin( pars(5) ),
  // // 				       pars(3), std::sin( pars(4) ), std::cos( pars(5) ),
  // // 				       pars(6) ) ;
  // //     m_measurements.clear() ;
  // //   }
  // //   //std::cout << text.str() << std::endl ;
  // // }

  void StackCalibration::initialize()
  {
    auto geomsvc = GeometrySvc::instance() ;
    m_pars(0) = 0 ;
    m_pars(1) = 1 ;
    m_pars(2) = geomsvc->stackX0() ;
    m_pars(3) = geomsvc->stackXA() ;
    m_pars(4) = geomsvc->stackXB() ;
    m_pars(5) = geomsvc->stackY0() ;
    m_pars(6) = geomsvc->stackYA() ;
    m_pars(7) = geomsvc->stackYB() ;
    m_pars(8) = geomsvc->stackPhi0() ;

    // we'll need to finetune the errors a bit
    m_cov = Eigen::Matrix<double,DIM,DIM>::Zero() ;
    m_cov(0,0) = m_cov(1,1) = 1 ;
    m_cov(2,2) = m_cov(5,5) = 1 ;
    m_cov(3,3) = m_cov(4,4) = m_cov(6,6) = m_cov(7,7) = 1e-4 ;
    m_cov(8,8) = 1e-2 ;
        
    updateTable() ;
  }
  
  void StackCalibration::update()
  {
    auto geomsvc = GeometrySvc::instance() ;
    geomsvc->updateStackCalibration( m_pars(2), m_pars(3), m_pars(4),
				     m_pars(5), m_pars(6), m_pars(7),
				     m_pars(8) ) ;
  }

  void StackCalibration::filter()
  {
    auto geomsvc = GeometrySvc::instance() ;
    const auto m = MotionSystemSvc::instance()->coordinates() ;
    Coordinates2D markerglobalposref = geomsvc->toGlobal( m.main ) ;
    const Coordinates2D stackaxisorig = geomsvc->stackAxisInGlobal( m.stack ) ;

    const Coordinates2D stackaxis = {
      m_pars(2) + m_pars(3) * m.stack.x + m_pars(4) * m.stack.y,
      m_pars(5) + m_pars(6) * m.stack.x + m_pars(7) * m.stack.y,
      m_pars(8) + m.stack.phi } ;

    qDebug() << "orig: " << stackaxisorig.x() << stackaxisorig.y() << stackaxisorig.phi() ;
    qDebug() << "new: " << stackaxis.x() << stackaxis.y() << stackaxis.phi() ;
    
    	  
    const double sinphi = std::sin(stackaxis.phi()) ;
    const double cosphi = std::cos(stackaxis.phi()) ;
    double x = stackaxis.x() + m_pars(0) * cosphi + m_pars(1) * sinphi ;
    double y = stackaxis.y() - m_pars(0) * sinphi + m_pars(1) * cosphi ;

    Eigen::Matrix<double,2,1> r ;
    r(0) = x - markerglobalposref.x() ;
    r(1) = y - markerglobalposref.y() ;
    qDebug() << "Residual: " << r(0) << r(1) ;

    Eigen::Matrix<double,2,9> H = Eigen::Matrix<double,2,9>::Zero() ;
    // derivatives of residual X
    H(0,0) = cosphi ;
    H(0,1) = sinphi ;
    H(0,2) = 1 ;
    H(0,3) = m.stack.x ;
    H(0,4) = m.stack.y ;
    H(0,5) = 0 ;
    H(0,6) = 0 ;
    H(0,7) = 0 ;
    H(0,8) = -sinphi*m_pars(0)+cosphi*m_pars(1) ;
    // derivatives of residual Y
    H(1,0) = -sinphi ;
    H(1,1) =  cosphi ;
    H(1,2) = 0 ;
    H(1,3) = 0 ;
    H(1,4) = 0 ;
    H(1,5) = 1 ;
    H(1,6) = m.stack.x ;
    H(1,7) = m.stack.y ;
    H(1,8) = -cosphi*m_pars(0)-sinphi*m_pars(1) ;
    // now do the K-filter
    Eigen::Matrix<double,2,2> V ;
    V << m_sigma2,0,0,m_sigma2 ;
    Eigen::Matrix<double,2,2> R = V + H * m_cov * H.transpose() ;
    Eigen::Matrix<double,9,2> K = m_cov * H.transpose() * R.inverse() ;

    //qDebug() << "Not even sure we have sign right in K-filter!" ;
    m_pars -= K * r ;
    m_cov  -= K * H * m_cov ;
    updateTable() ;
  }
  
  // determine the current global position of the camera in the stack frame
  void StackCalibration::tag()
  {
    auto geomsvc = GeometrySvc::instance() ;
    QTransform fromCameraToGlobal = geomsvc->fromCameraToGlobal() ;
    QTransform fromStackToGlobal = geomsvc->fromStackToGlobal() ;
    QTransform fromCameraToStack =  fromCameraToGlobal * fromStackToGlobal.inverted();
    
    //QPointF cameraInGlobal = QPointF(0,0) * fromCameraToGlobal ;
    QPointF cameraInStack  = QPointF(0,0) * fromCameraToStack ;
    qDebug() << "CameraInStack: " << cameraInStack ;
    m_taggedCameraInStack = cameraInStack ;

    // let's do this without the transforms
    {
      const auto m0 = MotionSystemSvc::instance()->coordinates() ;
      const Coordinates2D markerglobalpos = geomsvc->toGlobal( m0.main ) ;
      const Coordinates2D stackaxis = geomsvc->stackAxisInGlobal( m0.stack ) ;
      const double dx = markerglobalpos.x() - stackaxis.x() ;
      const double dy = markerglobalpos.y() - stackaxis.y() ;
      // now just rotate in the stack frame: need to get the sign of the angle right:-)
      const double sinphi = std::sin(stackaxis.phi()) ;
      const double cosphi = std::cos(stackaxis.phi()) ;
      double dxprime = cosphi*dx - sinphi*dy ; 
      double dyprime = sinphi*dx + cosphi*dy ; 
      qDebug() << "Position of marker in stack frame: " << dxprime << dyprime ;
      m_pars(0) = dxprime ;
      m_pars(1) = dyprime ;
      //m_cov(0,0) = m_cov(1,1) = m_sigma2 ;
      updateTable() ;
    }
  } ;
  
  // move the camera to the tagged position
  void StackCalibration::track() const
  {
    //
    auto geomsvc = GeometrySvc::instance() ;
    /*
    QTransform fromStackToGlobal = geomsvc->fromStackToGlobal() ;
    QPointF globalpos = m_taggedCameraInStack * fromStackToGlobal ;
    */    
    // now do this without the transforms
    const auto stack = MotionSystemSvc::instance()->stackcoordinates() ;
    const Coordinates2D stackaxis = {
      m_pars(2) + m_pars(3) * stack.x + m_pars(4) * stack.y,
      m_pars(5) + m_pars(6) * stack.x + m_pars(7) * stack.y,
      m_pars(8) + stack.phi } ;
    const double sinphi = std::sin(stackaxis.phi()) ;
    const double cosphi = std::cos(stackaxis.phi()) ;
    double globalX = stackaxis.x() + m_pars(0) * cosphi + m_pars(1) * sinphi ;
    double globalY = stackaxis.y() - m_pars(0) * sinphi + m_pars(1) * cosphi ;
    
    // we must find a better way to do this
    auto msmaincoordinates = geomsvc->toMSMain( QPointF{globalX,globalY} ) ;
    auto mssvc = MotionSystemSvc::instance() ;
    mssvc->mainXAxis().moveTo( msmaincoordinates.x ) ;
    mssvc->mainYAxis().moveTo( msmaincoordinates.y ) ;
    // Current status: this orks for x,y displacements, but rotation
    // is exactly in opposite direction!
  } ;
  
  // temporary, for debugging the fitting routine
  void StackCalibration::exportToFile() const
  {
    auto filename = QFileDialog::getSaveFileName(nullptr, tr("Save data"),
						 "stackcalibdata.dat",
						 tr("dat files (*.dat)"));
    QFile f( filename );
    f.open(QIODevice::WriteOnly) ;
    QTextStream data( &f );
    for( const auto& m : m_measurements ) {
      QStringList strList;
      strList << QString::number(m.main.x) << QString::number(m.main.y) << QString::number(m.stack.x) << QString::number(m.stack.y) << QString::number(m.stack.phi) ;
      data << strList.join(";") << "\n";
    }
    f.close() ;
    qDebug() << "Wrote to file: " << m_measurements.size() ;
  }

  void StackCalibration::importFromFile()
  {
    // pop up a dialog to get a file name
    auto filename = QFileDialog::getOpenFileName(nullptr, tr("Read data"),
						 "stackcalibdata.dat",
						 tr("dat files (*.dat)"));
    m_measurements.clear();
    QFile f( filename );
    if( f.open(QIODevice::ReadOnly) ) {
      while (!f.atEnd()){
	QString line = f.readLine();
	QStringList columns = line.split(";") ;
	columns = columns.replaceInStrings(" ","") ;
	columns = columns.replaceInStrings("\"","") ;
	columns = columns.replaceInStrings("\n","") ;
	if( columns.size()==5 ) {
	  MSCoordinates m ;
	  m.main.x = columns.at(0).toDouble() ;
	  m.main.y = columns.at(1).toDouble() ;
	  m.stack.x = columns.at(2).toDouble() ;
	  m.stack.y = columns.at(3).toDouble() ;
	  m.stack.phi = columns.at(4).toDouble() ;
	  m_measurements.push_back( m ) ;
	}
      }
    }
    f.close();
    qDebug() << "Read from file: " << m_measurements.size() ;
  }
}

