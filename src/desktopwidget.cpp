#include "include/desktopwidget.h"
#include <QApplication>
#include <QScreen>
#include <qvector.h>
#include <qdebug.h>
#include <qdir.h>
#include <qpainter.h>
#include <QThread>
#include <windows.h>
#include <sstream>
#include <QString>

QSet<QString> DesktopWidget::imageFormats;
QSet<QString> DesktopWidget::videoFormats;

DesktopWidget::DesktopWidget(QWidget* parent)
    : QWidget(parent)
    , bklabel(new QLabel(this))
{
    // 保存当前壁纸路径
    TCHAR path[MAX_PATH];
    SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0); // 请求获取当前的桌面壁纸路径且不更新系统配置文件或通知系统变化
    originalWallpaperPath = QString::fromWCharArray(path);

    imageFormats = { "jpg", "jpeg", "png", "bmp", "gif" };//添加支持的格式
    videoFormats = { "mp4", "avi", "mov", "mkv", "flv", "wmv" };

    // 获取主屏幕
    QScreen* screen = QApplication::primaryScreen();//获取主屏幕的 QScreen 对象

    // 获取屏幕分辨率
    QRect screenGeometry = screen->geometry(); // 获取屏幕的几何信息，包括屏幕的宽度和高度。
    screenWidth = screenGeometry.width();
    screenHeight = screenGeometry.height();

    this->setFixedSize(screenWidth, screenHeight);
    this->setWindowTitle("BackGround");

    layout = new QHBoxLayout(this);

    layout->setMargin(0);

    show_window = (HWND)this->winId();
    SetBackground(show_window); //将当前窗口设置为桌面背景的一部分

    // 创建播放列表并添加视频
    playlist = new QMediaPlaylist();
    playlist->setPlaybackMode(QMediaPlaylist::Loop);
    videoPlayer = new QMediaPlayer(this);
    videoWidget = new QVideoWidget(this);
    videoWidget->setGeometry(0, 0, screenWidth, screenHeight);
    videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);

    // 创建图片显示label
    bklabel->setFixedSize(screenWidth, screenHeight);
    videoPlayer->setVideoOutput(videoWidget);
    layout->addWidget(bklabel);
    layout->addWidget(videoWidget);

    wallpaperEmptyAreaColor = Qt::white; // 填充色初始化为白色
    loadWallpaperEmptyAreaColor();
}

DesktopWidget::~DesktopWidget(){
}

void DesktopWidget::UpdateWallpaper() {
    if (filePath_.isEmpty()) {
        return;
    }
    QString suffix = QFileInfo(filePath_).suffix().toLower();
    if (imageFormats.contains(suffix)) {
        showImage(filePath_);
//        qDebug() << "The file is an image." << filePath_;
    }
    else if (videoFormats.contains(suffix)) {
        videoWidget->show();
        showVideo(filePath_);
//        qDebug() << "The file is a video." << filePath_;
    }
    else {
//        qDebug() << "Unknown file type.";
    }
}

void DesktopWidget::showVideo(QString filePath) {
    playlist->clear();
    playlist->setCurrentIndex(0);
    playlist->addMedia(QUrl::fromLocalFile(filePath));
    videoPlayer->setPlaylist(playlist);
    connect(videoPlayer, &QMediaPlayer::mediaStatusChanged, this, [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {  // 确保媒体已加载
            this->showFullScreen();
            videoPlayer->play();
            bklabel->hide();
        }
    });
}

void DesktopWidget::showImage(QString filePath) {
    if (QPixmap(filePath).isNull()) {
        return;
    }
    QPixmap bkPixmap;
    bool success = bkPixmap.load(filePath);
    if (!success) {
        qDebug() << "image load faild.";
        return;
    }
    bklabel->setPixmap(QPixmap());
    bklabel->setStyleSheet(QString("background-color: %1;").arg(wallpaperEmptyAreaColor .name()));
    switch (imageMode_) {
    case 0:
    {
        bkPixmap = bkPixmap.scaled(screenWidth, screenHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
//        qDebug() << "填充";
    }
        break;
    case 1:
    {
        bkPixmap = bkPixmap.scaled(screenWidth, screenHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        bklabel->setAlignment(Qt::AlignCenter);
//        qDebug() << "适应";
    }
        break;
    case 2:
    {
        bkPixmap = bkPixmap.scaled(screenWidth, screenHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
//        qDebug() << "拉伸";
    }
        break;
    case 3:
    {
        bklabel->setAlignment(Qt::AlignCenter);
//        qDebug() << "居中";
    }
        break;
    case 4:
    {
        bklabel->setStyleSheet("");
        QPalette palette;
        palette.setBrush(bklabel->backgroundRole(), QBrush(bkPixmap));
        bklabel->setAutoFillBackground(true);
        bklabel->setPalette(palette);
        this->showFullScreen();
//        qDebug() << "平铺";
        return;
    }
    }
    videoPlayer->stop();
    videoPlayer->setMedia(QMediaContent());  // 清除当前媒体
    bklabel->setPixmap(bkPixmap);
    bklabel->setAutoFillBackground(false); // 清除可能存在的背景色设置
    bklabel->show();
    this->showFullScreen();
    bklabel->resize(this->size());
}

//获取背景窗体句柄
HWND DesktopWidget::GetBackground() {

    //遍历所有workerW类型的窗体，通过比较父窗体是不是Program Manager来找到背景窗体
    HWND hwnd = FindWindowA("progman", "Program Manager"); // 查找名为 Program Manager 的窗口，其类名为 "progman"，这是桌面窗口的根窗口
    worker = NULL;
    do {
        worker = FindWindowExA(NULL, worker, "WorkerW", NULL);
        if (worker && GetParent(worker) == hwnd) {
            return worker; // 找到 WorkerW 窗口，返回句柄
        }
    } while (worker != NULL);

    //没有找到，发送消息生成一个WorkerW窗体
    SendMessage(hwnd, 0x052C, 0, 0);

    // 再次查找 WorkerW 窗口
    do {
        worker = FindWindowExA(NULL, worker, "WorkerW", NULL);
        if (worker && GetParent(worker) == hwnd) {
            return worker; // 找到 WorkerW 窗口，返回句柄
        }
    } while (worker != NULL);
    return NULL;
}

void DesktopWidget::SetBackground(HWND child) {
    SetParent(child, GetBackground()); // 把视频窗口设置为Program Manager的子项
}

void DesktopWidget::resizeEvent(QResizeEvent* event) {
        QWidget::resizeEvent(event);
}

void DesktopWidget::restoreWallpaper() {  // 恢复原始壁纸
    if (!originalWallpaperPath.isEmpty()) {
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, const_cast<void*>(static_cast<const void*>(originalWallpaperPath.utf16())), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        // 请求设置新的桌面壁纸 更新配置文件并通知系统壁纸已更改
    }
}

void DesktopWidget::closeEvent(QCloseEvent* event)
{
    // 停止视频播放并清除媒体内容
    if (videoPlayer) {
        videoPlayer->stop();
        videoPlayer->setMedia(QMediaContent());
        delete videoPlayer;
    }

    // 删除布局和控件
    delete bklabel;
    delete videoWidget;
    delete playlist;

    // 删除布局管理器
    delete layout;

    // 调用基类的closeEvent
    QWidget::closeEvent(event);
}

// 读取保存的填充色
void DesktopWidget::loadWallpaperEmptyAreaColor() {
    QString statusConfigFilePath = QDir::currentPath() + "/resource/status_config.txt";
    QFile statusConfigFile(statusConfigFilePath);

    QString fillColor;
    bool colorFound = false;

    // 尝试打开文件进行读取
    if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&statusConfigFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed(); // 读取并去除首尾空白
            if (line.startsWith("fill_color:")) {
                fillColor = line.mid(QString("fill_color:").length()).trimmed();
                colorFound = true;
                break;
            }
        }
        statusConfigFile.close();
    } else {
        qDebug() << "Failed to open file for reading:" << statusConfigFilePath;
        return;
    }

    // 将读取的颜色值转换为 QColor 对象
    QColor loadedColor(fillColor);
    if (loadedColor.isValid()) {
        // 赋值给 wallpaperEmptyAreaColor
        wallpaperEmptyAreaColor = loadedColor;
//        qDebug() << "Loaded color:" << loadedColor.name();

        // 更新 bklabel 的样式表以应用颜色
        bklabel->setStyleSheet(QString("background-color: %1;").arg(loadedColor.name()));
    } else {
        qDebug() << "Invalid color read from file:" << fillColor;
    }
}



void DesktopWidget::resetToDefault()
{
    // 停止视频播放并清除媒体内容
    if (videoPlayer) {
        videoPlayer->stop();
        videoPlayer->setMedia(QMediaContent());  // 清除当前媒体
    }

    // 清除图片显示
    bklabel->clear();
    bklabel->setPixmap(QPixmap()); // 清除可能存在的图片
    bklabel->setStyleSheet(QString("background-color: %1;").arg(wallpaperEmptyAreaColor.name())); // 重置背景色

    // 恢复原始桌面壁纸
    restoreWallpaper();

    // 隐藏窗口以避免阻挡默认壁纸
    this->hide();

    // 释放 show_window 句柄
    SetParent(show_window, NULL);  // 解除窗口与桌面背景的关联

}

void DesktopWidget::restoreWindow()
{
    // 确保窗口句柄有效
    if (!this->isVisible()) {
        // 显示窗口
        this->show();
    }

    // 获取主屏幕的句柄
    HWND backgroundHwnd = GetBackground();
    if (backgroundHwnd) {
        // 设置窗口为主屏幕的子窗口
        SetParent((HWND)this->winId(), backgroundHwnd);
    }

}
