#include "include/MyWebEngineView.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QTabBar>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QWebEnginePage>
#include <QWebEngineContextMenuData>
#include <QDebug>
#include <QCursor>
#include <QPoint>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QImage>
#include <QImageWriter>
#include <QTimer>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

MyWebEngineView::MyWebEngineView(QTabWidget *tabWidget, QWidget *parent)
    : QWebEngineView(parent), m_tabWidget(tabWidget)
{
    connect(this, &MyWebEngineView::loadFinished, this, &MyWebEngineView::applyCustomScrollBarStyle); // 加载滚动条样式
}

MyWebEngineView *MyWebEngineView::createWindow(QWebEnginePage::WebWindowType type)
{
    QWidget *newTabPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(newTabPage);

    MyWebEngineView *newWebView = new MyWebEngineView(m_tabWidget);
    layout->addWidget(newWebView);

    newTabPage->setLayout(layout);

    int index = m_tabWidget->addTab(newTabPage, "");
    connect(newWebView, &MyWebEngineView::titleChanged, [this, index](const QString &title) {
        if (index >= 3) {
            m_tabWidget->setTabText(index, title);
        }
    });
    m_tabWidget->setCurrentIndex(index);
    newWebView->setProperty("tabIndex", index);
    return newWebView;
}

void MyWebEngineView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu contextMenu(this);

    QAction *backAction = new QAction(tr("返回"), this);
    QAction *forwardAction = new QAction(tr("前进"), this);
    QAction *reloadAction = new QAction(tr("刷新页面"), this);
    QAction *openInNewTabAction = new QAction(tr("在新标签页中打开"), this);

    connect(backAction, &QAction::triggered, this, &QWebEngineView::back);
    connect(forwardAction, &QAction::triggered, this, &QWebEngineView::forward);
    connect(reloadAction, &QAction::triggered, this, &QWebEngineView::reload);
    connect(openInNewTabAction, &QAction::triggered, this, &MyWebEngineView::openLinkInNewTab);

    QAction *saveImageAction = nullptr;
    QAction *copyImageAction = nullptr;
    QAction *copyImageUrlAction = nullptr;

    QWebEngineContextMenuData contextMenuData = page()->contextMenuData();
    if (contextMenuData.mediaType() == QWebEngineContextMenuData::MediaTypeImage) {
        saveImageAction = new QAction(tr("保存图片"), this); // 通过先将图片复制到剪贴板再保存到文件夹中
        copyImageAction = new QAction(tr("复制图片"), this);
        copyImageUrlAction = new QAction(tr("复制图片链接"), this);

        connect(saveImageAction, &QAction::triggered,  [this, contextMenuData]()  {
            // 先复制图像到剪贴板
            page()->triggerAction(QWebEnginePage::CopyImageToClipboard);
            // 获取图片的 URL 和 basename
            QUrl imageUrl = contextMenuData.mediaUrl();
            QFileInfo fileInfo(imageUrl.path());
            QString defaultFileName = fileInfo.fileName(); // 默认文件名为图像的 basename

            // 设置一个短暂的延迟来确保剪贴板内容已经更新
            QTimer::singleShot(500, [this,defaultFileName]() {
                // 打开文件对话框以选择保存文件的名称
                QString fileName = QFileDialog::getSaveFileName(this, tr("保存图片"), QDir::currentPath() + "/resource/saveimage" + "/" + defaultFileName, tr("Images (*.jpg *.jpeg *.bmp *.gif)"));
                if (!fileName.isEmpty()) {
                    saveImageFromClipboard(fileName);
                }
            });
        });

        connect(copyImageAction, &QAction::triggered, this, [this]() {
            page()->triggerAction(QWebEnginePage::CopyImageToClipboard);
        });

        connect(copyImageUrlAction, &QAction::triggered, this, [contextMenuData]() {
            QApplication::clipboard()->setText(contextMenuData.mediaUrl().toString());
        });

        contextMenu.addAction(saveImageAction);
        contextMenu.addAction(copyImageAction);
        contextMenu.addAction(copyImageUrlAction);
    }

    contextMenu.addAction(backAction);
    contextMenu.addAction(forwardAction);
    contextMenu.addAction(reloadAction);
    contextMenu.addAction(openInNewTabAction);

    contextMenu.exec(event->globalPos());
}

void MyWebEngineView::openLinkInNewTab() // 在新的标签栏打开网址
{
    QWebEngineContextMenuData contextMenuData = page()->contextMenuData();

    if (contextMenuData.linkUrl().isValid()) {
        QUrl linkUrl = contextMenuData.linkUrl();

        MyWebEngineView *newWebView = createWindow(QWebEnginePage::WebBrowserTab);
        newWebView->setUrl(linkUrl);
    } else {
        qDebug() << "未选中有效的链接。";
    }
}

void MyWebEngineView::saveImageFromClipboard(const QString &filePath) // 将图片从剪贴板保存到文件夹
{
    clipboard = QApplication::clipboard();
    const QPixmap pixmap = clipboard->pixmap();

    if (!pixmap.isNull()) {
        QImage image = pixmap.toImage();
        QImageWriter writer(filePath);
        if (writer.write(image)) {
//            qDebug() << "Image saved to:" << filePath;
        } else {
            qDebug() << "Failed to save image:" << writer.errorString();
        }
    } else {
        qDebug() << "No image found in clipboard.";
    }
}

void MyWebEngineView::applyCustomScrollBarStyle() // 加载滚动条样式
{
    QFile cssfile(":/web_scrollbarstyle.css");
    if (cssfile.open(QFile::ReadOnly)) {
        QTextStream stream(&cssfile);
        const QString styleJS = QString::fromLatin1("(function() {"\
            "    css = document.createElement('style');"\
            "    css.type = 'text/css';"\
            "    css.id = '%1';"\
            "    document.head.appendChild(css);"\
            "    css.innerText = '%2';"\
            "})()\n").arg("scrollbarStyle").arg(stream.readAll().simplified());
        cssfile.close();
        QWebEngineScript script;
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setSourceCode(styleJS);
        page()->scripts().insert(script);
        page()->runJavaScript(styleJS, QWebEngineScript::ApplicationWorld);
    }
}
