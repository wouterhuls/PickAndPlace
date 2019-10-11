#ifndef __VIDEORECORDER_HH__
#define __VIDEORECORDER_HH__

#include <QWidget>
#include <QMediaRecorder>

class QMediaRecorder ;
class QPushButton ;
class QCamera ;

namespace PAP
{
  class VideoRecorder : public QWidget // this should become a page
  {

    Q_OBJECT
  public:
    VideoRecorder(QWidget* parent );
  public:
    void record();
    void stop();
    void setCamera(QCamera* camera) ;
  public slots:
    void updateRecorderState(QMediaRecorder::State state) ;
    void displayRecorderError() ;
    void updateRecordTime() ;
  private:
    QMediaRecorder* m_recorder{nullptr} ;
    QPushButton* m_recordButton{nullptr} ;
    QPushButton* m_stopButton{nullptr} ;
    unsigned char m_flashstate{0} ;
  } ;

} ;

#endif
