#ifndef MOVEMENTSERIES_H
#define MOVEMENTSERIES_H

#include <QWidget>
#include "CameraWindow.h"
#include "MetrologyReport.h"

// Let's implement the list of measurements as a qabstracttablemodel
// http://doc.qt.io/qt-5/qabstracttablemodel.html#details
// https://stackoverflow.com/questions/11906324/binding-model-to-qt-tableview

class QTableWidget ;
class QHBoxLayout ;
class QPlainTextEdit ;

namespace PAP
{
  class CameraWindow ;
  class MovementSeries : public QWidget
  {
    Q_OBJECT
  public:
    using Coordinate = PAP::ReportCoordinate ;
  private:
    CameraWindow* m_camerasvc{0} ;
    std::vector<Coordinate> m_coordinates ;
    int m_currentcoordinate{0} ;
    std::vector<QMetaObject::Connection> m_conns ;
    QTableWidget* m_table{0} ;
    ViewDirection m_viewdir{ViewDirection::NumViews} ;
    QHBoxLayout* m_buttonlayout{0} ;
    QPlainTextEdit* m_textbox{0} ;
  signals:
    void executeReady() ;
    void movementReady() ;
    void finalizeReady() ; // emitted after the last movement ready
    void initializeReady() ;
  protected:
    void fillTable() ;
  public:
    MovementSeries(CameraWindow& camerasvc, ViewDirection viewdir) ;
    virtual ~MovementSeries() {}
    // accessors
    const std::vector<ReportCoordinate>& coordinates() const { return  m_coordinates ; }
    std::vector<ReportCoordinate>& coordinates() { return m_coordinates; }
    int currentcoordinate() const { return m_currentcoordinate ; }
    CameraWindow* camerasvc() const { return m_camerasvc ; } //static_cast<CameraWindow*>(QWidget::parent()) ; }
    QHBoxLayout* buttonlayout() { return m_buttonlayout ; }
    CameraView* cameraview() { return camerasvc()->cameraview() ; }
    ViewDirection viewdirection() const { return m_viewdir ; }
    
    void disconnectsignals() ;
    void connectsignals() ;
    void addCoordinate() ;
    void setCoordinates( const std::vector<Coordinate>& newpoints ) ;
    void updateTableRow( int row, const ReportCoordinate& coord ) ;
    void activateRow( int row ) ;
    void record(const CoordinateMeasurement& measurement) ;
    void recordCentre() ;
    ViewDirection viewdir() const { return m_viewdir ; }
    void exportToFile() const ;
    void importFromFile() ;
    void importFromFile(const QString& filename) ;
    void reset() ;
    QString defaultFileName() const ;
    void focus() const ;
    void move() const ;
    virtual QString pageName() const { return QString{"unknown"} ; }
    virtual void initialize() {}   // modify this function to do stuff at the start of the movement series
    virtual void execute() { recordCentre() ; emit executeReady() ; }
    virtual void finalize() { emit finalizeReady() ; } // modify this function to do stuff at the end of the movement series
    virtual void initializeCoordinates() {} ;
  } ;
}

#endif
