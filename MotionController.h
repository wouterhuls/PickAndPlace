#ifndef MOTIONCONTROLLER_H
#define MOTIONCONTROLLER_H

//#include "MotionAxis.h"
#include <QString>
#include <QObject>

namespace PAP
{
  class MotionAxis ;
  
  class MotionController : public QObject
  {
    Q_OBJECT
  public:
    MotionController( const int& id, const std::string& name ) ;
    virtual ~MotionController() {}
    void switchMotorsOn(bool on = true) const ;
    bool hasMotorsOn() const ;
    const std::string name() const { return m_name ; }
    int id() const { return m_id ; }
    const unsigned int& status() const { return m_status ; }
    const QString& error() const { return m_error; }
    void setStatus( unsigned int status ) ;
    void setError( const QString& error ) ;
    void addAxis( MotionAxis* a) { m_axes.push_back(a); }
  signals:
    void statusChanged() ;
  private:
    int m_id ;
    std::string m_name ;
    unsigned int m_status ;
    QString m_error ;
    std::vector<MotionAxis*> m_axes ;
  };
}

#endif // MOTIONAXIS_H
