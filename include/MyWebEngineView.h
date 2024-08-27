#pragma once

#include <QWebEngineView>
#include <QTabWidget>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

class MyWebEngineView : public QWebEngineView
{
    Q_OBJECT

public:
    // 显式构造函数，初始化一个 MyWebEngineView 对象，并将它与特定的 QTabWidget 关联起来
    explicit MyWebEngineView(QTabWidget *tabWidget, QWidget *parent = nullptr);

    // 重写createWindow，实现了在浏览器应用程序中打开一个新标签页的功能
    MyWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;

protected:
    // 重写的上下文菜单事件处理函数
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    // 在新标签页中打开链接
    void openLinkInNewTab();

private:
    QTabWidget *m_tabWidget; // 存储关联的 QTabWidget 指针
    QClipboard *clipboard; // 复制版
    void saveImageFromClipboard(const QString &filePath); // 将图片从剪贴板保存到文件夹
    void applyCustomScrollBarStyle(); // 加载滚动条样式
};

