#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QMainWindow>
#include "customslider.h"
#include "jpeg.h"
#include "keyframe.h"
namespace Ui {
class MediaPlayer;
}

class MediaPlayer : public QMainWindow
{
    Q_OBJECT

public:
    explicit MediaPlayer(QWidget *parent = 0);
    ~MediaPlayer();
    void pngTomp4_2();
private slots:
    void on_pushButton_Volume_clicked();

    void on_pushButton_Open_clicked();

    void on_pushButton_Player_clicked();

    void on_pushButton_Jpeg_clicked();
    //自定义槽函数
    void horizontalSlider_clicked();

    void horizontalSlider_moved();

    void horizontalSlider_released();

    void slider_Volume_Changed();

    void onTimerOut();


    void on_pushButton_key_clicked();

private:
    Ui::MediaPlayer *ui;
    CustomSlider *slider_Volume;
    JPEG jpeg;
    KeyFrame keyframe;
    QString filename, outFilename;

};

#endif // MEDIAPLAYER_H
