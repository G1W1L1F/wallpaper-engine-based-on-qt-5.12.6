#pragma once
#include <qt_windows.h>
#include <QWidget>
#include <qlabel.h>
#include <QPixmap>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <qstackedwidget.h>
#include <QDebug>
#include <QHBoxLayout>
#include <QThread>
#include <QColor>
#include <QColorDialog>

class DesktopWidget  : public QWidget
{
    Q_OBJECT

public:
    DesktopWidget(QWidget *parent = nullptr);
    ~DesktopWidget();

    int imageMode_; // 图片壁纸显示模式 拉伸 适应等
    QString filePath_; // 壁纸来源路径
    QMediaPlayer* videoPlayer;
    QVideoWidget* videoWidget;
    QColor wallpaperEmptyAreaColor; // 图片壁纸填充色

    void SetimageMode(const int& iamgeMode) { imageMode_ = iamgeMode; } // 设置图片格式
    void SetfilePath(const QString& filePath) { filePath_ = filePath; } // 设置壁纸来源路径
    void restoreWallpaper(); // 恢复系统默认壁纸
    void resetToDefault(); //  清空当前壁纸显示
    void restoreWindow(); //  恢复壁纸显示窗口

private:
    static QSet<QString> imageFormats; // 支持的图片格式
    static QSet<QString> videoFormats; // 支持的视频格式
    QLabel* bklabel;
    QMediaPlaylist* playlist;
    QHBoxLayout* layout;
    QString originalWallpaperPath; // 本地系统默认壁纸
    HWND worker; // 获取到的桌面窗口
    HWND show_window; // 当前壁纸显示窗口
    int screenWidth;
    int screenHeight;

    HWND GetBackground(); // 获取桌面背景窗口
    void SetBackground(HWND child); // 设置壁纸显示窗口为桌面子窗口
    void showImage(QString filePath);
    void showVideo(QString filePath);
    void loadWallpaperEmptyAreaColor(); // 加载填充色
    void resizeEvent(QResizeEvent* event);

protected:
    void closeEvent(QCloseEvent* event);

public slots:
    void UpdateWallpaper(); // 更新壁纸
};
