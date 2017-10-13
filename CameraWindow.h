#ifndef WH_CAMERAWINDOW_H
#define WH_CAMERAWINDOW_H

//namespace Ui {
//    class CameraWindow;
//}

class QGraphicsScene ;
class QCamera ;

class CameraWindow : public QWidget
{
    Q_OBJECT

 public:
    explicit CameraWindow(QWidget *parent = 0);
    ~CameraWindow();
        
 private:
    //Ui::MainWindow *ui;
    QGraphicsScene* m_imagedisplay ; // a graphics scene that
				     // displayes the image and a
				     // cross. if I understand it
				     // well, then it can also tell
				     // where the cursor is and do
				     // things like zoom.

    QCamera* m_camera ;
};

#endif // MAINWINDOW_H
