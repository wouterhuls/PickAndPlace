#include "VideoRecorder.h"
#include <QMediaRecorder>
#include <QPushButton>
#include <QHBoxLayout>
#include <QCamera>
#include <QMessageBox>
#include <QUrl>
#include <QTimer>

namespace PAP
{
  
  VideoRecorder::VideoRecorder(QWidget* parent)
    : QWidget{parent}
  {
    auto layout = new QHBoxLayout{} ;
    this->setLayout( layout) ;
    
    m_recordButton = new QPushButton{"Record",this} ;
    connect( m_recordButton,  &QPushButton::clicked, [=](){ this->record() ; } ) ;
    layout->addWidget( m_recordButton ) ;
      
    m_stopButton = new QPushButton{"Stop",this} ;
    connect( m_stopButton,  &QPushButton::clicked, [=](){ this->stop() ; } ) ;
    layout->addWidget( m_stopButton ) ;

    // make sure that the record button if flashing when we are recording
    auto defaultcolor =  m_recordButton->palette().color(QPalette::Button) ;
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [=]() {
	auto color = !m_recordButton->isEnabled() && (++m_flashstate % 2) ? QColor(Qt::red) : defaultcolor ;
	auto palette = m_recordButton->palette() ;
	palette.setColor(QPalette::Button,color) ;
	m_recordButton->setPalette(palette) ;
      } ) ;
    timer->start(1000);
  }
  
  void VideoRecorder::setCamera(QCamera* camera)
  {
    if( camera ) {
      if( m_recorder ) delete m_recorder ;
      qDebug() << "In setCamera" ;
      
      m_recorder = new QMediaRecorder(camera);
      m_recorder->setMuted(true) ;
      connect(m_recorder, SIGNAL(stateChanged(QMediaRecorder::State)), this, SLOT(updateRecorderState(QMediaRecorder::State)));
      
      //connect(m_recorder, SIGNAL(durationChanged(qint64)), this, SLOT(updateRecordTime()));
      connect(m_recorder, SIGNAL(error(QMediaRecorder::Error)), this, SLOT(displayRecorderError()));
      
      //m_recorder->setMetaData(QMediaMetaData::Title, QVariant(QLatin1String("Test Title")));
      updateRecorderState(m_recorder->state());
      
      //m_recorder->setOutputLocation(QUrl{"movie.mp4"}) ;
      QVideoEncoderSettings settings = m_recorder->videoSettings();
      qDebug() << "Framerate: " << settings.frameRate() ;
      qDebug() << "Quality: " << settings.quality() ;
      qDebug() << "Resolution: " << settings.resolution() ;
      qDebug() << "Codecs: " << m_recorder->supportedVideoCodecs();
    }
  }
  
  void VideoRecorder::record()
  {
    m_recorder->record();
    updateRecordTime();
    qDebug()<< m_recorder->supportedVideoCodecs();
  }
  
  void VideoRecorder::stop()
  {
    m_recorder->stop();
  }
  
  // void VideoRecorder::setMuted(bool muted)
  // {
  //   m_recorder->setMuted(muted);
  // }
  
  
  void VideoRecorder::updateRecorderState(QMediaRecorder::State state)
  {
    switch (state) {
    case QMediaRecorder::StoppedState:
      m_recordButton->setEnabled(true);
      //m_pauseButton->setEnabled(true);
      m_stopButton->setEnabled(false);
      break;
    case QMediaRecorder::PausedState:
      m_recordButton->setEnabled(true);
      //m_pauseButton->setEnabled(false);
      m_stopButton->setEnabled(true);
      break;
    case QMediaRecorder::RecordingState:
      m_recordButton->setEnabled(false);
      //m_pauseButton->setEnabled(true);
      m_stopButton->setEnabled(true);
      break;
    }
  }
  
  void VideoRecorder::displayRecorderError()
  {
    QMessageBox::warning(this, tr("Capture error"), m_recorder->errorString());
  }
  
  void VideoRecorder::updateRecordTime()
  {
    QString str = QString("Recorded %1 sec").arg(m_recorder->duration()/1000);
    //statusBar()->showMessage(str);
  }
}
