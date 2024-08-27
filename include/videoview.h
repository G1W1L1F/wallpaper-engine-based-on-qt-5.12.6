#pragma once

#include <QWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QVideoWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QEvent>

class SliderFilter; // Forward declaration

class VideoView : public QWidget
{
    Q_OBJECT

public:
    explicit VideoView(QWidget *parent = nullptr);
    ~VideoView();

    void getvideopath(const QString& filePath); // 获取视频路径
    void setSliderStyle(QSlider *slider); // 设置滑块样式
    void setWindowStyle(); // 设置窗口样式
    void setupLayout(); // 设置总体布局
    void adjustBottomContainerPosition(); // 调整控件窗口位置，使得能够跟随视频窗口的移动
    void toggleSpeedComboBox(); // 切换 speedComboBox 的显示和隐藏
    void toggleVolumeSlider();// 切换 VolumeSlider 的显示和隐藏

protected:
    void paintEvent(QPaintEvent* e) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private slots:
    void on_play_pause_Btn_clicked(); // 播放按键
    void on_restart_Btn_clicked(); // 重新开始按键
    void on_back10s_Btn_clicked(); // 快进10s按键
    void on_forward10s_Btn_clicked(); // 后退10s按键
    void updateDuration(qint64 duration); // 更新视频总时间
    void updatePosition(qint64 position); // 更新视频当前时间
    void updatePlaybackRate(); // 设置播放速率
    void hideBottomContainer();
    void showBottomContainer();

private:
    void startHidingTimer(); // 开始隐藏控件栏的计时器
    void stopHidingTimer();

    QMediaPlayer* pPlayer;
    QMediaPlaylist* pPlayerList;
    QVideoWidget* pVideoWidget;
    QSlider* volumeSlider; // 音量条
    QSlider* progressSlider; // 进度条
    QLabel* currentLabel; // 当前时间显示
    QLabel* totalLabel; // 总时间显示
    QPushButton* volume_Btn;
    QPushButton* play_pause_Btn;
    QPushButton* restart_Btn;
    QPushButton* back10s_Btn;
    QPushButton* forward10s_Btn;
    QPushButton* showspeed_Btn;
    QComboBox* speedComboBox; // 速率下拉菜单
    QWidget* bottomContainer; // 底部控件容器
    QWidget* videoContainer; // 视频容器
    QWidget* volumeContainer;
    QString videoPath;
    QTimer* mouseInactivityTimer; // 鼠标不活动计时器，用来触发startHidingTimer
    QTimer* hideTimer;

    bool bottomContainerVisible;
};
