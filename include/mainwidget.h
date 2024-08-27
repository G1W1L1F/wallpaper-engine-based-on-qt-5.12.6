#pragma once

#include <QWidget>
#include "ui_mainwidget.h"
#include "include/ImageGroup.h"
#include "include/desktopwidget.h"
#include "include/listwidgetitem.h"
#include "include/videoview.h"
#include "include/EnterKeyEventFilter.h"
#include <QMessageBox>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include "include/MyWebEngineView.h"
#include <QMenuBar>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <windows.h>
#include <QSettings>

class MainWidget : public QWidget
{
    Q_OBJECT

public:

    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

    QSystemTrayIcon *trayIcon; // 最小化至托盘的图标

private:
    void init();
    Ui::MainWidgetClass* ui;
    QString saveImageDir; // 图库路径
    ImageGroup* imageGroup; // 处理缩略图生成以及在主窗口显示各个壁纸图标
    DesktopWidget* desktopWidget; // 处理设置壁纸为桌面壁纸的逻辑
    QString selectImage_pre; // 设置选中的图片预览图的路径
    QString selectImage_; // 设置选中的图片的路径
    int imageMode_; // 设置图片填充模式
    QList<QWidget*> widgetsToClose; // 待关闭窗口列表，用来维护在主窗口打开的子窗口的回收与销毁
    QStringList videoFormats = {"mp4", "avi", "mov", "mkv", "flv", "wmv"};
    QStringList imageFormats = { "jpg", "jpeg", "png", "bmp", "gif" }; // 支持的壁纸文件格式
    QString messageBoxStyleSheet; // 消息窗口样式表
    QMenuBar *menuBar; // 菜单栏
    QString current_Engine_BG_Path_; // 当前软件背景路径
    QColorDialog *globalColorDialog; // 调色盘
    void createMenuBar(); // 创建顶部菜单栏
    void setupTabWidget(); // 初始化标签栏
    void changeBG(); // 更改壁纸
    void clearSelectionOnTabChange(); // 清除listwidget的选中项
    void openBGdir(); // 打开图库
    void setStartup(); // 设置开机自启动
    void change_engine_BG(); // 改变软件背景
    void Load_Engine_BG(); // 加载软件背景
    void changeWallpaperEmptyAreaColor(); // 改变填充色
    void load_ImageMode(); // 加载图片背景格式
    void ensureStatusConfigFileExists(); // 初始化配置文件
    void pre_process_engine_BG(); // 载入当前背景图进行预处理
    bool now_is_video;
    bool desktophide;
    QPixmap engine_BG_pixmap; // 软件背景

protected:
    void closeEvent(QCloseEvent* event);
    void paintEvent(QPaintEvent* event);
    void confirmAndDelete(); // 确认删除消息框
    bool eventFilter(QObject *obj, QEvent *event) override;
    void checkAndLoadBG(); // 加载桌面壁纸

public slots:
    void addIconToList(QString filePath); // 添加各个壁纸的图标显示

private slots:
    void updateImageMode(int imageMode); // 更新桌面图片填充模式
    void enlargeImage(QListWidgetItem* item); // 查看图片
    void previewImage(QListWidgetItem* item); // 预览图片
    void on_ImageListBnt_clicked(); // 图库按钮
    void on_SetDesktop_clicked(); // 设置壁纸按钮
    void showContextMenu(const QPoint& pos); // 全部壁纸的上下文菜单
    void showContextMenu_favor(const QPoint& pos); // 收藏壁纸的上下文菜单
    void deleteSelectedItems(); // 删除壁纸，会连带删除文件
    void AddToFavor(); // 加入收藏
    void RemoveFromFavor(); // 移除收藏
    void LoadFavorites(); // 加载收藏壁纸
    void on_cancelBTN_clicked(); // 取消当前设置的壁纸
    void minimizeToTray(); // 最小化到托盘
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason); // 从最小化图标恢复
};
