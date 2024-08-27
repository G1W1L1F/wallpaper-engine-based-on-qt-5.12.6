#include "include/videoview.h"
#include "include/sliderfilter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrl>
#include <QDesktopWidget>
#include <QWindow>


void VideoView::setWindowStyle()
{
    QString styleSheet = R"(
                         VideoView {
                         background-color: black; /* 纯黑背景 */
                         border: 20px solid black; /* 黑色边缘 */
                         }
                         QVideoWidget {
                         background-color: black; /* 确保视频播放区域背景为黑色 */
                         }
                         )";

    this->setStyleSheet(styleSheet);
    pVideoWidget->setStyleSheet("background-color: black;");
}
void VideoView::setSliderStyle(QSlider *slider)
{
    QString styleSheet = R"(
                         QSlider::groove:horizontal {
                         border: 1px solid #999; /* 滑槽边框颜色 */
                         height: 5px; /* 滑槽高度 */
                         background: #ddd; /* 滑槽背景颜色 */
                         }
                         QSlider::handle:horizontal {
                         background: #FFFFFF; /* 滑块背景颜色 */
                         border: 2px solid black; /* 滑块边框颜色 */
                         width: 20px; /* 滑块宽度 */
                         height: 20px; /* 滑块高度 */
                         margin: -7px 0; /* 确保滑块在滑槽中心 */
                         border-radius: 10px; /* 滑块圆角半径 */
                         }
                         QSlider::handle:horizontal:pressed {
                         background: #FFFFFF; /* 滑块被按下时的颜色 */
                         border: 2px solid black; /* 滑块被按下时的边框颜色 */
                         }
                         QSlider::groove:vertical {
                         border: 1px solid #999; /* 滑槽边框颜色 */
                         width: 5px; /* 滑槽宽度 */
                         background: #ddd; /* 滑槽背景颜色 */
                         }
                         QSlider::handle:vertical {
                         background: #FFFFFF; /* 滑块背景颜色 */
                         border: 2px solid black; /* 滑块边框颜色 */
                         width: 20px; /* 滑块宽度 */
                         height: 20px; /* 滑块高度 */
                         margin: 0 -7px; /* 确保滑块在滑槽中心 */
                         border-radius: 10px; /* 滑块圆角半径 */
                         }
                         QSlider::handle:vertical:pressed {
                         background: #FFFFFF; /* 滑块被按下时的颜色 */
                         border: 2px solid black; /* 滑块被按下时的边框颜色 */
                         }
                         )";

    slider->setStyleSheet(styleSheet);
}


VideoView::VideoView(QWidget *parent) :
    QWidget(parent),
    pPlayer(new QMediaPlayer(this)),
    pPlayerList(new QMediaPlaylist(this)),
    pVideoWidget(new QVideoWidget(this)),
    volumeSlider(new QSlider(Qt::Vertical)),
    progressSlider(new QSlider(Qt::Horizontal)),
    currentLabel(new QLabel("00:00:00")),
    totalLabel(new QLabel("00:00:00")),
    volume_Btn(new QPushButton("")),
    play_pause_Btn(new QPushButton("")),
    restart_Btn(new QPushButton("")),
    back10s_Btn(new QPushButton("")),
    forward10s_Btn(new QPushButton("")),
    showspeed_Btn(new QPushButton("")),
    speedComboBox(new QComboBox()),

    videoContainer(new QWidget(this, Qt::FramelessWindowHint)),
    bottomContainer(new QWidget(this, Qt::FramelessWindowHint | Qt::Window)), // 设置控件容器为顶部窗口
    hideTimer(new QTimer(this)),
    mouseInactivityTimer(new QTimer(this)),
    bottomContainerVisible(true)
{
    setWindowTitle("查看视频");

    pPlayerList->setPlaybackMode(QMediaPlaylist::Loop);
    pPlayer->setPlaylist(pPlayerList);
    pPlayer->setVideoOutput(pVideoWidget);
    pPlayer->setVolume(50);

    bottomContainer->setStyleSheet("background-color: rgba(0, 0, 0, 255);");
    setupLayout();
    // 设置 bottomContainer 的透明度
    QWindow* windowHandle_bottom = bottomContainer->windowHandle();
    if (windowHandle_bottom) {
        windowHandle_bottom->setOpacity(0.5); // 设置透明度为50%
    }


    connect(volumeSlider, &QSlider::sliderMoved, pPlayer, &QMediaPlayer::setVolume);
    connect(pPlayer, &QMediaPlayer::durationChanged, this, &VideoView::updateDuration);
    connect(pPlayer, &QMediaPlayer::positionChanged, this, &VideoView::updatePosition);
    connect(progressSlider, &QSlider::sliderReleased, this, [=]() {
        pPlayer->setPosition(progressSlider->value());
    });

    SliderFilter *filter_process = new SliderFilter(progressSlider, this);
    connect(filter_process, &SliderFilter::sliderClicked, pPlayer, &QMediaPlayer::setPosition);

    SliderFilter *filter_volume = new SliderFilter(volumeSlider, this);
    connect(filter_volume, &SliderFilter::sliderClicked, pPlayer, &QMediaPlayer::setVolume);

    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);

    setSliderStyle(volumeSlider);
    setSliderStyle(progressSlider);

    speedComboBox->addItem("0.75x", 0.75);
    speedComboBox->addItem("1.0x", 1.0);
    speedComboBox->addItem("1.5x", 1.5);
    speedComboBox->addItem("2.0x", 2.0);
    speedComboBox->setCurrentIndex(1);

    connect(speedComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VideoView::updatePlaybackRate);
    connect(play_pause_Btn, &QPushButton::clicked, this, &VideoView::on_play_pause_Btn_clicked);
    connect(restart_Btn, &QPushButton::clicked, this, &VideoView::on_restart_Btn_clicked);
    connect(back10s_Btn, &QPushButton::clicked, this, &VideoView::on_back10s_Btn_clicked);
    connect(forward10s_Btn, &QPushButton::clicked, this, &VideoView::on_forward10s_Btn_clicked);

    hideTimer->setInterval(1000); // 隐藏控件容器计时器
    connect(hideTimer, &QTimer::timeout, this, &VideoView::hideBottomContainer);

    mouseInactivityTimer->setInterval(2000); // 设置鼠标不活动超时时间为2秒
    connect(mouseInactivityTimer, &QTimer::timeout, this, &VideoView::startHidingTimer); // 连接信号与槽

    pVideoWidget->setMouseTracking(true);
    bottomContainer->setMouseTracking(true);
    this->setMouseTracking(true);

    this->installEventFilter(this);
    pVideoWidget->installEventFilter(this);
    bottomContainer->installEventFilter(this);
}

VideoView::~VideoView()
{
    if (pPlayer->state() == QMediaPlayer::PlayingState) {
        pPlayer->stop();
    }
}

void VideoView::getvideopath(const QString& filePath)
{
    videoPath = filePath;
    pPlayerList->clear();
    QUrl url(videoPath);
    pPlayerList->addMedia(url);
    pVideoWidget->resize(size());
}

void VideoView::on_play_pause_Btn_clicked()
{
    QIcon playIcon(":/resource/my_button_icon/play.png");
    QIcon pauseIcon(":/resource/my_button_icon/pause.png");
    if (pPlayerList->isEmpty()) return;

    if (pPlayer->state() == QMediaPlayer::PlayingState) {
        pPlayer->pause();
        play_pause_Btn->setIcon(playIcon);
        play_pause_Btn->setIconSize(QSize(50, 50));
        play_pause_Btn->setFixedSize(60, 60);
    } else {
        pPlayer->play();
        play_pause_Btn->setIcon(pauseIcon);
        play_pause_Btn->setIconSize(QSize(50, 50));
        play_pause_Btn->setFixedSize(60, 60);
    }
}

void VideoView::on_restart_Btn_clicked()
{
    pPlayer->setPosition(0);
    pPlayer->play();
}

void VideoView::paintEvent(QPaintEvent* e)
{
    QSize newSize = size();
    if (pVideoWidget->size() != newSize) {
        pVideoWidget->resize(newSize);
    }
    if (bottomContainer->size() != QSize(newSize.width(), bottomContainer->height())) {
        bottomContainer->resize(newSize.width(), bottomContainer->height());
    }
    QWidget::paintEvent(e);
}

void VideoView::on_back10s_Btn_clicked()
{
    if (pPlayer) {
        qint64 newPosition = pPlayer->position() - 10000;
        pPlayer->setPosition(newPosition < 0 ? 0 : newPosition);
    }
}

void VideoView::on_forward10s_Btn_clicked()
{
    if (pPlayer) {
        qint64 newPosition = pPlayer->position() + 10000;
        pPlayer->setPosition(newPosition > pPlayer->duration() ? pPlayer->duration() : newPosition);
    }
}

void VideoView::toggleSpeedComboBox()
{
    bool isVisible = speedComboBox->isVisible();
    speedComboBox->setVisible(!isVisible);
}

void VideoView::toggleVolumeSlider()
{
    bool isVisible = volumeSlider->isVisible();
    volumeSlider->setVisible(!isVisible);
}


void VideoView::closeEvent(QCloseEvent *event)
{
    if (pPlayer->state() == QMediaPlayer::PlayingState) {
        pPlayer->stop();
        disconnect(pPlayer, nullptr, nullptr, nullptr);
    }
    QWidget::closeEvent(event);
}

void VideoView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    adjustBottomContainerPosition();
}

void VideoView::updateDuration(qint64 duration)
{
    totalLabel->setText(QString("%1:%2:%3").arg(duration / 3600000, 2, 10, QChar('0')).arg((duration / 60000) % 60, 2, 10, QChar('0')).arg((duration / 1000) % 60, 2, 10, QChar('0')));
    progressSlider->setRange(0, duration);
}

void VideoView::updatePosition(qint64 position)
{
    currentLabel->setText(QString("%1:%2:%3").arg(position / 3600000, 2, 10, QChar('0')).arg((position / 60000) % 60, 2, 10, QChar('0')).arg((position / 1000) % 60, 2, 10, QChar('0')));
    progressSlider->setValue(position);
}

void VideoView::updatePlaybackRate()
{
    double speed = speedComboBox->currentData().toDouble();
    pPlayer->setPlaybackRate(speed);
}

void VideoView::startHidingTimer()
{
    hideTimer->start();
}

void VideoView::stopHidingTimer()
{
    hideTimer->stop();
    if (!bottomContainerVisible) {
        showBottomContainer();
    }
}

void VideoView::hideBottomContainer()
{
    if (bottomContainerVisible) {
        bottomContainer->hide();
        bottomContainerVisible = false;
    }
}

void VideoView::showBottomContainer()
{
    if (!bottomContainerVisible) {
        bottomContainer->show();
        bottomContainerVisible = true;
    }
}

bool VideoView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this || obj == bottomContainer || obj == pVideoWidget) {
        if (event->type() == QEvent::MouseMove ||
                event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseButtonRelease ||
                event->type() == QEvent::MouseButtonDblClick)
        {
            stopHidingTimer();
            if (obj == pVideoWidget)
            {
                this->volumeSlider->hide();
                this->speedComboBox->hide();
            }
            mouseInactivityTimer->stop();
            mouseInactivityTimer->start();  // 重新启动定时器
        }
    }


    return QWidget::eventFilter(obj, event);
}

void VideoView::adjustBottomContainerPosition() {
    if (videoContainer && bottomContainer) {
        // 获取 videoContainer 在屏幕上的位置
        QPoint globalPos = videoContainer->mapToGlobal(videoContainer->rect().bottomLeft());

        // 计算 bottomContainer 的新位置
        int x = globalPos.x();
        int y = globalPos.y() - bottomContainer->height();

        // 移动 bottomContainer
        bottomContainer->move(x, y);
    }
}

void VideoView::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    adjustBottomContainerPosition();
}

void VideoView::setupLayout()
{
    // 设置 QLabel 样式表，使字体颜色为白色
    QString labelStyle = R"(
                         QLabel {
                         color: white;
                         }
                         )";
    currentLabel->setStyleSheet(labelStyle);
    totalLabel->setStyleSheet(labelStyle);

    // 设置按钮图标
    QIcon volumeIcon(":/resource/my_button_icon/volume.png");
    QIcon restartIcon(":/resource/my_button_icon/restart.png");
    QIcon back10sIcon(":/resource/my_button_icon/backward_ten.png");
    QIcon playIcon(":/resource/my_button_icon/play.png");
    QIcon forward10sIcon(":/resource/my_button_icon/forward_ten.png");
    QIcon showspeedIcon(":/resource/my_button_icon/speed.png");

    volume_Btn->setIcon(volumeIcon);
    volume_Btn->setIconSize(QSize(30, 30));
    volume_Btn->setFixedSize(40, 40);

    restart_Btn->setIcon(restartIcon);
    restart_Btn->setIconSize(QSize(30, 30));
    restart_Btn->setFixedSize(40, 40);

    back10s_Btn->setIcon(back10sIcon);
    back10s_Btn->setIconSize(QSize(30, 30));
    back10s_Btn->setFixedSize(40, 40);

    play_pause_Btn->setIcon(playIcon);
    play_pause_Btn->setIconSize(QSize(50, 50));
    play_pause_Btn->setFixedSize(60, 60);

    forward10s_Btn->setIcon(forward10sIcon);
    forward10s_Btn->setIconSize(QSize(30, 30));
    forward10s_Btn->setFixedSize(40, 40);

    showspeed_Btn->setIcon(showspeedIcon);
    showspeed_Btn->setIconSize(QSize(30, 30));
    showspeed_Btn->setFixedSize(40, 40);

    // 设置按钮样式表
    QString buttonStyle = R"(
                          QPushButton {
                          background-color: #444444;
                          border: none;
                          border-radius: 5px;
                          }
                          QPushButton:hover {
                          background-color: #CCCCCC;
                          }
                          QPushButton:pressed {
                          background-color: #AAAAAA;
                          filter: brightness(0.7);
                          }
                          )";
    volume_Btn->setStyleSheet(buttonStyle);
    restart_Btn->setStyleSheet(buttonStyle);
    back10s_Btn->setStyleSheet(buttonStyle);
    play_pause_Btn->setStyleSheet(buttonStyle);
    forward10s_Btn->setStyleSheet(buttonStyle);
    showspeed_Btn->setStyleSheet(buttonStyle);

    // 设置 combobox 样式表
    QString comboBoxStyle = R"(
                            QComboBox {
                            background-color: #444444;
                            border: 1px solid #666666;
                            border-radius: 5px;
                            padding: 5px;
                            color: white;
                            }
                            QComboBox::drop-down {
                            subcontrol-origin: padding;
                            subcontrol-position: top right;
                            width: 15px;
                            border-left-width: 1px;
                            border-left-color: #666666;
                            border-left-style: solid;
                            }
                            QComboBox QAbstractItemView {
                            background-color: #444444;
                            border: 1px solid #666666;
                            selection-background-color: #666666;
                            selection-color: white;
                            }
                            )";
    speedComboBox->setFixedWidth(80);
    speedComboBox->setStyleSheet(comboBoxStyle);

    // 连接按钮事件
    connect(showspeed_Btn, &QPushButton::clicked, this, &VideoView::toggleSpeedComboBox);
    connect(volume_Btn, &QPushButton::clicked, this, &VideoView::toggleVolumeSlider);

    // 创建 speedComboBox 的父控件并设置布局
    //    QWidget *speedWidget = new QWidget();
    //    QHBoxLayout *speedLayout = new QHBoxLayout(speedWidget);
    //    speedLayout->setContentsMargins(0, 0, 0, 0);
    //    speedLayout->setSpacing(0);


    //    speedLayout->addWidget(showspeed_Btn);  // 添加 showspeed_Btn
    //    speedLayout->addWidget(speedComboBox);  // 添加 speedComboBox
    speedComboBox->setVisible(false);
    QSizePolicy sp_retain = speedComboBox->sizePolicy(); // 设置隐藏控件不改变原有控件布局
    sp_retain.setRetainSizeWhenHidden(true);
    speedComboBox->setSizePolicy(sp_retain);

    // 创建一个隐藏的 QComboBox 复制体
    QComboBox *hiddenComboBox = new QComboBox(speedComboBox);
    hiddenComboBox->setVisible(false);
    QSizePolicy hiddenSp = hiddenComboBox->sizePolicy();
    hiddenSp.setRetainSizeWhenHidden(true);
    hiddenComboBox->setSizePolicy(hiddenSp);

    // 设置视频容器的布局
    QVBoxLayout *videoLayout = new QVBoxLayout(videoContainer);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    videoLayout->addWidget(pVideoWidget);
    pVideoWidget->setMinimumSize(640, 360);
    pVideoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置进度条布局
    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->addWidget(currentLabel);
    progressLayout->addWidget(progressSlider, 1);
    progressLayout->addWidget(totalLabel);

    // 设置控制按钮布局
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(10);

    // 添加按钮组
    controlsLayout->addWidget(hiddenComboBox);
    controlsLayout->addWidget(restart_Btn);
    controlsLayout->addWidget(back10s_Btn);
    controlsLayout->addWidget(play_pause_Btn); // 确保这个按钮居中
    controlsLayout->addWidget(forward10s_Btn);
    controlsLayout->addWidget(showspeed_Btn);
    controlsLayout->addWidget(speedComboBox);

    // 创建一个不可见的 QSpacerItem 以使 controlsLayout 居中
    QSpacerItem *leftSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QSpacerItem *rightSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);


    // 创建 volumeWidget 并设置布局
    QHBoxLayout *volumeLayout = new QHBoxLayout();
    volumeLayout->setContentsMargins(0, 0, 0, 0);
    volumeLayout->setSpacing(0); // 设置为0以确保按钮和滑块紧密排列

    volumeLayout->addWidget(volume_Btn); // 添加音量按钮
    volumeLayout->addWidget(volumeSlider); // 添加音量滑块

    volumeSlider->setVisible(false);
    QSizePolicy sp_retain_vs = volumeSlider->sizePolicy(); // 设置隐藏控件不改变原有控件布局
    sp_retain_vs.setRetainSizeWhenHidden(true);
    volumeSlider->setSizePolicy(sp_retain_vs);

    QWidget *volumeWidget = new QWidget();
    volumeWidget->setLayout(volumeLayout);
    volumeWidget->setFixedWidth(100); // 调整为适当宽度

    // 创建新的水平布局，将 leftSpacer、controlsLayout 和 volumeWidget 放在一起
    QHBoxLayout *controlsAndVolumeLayout = new QHBoxLayout();
    controlsAndVolumeLayout->setContentsMargins(0, 0, 0, 0);
    controlsAndVolumeLayout->setSpacing(0);
    controlsAndVolumeLayout->addItem(rightSpacer); // 添加不可见的 QSpacerItem 使 controlsLayout 居中
    controlsAndVolumeLayout->addLayout(controlsLayout); // 添加按钮组布局
    controlsAndVolumeLayout->addItem(leftSpacer); // 添加不可见的 QSpacerItem 使 controlsLayout 居中
    controlsAndVolumeLayout->addWidget(volumeWidget);   // 添加音量按钮和滑块


    // 设置进度条和控制按钮布局的垂直排列
    QVBoxLayout *progressAndControlsLayout = new QVBoxLayout();
    progressAndControlsLayout->setContentsMargins(0, 0, 0, 0);
    progressAndControlsLayout->addLayout(progressLayout);
    progressAndControlsLayout->addLayout(controlsAndVolumeLayout);

    // 设置底部布局
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);
    bottomLayout->addLayout(progressAndControlsLayout);

    bottomContainer->setLayout(bottomLayout);
    bottomContainer->setFixedHeight(100);
    bottomContainer->resize(640, bottomContainer->height());
    bottomContainer->setVisible(true);
    bottomContainer->setAttribute(Qt::WA_StyledBackground, true);
    this->setAttribute(Qt::WA_StyledBackground, true);

    // 设置主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(videoContainer);
    mainLayout->addWidget(bottomContainer);

    this->setLayout(mainLayout);

    setWindowStyle();
}
