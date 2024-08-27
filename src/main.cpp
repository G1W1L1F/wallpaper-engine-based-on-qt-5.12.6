#include<qapplication.h>
#include <include/mainwidget.h>
#include <QSharedMemory> // 通过互斥量控制一次只能打开一个实例
#include <QCoreApplication>

int main(int argc, char* argv[]) {

    QApplication app(argc, argv);

    // 设置当前工作目录为应用程序的目录
    // 在qt里面打开时要去掉这一行，以免更改当前目录，单独使用exe文件需要加上以启用开机自启动功能
    QDir::setCurrent(QCoreApplication::applicationDirPath());
//    qDebug() << "Current working directory:" << QDir::currentPath();
    // 定义全局样式表
    QString messageBoxStyleSheet =
            "QMessageBox {"
            "    background-color: #2c3e50;" /* 背景颜色 */
            "    color: #ecf0f1;"            /* 文字颜色 */
            "    border: 1px solid #34495e;" /* 边框颜色 */
            "    border-radius: 0px;"       /* 圆角 */
            "}"
            "QMessageBox QLabel {"
            "    color: #ecf0f1;"            /* 标签文字颜色 */
            "    font-size: 14px;"           /* 字体大小 */
            "}"
            "QMessageBox QPushButton {"
            "    background-color: #34495e;" /* 按钮背景颜色 */
            "    color: #ecf0f1;"            /* 按钮文字颜色 */
            "    border: none;"
            "    padding: 5px 15px;"        /* 内边距 */
            "    border-radius: 5px;"       /* 按钮圆角 */
            "}"
            "QMessageBox QPushButton:hover {"
            "    background-color: #3a536b;" /* 鼠标悬停时按钮背景颜色 */
            "}"
            "QMessageBox QPushButton:pressed {"
            "    background-color: #2b3e50;" /* 按钮按下时背景颜色 */
            "}"
            "QMessageBox QPushButton:focus {"
            "    outline: none;"             /* 按钮聚焦时无边框 */
            "}";

    // 设置全局样式表
    app.setStyleSheet(messageBoxStyleSheet);

    // 创建一个 QSharedMemory 对象
    QSharedMemory sharedMemory;
    sharedMemory.setKey("WallpaperKey");

    // 尝试附加到现有的共享内存段
    if (sharedMemory.attach()) {
        // 如果能附加，说明已经有一个实例在运行
        QMessageBox::warning(nullptr, QObject::tr("警告"),
                             QObject::tr("程序已经在运行中，无法启动多个实例。"));
        return -1; // 退出应用程序
    }

    // 尝试创建共享内存段
    if (!sharedMemory.create(1)) {
        QMessageBox::warning(nullptr, QObject::tr("错误"),
                             QObject::tr("无法创建共享内存段。"));
        return -1; // 退出应用程序
    }


    app.setWindowIcon(QIcon(":/resource/my_button_icon/gear.png")); // 设置窗口图标

    MainWidget mainwidget;


    // 检查命令行参数是否包括 --minimized
    bool minimizeToTray = false;
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--minimized") {
            minimizeToTray = true;
            break;
        }
    }

    if (minimizeToTray) {
        mainwidget.hide();
        mainwidget.trayIcon->show();
    } else {
        mainwidget.show();
        //        mainwidget.trayIcon->hide();
    }
    return app.exec();
}
