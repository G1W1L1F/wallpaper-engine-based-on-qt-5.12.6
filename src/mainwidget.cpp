#include "include/mainwidget.h"
#include "include/imageview.h"

#include <qdebug.h>
#include <qlistWidget.h>
#include <qfiledialog.h>
#include <QVBoxLayout>
#include <qpainter.h>
#include <qmenu.h>
#include <qaction.h>
#include <QDesktopServices>
#include <QProcess>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidgetClass)
    , desktopWidget(new DesktopWidget(nullptr))
    , imageMode_(0)
    , desktophide(false)
{
    init();

    menuBar->setGeometry(0, 0, 4000, menuBar->height()); // 让menubar在宽度上占满窗口，4000为人为设定的一个合适值
    desktopWidget->setAttribute(Qt::WA_DeleteOnClose);
    imageGroup = new ImageGroup(this);

    ui->ImagelistWidget->viewport()->installEventFilter(this); // 安装事件过滤器，单击空白处取消之前所有选中项目
    ui->FavoritelistWidget->viewport()->installEventFilter(this); // 不能直接添加，需要给viewport添加

    ui->videoVolumeSlider->setRange(0, 100);
    desktopWidget->videoPlayer->setVolume(50);
    connect(ui->videoVolumeSlider,&QSlider::sliderMoved,desktopWidget->videoPlayer,[=](int position) // 设置视频壁纸音量
    {
        desktopWidget->videoPlayer->setVolume(position);
    });
    if(now_is_video)
    {
        ui->videoVolumeSlider->show(); // 视频壁纸音量控制条
        ui->wallpaperMode->hide(); // 图片壁纸显示的下拉菜单
    }
    else
    {
        ui->videoVolumeSlider->hide(); // 视频壁纸音量控制条
        ui->wallpaperMode->show(); // 图片壁纸显示的下拉菜单
    }

    connect(imageGroup, &ImageGroup::sendImage, this, &MainWidget::addIconToList, Qt::QueuedConnection);  //添加图标到列表，跨线程时传递信号和槽
    connect(ui->ImagelistWidget, &QListWidget::itemDoubleClicked, this, &MainWidget::changeBG);  //双击更换壁纸
    connect(ui->ImagelistWidget, &QListWidget::itemClicked, this, &MainWidget::previewImage);  //单击预览图片
    connect(ui->ImagelistWidget, &QListWidget::customContextMenuRequested, this, &MainWidget::showContextMenu);//右键选中图片

    connect(ui->FavoritelistWidget, &QListWidget::itemDoubleClicked, this, &MainWidget::changeBG);  //双击更换壁纸
    connect(ui->FavoritelistWidget, &QListWidget::itemClicked, this, &MainWidget::previewImage);  //单击预览图片
    connect(ui->FavoritelistWidget, &QListWidget::customContextMenuRequested, this, &MainWidget::showContextMenu_favor);//右键选中图片

    connect(ui->wallpaperMode, SIGNAL(currentIndexChanged(int)), this, SLOT(updateImageMode(int)));
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWidget::clearSelectionOnTabChange); // 切换页面情空选中项

    imageGroup->creatPreviewPixmap(); // 显示壁纸图标

}

void MainWidget::init()
{
    ui->setupUi(this);
    ensureStatusConfigFileExists(); // 初始化配置文件
    Load_Engine_BG();
    load_ImageMode();
    createMenuBar();
    LoadFavorites();
    setupTabWidget();
    checkAndLoadBG();
    trayIcon = new QSystemTrayIcon(QIcon(":/resource/my_button_icon/gear.png"), this);
    globalColorDialog = new QColorDialog(Qt::black); // 初始化调色盘

    // 创建托盘图标的上下文菜单
    QMenu *trayMenu = new QMenu(this);
    QAction *quitAction = new QAction(tr("退出"), this);
    connect(quitAction, &QAction::triggered, this,  &MainWidget::close);
    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWidget::onTrayIconActivated);
    trayIcon->show();

    this->setWindowTitle("Wallpaper");//  设置窗口的标题为 "Wallpaper"
    ui->ImageNameInfoLabel->setText("图像名称");
    ui->ImageNameInfoLabel->setWordWrap(true);
    ui->ImageSizeInfoLabel->setText("分辨率");
    ui->ImageSizeInfoLabel->setWordWrap(true);
    ui->ImagePreview->setText("预览图象");

    //设置壁纸列表
    ui->ImagelistWidget->setIconSize(QSize(195, 195));//设置单个Icon大小
    ui->ImagelistWidget->setViewMode(QListView::IconMode);//设置显示模式
    ui->ImagelistWidget->setFlow(QListView::LeftToRight);//从左到右
    ui->ImagelistWidget->setResizeMode(QListView::Adjust);//大小自适应
    ui->ImagelistWidget->setMovement(QListView::Static);//设置列表每一项不可移动
    ui->ImagelistWidget->setSelectionMode(QAbstractItemView::MultiSelection);//设置是否允许多选 SingleSelection MultiSelection
    ui->ImagelistWidget->setContextMenuPolicy(Qt::CustomContextMenu);//设置允许菜单

    ui->FavoritelistWidget->setIconSize(QSize(195, 125));//设置单个Icon大小
    ui->FavoritelistWidget->setViewMode(QListView::IconMode);//设置显示模式
    ui->FavoritelistWidget->setFlow(QListView::LeftToRight);//从左到右
    ui->FavoritelistWidget->setResizeMode(QListView::Adjust);//大小自适应
    ui->FavoritelistWidget->setMovement(QListView::Static);//设置列表每一项不可移动
    ui->FavoritelistWidget->setSelectionMode(QAbstractItemView::MultiSelection);//设置是否允许多选
    ui->FavoritelistWidget->setContextMenuPolicy(Qt::CustomContextMenu);//设置允许菜单

    //设置桌面图片填充模式
    ui->wallpaperMode->addItem("填充", QVariant("fill"));
    ui->wallpaperMode->addItem("适应", QVariant("fit"));
    ui->wallpaperMode->addItem("拉伸", QVariant("stretch"));
    ui->wallpaperMode->addItem("居中", QVariant("center"));
    ui->wallpaperMode->addItem("平铺", QVariant("tile"));
}

// 向图片列表添加缩略图图标
void MainWidget::addIconToList(QString filePath) {
    QFileInfo fileInfo(filePath);
    QString fileSuffix = fileInfo.suffix().toLower();
    QString baseName = fileInfo.baseName();
    QString dirPath = fileInfo.dir().path();
    QString thumbnailPath;

    // 根据文件类型设置缩略图路径
    if (videoFormats.contains(fileSuffix)) {
        thumbnailPath = dirPath + "/" + baseName + "_preview.gif";
    } else if (imageFormats.contains(fileSuffix)) {
        thumbnailPath = dirPath + "/" + baseName + "_preview.jpg";
    }

//    qDebug() << "basename:" << baseName << "dirpath:" << dirPath << "thumbnail" << thumbnailPath;

    // 创建新的 QListWidgetItem 和 ListWidgetItem
    QListWidgetItem* newitem = new QListWidgetItem();
    ListWidgetItem* itemWidget = new ListWidgetItem(thumbnailPath);
    newitem->setSizeHint(itemWidget->sizeHint());
    newitem->setData(Qt::UserRole, QVariant(thumbnailPath));
    newitem->setData(Qt::UserRole + 1, QVariant(filePath));
    newitem->setData(Qt::UserRole + 2, QVariant(dirPath));
    newitem->setText(""); // 不需要显示文本

    // 插入新项到列表的最前面
    ui->ImagelistWidget->insertItem(0, newitem);
    ui->ImagelistWidget->setItemWidget(newitem, itemWidget);
}

void MainWidget::showContextMenu(const QPoint& pos)
{
    // 获取被点击的 QListWidgetItem
    QListWidgetItem* clickedItem = ui->ImagelistWidget->itemAt(pos);
    // 如果点击的位置不在任何项上，则直接返回
    if (!clickedItem) {
        ui->ImagelistWidget->clearSelection();
        return;
    }
    // 将被点击的项添加到选中的项
    ui->ImagelistWidget->setItemSelected(clickedItem, true);
    previewImage(clickedItem);
    QMenu contextMenu(tr("Context menu"), this);

    QAction action_set_desktop("设置为壁纸", this);
    connect(&action_set_desktop, &QAction::triggered, this, &MainWidget::changeBG);
    contextMenu.addAction(&action_set_desktop);

    QAction actionDelete("删除", this);
    connect(&actionDelete, &QAction::triggered, this, &MainWidget::confirmAndDelete);
    contextMenu.addAction(&actionDelete);

    QAction actionAddToFavor("加入收藏", this);
    connect(&actionAddToFavor, &QAction::triggered, this, &MainWidget::AddToFavor);
    contextMenu.addAction(&actionAddToFavor);
    // 获取被点击的 QListWidgetItem
    QString filepath = clickedItem->data(Qt::UserRole + 1).toString();
    QFileInfo fileInfo(filepath);
    QString fileSuffix = fileInfo.suffix().toLower();

    // 查看原视频/原图
    QAction *actionEnlargeImage = nullptr;
    if (videoFormats.contains(fileSuffix)) {
        actionEnlargeImage = new QAction("查看原视频", this);
    }
    else
    {
        actionEnlargeImage = new QAction("查看原图", this);
    }
    if (clickedItem) {
        connect(actionEnlargeImage, &QAction::triggered, this, [=]() {
            //            QList<QListWidgetItem*> selectedItems = ui->ImagelistWidget->selectedItems();
            //            for (QListWidgetItem* item : selectedItems) {
            //                this->enlargeImage(item);
            //            }
            this->enlargeImage(clickedItem); // 只显示当前右键选中壁纸的源文件，而不是所有被选中的壁纸
        });
    }
    contextMenu.addAction(actionEnlargeImage);

    QAction* actionOpenInFolder = new QAction("在文件夹中打开", this);
    connect(actionOpenInFolder, &QAction::triggered, this, [=]() {
        QList<QListWidgetItem*> selectedItems = ui->ImagelistWidget->selectedItems();
        for (QListWidgetItem* item : selectedItems) {
            QString filepath = item->data(Qt::UserRole + 1).toString();
            QString command = QString("explorer.exe /select,\"%1\"").arg(QDir::toNativeSeparators(filepath));
            QProcess::startDetached(command);
        }
    });
    contextMenu.addAction(actionOpenInFolder);
    contextMenu.exec(ui->ImagelistWidget->viewport()->mapToGlobal(pos));
}

void MainWidget::showContextMenu_favor(const QPoint& pos) // 收藏夹壁纸右键菜单
{
    // 获取被点击的 QListWidgetItem
    QListWidgetItem* clickedItem = ui->FavoritelistWidget->itemAt(pos);
    // 如果点击的位置不在任何项上，则直接返回
    if (!clickedItem) {
        ui->FavoritelistWidget->clearSelection();
        return;
    }
    ui->FavoritelistWidget->setItemSelected(clickedItem, true);

    previewImage(clickedItem);
    QMenu contextMenu(tr("Context menu"), this);

    QAction action_set_desktop("设置为壁纸", this);
    connect(&action_set_desktop, &QAction::triggered, this, &MainWidget::changeBG);
    contextMenu.addAction(&action_set_desktop);

    QAction actionRemoveFavor("移除收藏", this);
    connect(&actionRemoveFavor, &QAction::triggered, this, &MainWidget::RemoveFromFavor);
    contextMenu.addAction(&actionRemoveFavor);
    QString filepath = clickedItem->data(Qt::UserRole + 1).toString();
    QFileInfo fileInfo(filepath);
    QString fileSuffix = fileInfo.suffix().toLower();

    // 查看原视频/原图
    QAction *actionEnlargeImage = nullptr;
    if (videoFormats.contains(fileSuffix)) {
        actionEnlargeImage = new QAction("查看原视频", this);
    }
    else
    {
        actionEnlargeImage = new QAction("查看原图", this);
    }
    if (clickedItem) {
        connect(actionEnlargeImage, &QAction::triggered, this, [=]() {
            QList<QListWidgetItem*> selectedItems = ui->FavoritelistWidget->selectedItems();
            for (QListWidgetItem* item : selectedItems) {
                this->enlargeImage(item);
            }
        });
    }
    contextMenu.addAction(actionEnlargeImage);

    QAction* actionOpenInFolder = new QAction("在文件夹中打开", this);
    connect(actionOpenInFolder, &QAction::triggered, this, [=]() {
        QList<QListWidgetItem*> selectedItems = ui->FavoritelistWidget->selectedItems();
        for (QListWidgetItem* item : selectedItems) {
            QString filepath = item->data(Qt::UserRole + 1).toString();
            QString command = QString("explorer.exe /select,\"%1\"").arg(QDir::toNativeSeparators(filepath));
            QProcess::startDetached(command);
        }
    });
    contextMenu.addAction(actionOpenInFolder);
    contextMenu.exec(ui->FavoritelistWidget->viewport()->mapToGlobal(pos));
}

void MainWidget::confirmAndDelete() {
    QMessageBox confirmBox;
    confirmBox.setWindowTitle("");
    confirmBox.setText("确认删除选中的壁纸?");
    confirmBox.setIcon(QMessageBox::Warning);
    QPushButton *yesButton = confirmBox.addButton(tr("是"), QMessageBox::YesRole);
    confirmBox.addButton(tr("否"), QMessageBox::NoRole);
    confirmBox.exec();

    if (confirmBox.clickedButton() == yesButton) {
        deleteSelectedItems();  // 如果用户选择了 "Yes"，执行删除操作
    } else {
        // 用户选择了 "No"，取消删除操作
    }
}

void MainWidget::deleteSelectedItems() {
    QString curBGPath;
    QString statusConfigFilePath = QDir::currentPath() + "/resource/status_config.txt";
    QFile statusConfigFile(statusConfigFilePath);
    if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&statusConfigFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("current_background:")) {
                curBGPath = line.mid(QString("current_background:").length()).trimmed();
                break;
            }
        }
        statusConfigFile.close();
    } else {
        qWarning() << "Failed to open status_config.txt for reading.";
        return;
    }

    // 读取配置文件用于检查收藏情况
    QFile configFile("resource/config_favor.txt");
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Unable to open config file for reading.";
        return;
    }

    QTextStream in(&configFile);
    QStringList existingPaths;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        existingPaths.append(line);
    }
    configFile.close();

    // 将 QListWidget 中的所有项目存储到一个临时列表中
    QList<QListWidgetItem*> itemsToDelete;

    // 遍历选中的项
    auto selectedItems = ui->ImagelistWidget->selectedItems();
    for (auto* item : selectedItems) {
        QString path0 = item->data(Qt::UserRole).toString();

        // 检查 path0 是否在收藏文件中
        if (existingPaths.contains(path0)) {
            QMessageBox warningBox;
            warningBox.setWindowTitle(tr("无法删除"));
            warningBox.setText(tr("该壁纸已被收藏，无法删除。"));
            warningBox.setIcon(QMessageBox::Warning);
            warningBox.exec();
            continue;  // 跳过删除此项，继续下一项
        }

        itemsToDelete.append(item);
    }

    // 从 QListWidget 中删除项
    for (auto* item : itemsToDelete) {
        QString path1 = item->data(Qt::UserRole + 1).toString();
        QString path2 = item->data(Qt::UserRole + 2).toString();

        if (path1 == curBGPath) {
            if(!desktophide) {
                desktopWidget->resetToDefault();
                desktophide = true;
            }

            // 清除 status_config.txt 文件中 current_background: 的内容
            QStringList lines;
            bool backgroundCleared = false;

            if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&statusConfigFile);
                while (!in.atEnd()) {
                    QString line = in.readLine().trimmed();
                    if (line.startsWith("current_background:")) {
                        lines.append("current_background:");  // 清空背景路径
                        backgroundCleared = true;
                    } else {
                        lines.append(line);
                    }
                }
                statusConfigFile.close();
            }

            if (backgroundCleared) {
                if (statusConfigFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                    QTextStream out(&statusConfigFile);
                    for (const QString& line : lines) {
                        out << line << "\n";
                    }
                    statusConfigFile.close();
                } else {
                    qWarning() << "Failed to open status_config.txt for writing.";
                }
            }
        }

        // 清理ImagelistWidget
        QWidget* widget = ui->ImagelistWidget->itemWidget(item);
        if (widget) {
            QLabel* label = widget->findChild<QLabel*>();
            if (label) {
                if (label->movie()) {
                    QMovie* currentMovie_label = label->movie();
                    label->setMovie(nullptr);
                    currentMovie_label->stop();
                    delete currentMovie_label;
                }
                label->clear();
            }
            ui->ImagelistWidget->removeItemWidget(item);
            delete widget;
        }

        // 清理ImagePreview
        if (ui->ImagePreview) {
            if (ui->ImagePreview->movie()) {
                QMovie* currentMovie_pre = ui->ImagePreview->movie();
                ui->ImagePreview->setMovie(nullptr);
                currentMovie_pre->stop();
                delete currentMovie_pre;
            }
            ui->ImagePreview->clear();
        }

        // 清理ImageGroup数组
        QString baseName = QFileInfo(path1).fileName();
        for (int i = 0; i < imageGroup->all_images_.size(); ++i) {
            QFileInfo sourceFileInfo(imageGroup->all_images_.at(i));
            if (sourceFileInfo.fileName() == baseName) {
                imageGroup->all_images_.removeAt(i);
                break; // 如果只需要删除第一个匹配项，找到后就退出循环
            }
        }

        delete item; // 删除选中的项

        // 删除数据文件夹
        if (!path2.isEmpty()) {
            QDir dir(path2);
            if (dir.exists()) {
                if (dir.removeRecursively()) {
//                    qDebug() << "Successfully deleted directory and all its contents at path2:" << path2;
                } else {
                    qDebug() << "Failed to delete directory at path2:" << path2;
                }
            } else {
                qDebug() << "Directory does not exist at path2:" << path2;
            }
        }

        // 清除注册目录
        imageGroup->delete_config_line(path1);
    }
}


void MainWidget::AddToFavor() {

    QFile configFile("resource/config_favor.txt");

    // 读取现有的文件路径到一个集合中
    QSet<QString> existingPaths;
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&configFile);
        QString line;
        while (in.readLineInto(&line)) {
            existingPaths.insert(line.trimmed());
        }
        configFile.close();
    }

    // 以追加模式打开文件
    if (!configFile.open(QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Unable to open config file for writing.";
        qDebug() << "Current Path:" << QDir::currentPath();
        qDebug() << "Current Path:" << QCoreApplication::applicationDirPath();
        return;
    }

    QTextStream out(&configFile);
    bool hasNewFavorites = false;

    auto selectedItems = ui->ImagelistWidget->selectedItems();
    for (auto* item : selectedItems) {
        // 克隆 QListWidgetItem
        QListWidgetItem* newItem = new QListWidgetItem(*item);

        // 获取原始的 QWidget
        ListWidgetItem* originalWidget = qobject_cast<ListWidgetItem*>(ui->ImagelistWidget->itemWidget(item));
        if (!originalWidget) {
            continue;  // 如果无法获取原始 widget，跳过
        }
        QString filePath = originalWidget->getFilePath();

        // 检查文件路径是否已存在
        if (!existingPaths.contains(filePath)) {
            // 复制并设置关联的 QWidget
            ListWidgetItem* newWidget = new ListWidgetItem(filePath, this);

            // 将新项添加到收藏列表
            ui->FavoritelistWidget->insertItem(0, newItem);
            ui->FavoritelistWidget->setItemWidget(newItem, newWidget);

            // 将文件路径写入配置文件
            out << filePath << "\n";
            // 添加到集合中，以便后续检查
            existingPaths.insert(filePath);
            hasNewFavorites = true;
        }
    }

    configFile.close(); // 关闭文件

    if (hasNewFavorites) {
        QMessageBox warningBox;
        warningBox.setWindowTitle(tr("成功加入收藏"));
        warningBox.setText(tr("成功加入收藏。"));
        warningBox.setIcon(QMessageBox::Information);
        warningBox.exec();
    } else {
        QMessageBox warningBox;
        warningBox.setWindowTitle(tr("已加入收藏"));
        warningBox.setText(tr("所选壁纸已在收藏列表中。"));
        warningBox.setIcon(QMessageBox::Information);
        warningBox.exec();
    }
}

void MainWidget::RemoveFromFavor() {
    auto selectedItems = ui->FavoritelistWidget->selectedItems();

    // 临时存储要删除的文件路径
    QStringList pathsToRemove;

    for (auto* item : selectedItems) {
        ListWidgetItem* widget = qobject_cast<ListWidgetItem*>(ui->FavoritelistWidget->itemWidget(item));

        if (widget) {
            // 获取文件路径
            QString filePath = widget->getFilePath();

            // 添加到要删除的列表中
            pathsToRemove.append(filePath);

            // 移除 item 及其关联的 QWidget
            delete ui->FavoritelistWidget->takeItem(ui->FavoritelistWidget->row(item));
        }
    }

    if (pathsToRemove.isEmpty()) {
        QMessageBox infoBox;
        infoBox.setWindowTitle(tr("未选择项"));
        infoBox.setText(tr("未选择任何收藏项进行删除。"));
        infoBox.setIcon(QMessageBox::Information);
        infoBox.exec();
        return;
    }

    // 读取原始配置文件并重新写入，跳过要删除的路径
    QFile configFile("resource/config_favor.txt");
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Unable to open config file for reading.";
        return;
    }

    QStringList existingPaths;
    QTextStream in(&configFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!pathsToRemove.contains(line.trimmed())) { // 比较时通过line.trimmed()去掉首尾空白字符
            existingPaths.append(line);
        }
    }
    configFile.close();

    // 重新写入配置文件
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Unable to open config file for writing.";
        return;
    }

    QTextStream out(&configFile);
    for (const QString& path : existingPaths) {
        out << path << "\n";
    }
    configFile.close();

    QMessageBox warningBox;
    warningBox.setWindowTitle(tr("成功移除收藏"));
    warningBox.setText(tr("成功移除选中的收藏项。"));
    warningBox.setIcon(QMessageBox::Information);
    warningBox.exec();
}


void MainWidget::LoadFavorites() {
    QFile configFile("resource/config_favor.txt");
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Unable to open config file for reading.";
        return;
    }

    QTextStream in(&configFile);
    QString line;
    while (in.readLineInto(&line)) {
        if (!line.isEmpty()) {
            ListWidgetItem* newWidget = new ListWidgetItem(line, this);
            QListWidgetItem* newItem = new QListWidgetItem();

            QFileInfo fileInfo(line);
            QString fileName = fileInfo.completeBaseName();  // 不带后缀的文件名
            fileName.remove("_preview");
            QString dirPath = fileInfo.absolutePath();       // 获取所在文件夹路径

            QDir directory(dirPath);
            QStringList files = directory.entryList(QDir::Files);

            QString filePath;
            QString thumbnailPath;

            // 遍历文件夹中的所有文件，匹配文件名
            foreach (const QString& file, files) {
                QString baseName = QFileInfo(file).completeBaseName();

                if (baseName == fileName + "_preview") {
                    thumbnailPath = directory.absoluteFilePath(file);
                } else if (baseName == fileName) {
                    filePath = directory.absoluteFilePath(file);
                }
            }

            // 设置 newItem 的参数
            newItem->setSizeHint(newWidget->sizeHint());
            newItem->setData(Qt::UserRole, QVariant(thumbnailPath));  // 预览文件名
            newItem->setData(Qt::UserRole + 1, QVariant(filePath)); // 原始文件名
            newItem->setData(Qt::UserRole + 2, QVariant(dirPath)); // 文件夹路径
            newItem->setText("");

            ui->FavoritelistWidget->insertItem(0, newItem);
            ui->FavoritelistWidget->setItemWidget(newItem, newWidget);
        }
    }

    configFile.close();
}


// 更新图片显示模式
void MainWidget::updateImageMode(int imageMode) {
    imageMode_ = imageMode;
    desktopWidget->SetimageMode(imageMode);
}

// 查看图片
void MainWidget::enlargeImage(QListWidgetItem* item) {


    QString filepath = item->data(Qt::UserRole + 1).toString();
    QFileInfo fileInfo(filepath);
    QString fileSuffix = fileInfo.suffix().toLower();
    if (videoFormats.contains(fileSuffix)) {
        VideoView* showVideoWidget = new VideoView();
        showVideoWidget->setAttribute(Qt::WA_DeleteOnClose);
        showVideoWidget->getvideopath(filepath);
        showVideoWidget->show();
        widgetsToClose.append(showVideoWidget);
        // 连接销毁信号到自定义槽函数 可能没有起作用
        connect(showVideoWidget, &QWidget::destroyed, this, [=]() {
            widgetsToClose.removeOne(showVideoWidget);
        });
    }
    else if(imageFormats.contains(fileSuffix)){
        ImageView* showImageWidget = new ImageView();
        showImageWidget->setAttribute(Qt::WA_DeleteOnClose); //关闭窗口后释放内存
        showImageWidget->SetImage(filepath);
        showImageWidget->show();
        widgetsToClose.append(showImageWidget);
        // 连接销毁信号到自定义槽函数
        connect(showImageWidget, &QWidget::destroyed, this, [=]() {
            widgetsToClose.removeOne(showImageWidget);
        });
    }
}

// 预览图片
void MainWidget::previewImage(QListWidgetItem* item) {
    // 如果 item 为空，清除所有选中的项并返回
    if (!item) {
        ui->ImagePreview->clear();
        ui->ImageNameInfoLabel->setText("图像名称");
        ui->ImageSizeInfoLabel->setText("分辨率");
        ui->ImagePreview->setText("预览图象");
        return;
    }

    // 确保它在列表中
    if (ui->ImagelistWidget->row(item) != -1) {
        item->setSelected(true); // 选中该项
    }

    selectImage_pre = item->data(Qt::UserRole).toString();
    selectImage_ = item->data(Qt::UserRole + 1).toString();

    // 清理之前的动画或图片
    if (ui->ImagePreview->movie()) {
        QMovie* currentMovie = ui->ImagePreview->movie();
        ui->ImagePreview->setMovie(nullptr);
        currentMovie->stop();
        delete currentMovie;
    }

    ui->ImagePreview->setScaledContents(true); // 自动缩放其显示的内容
    if (QFileInfo(selectImage_pre).suffix().toLower() == "gif") {
        QMovie* movie = new QMovie(selectImage_pre);
        if(movie->isValid())
        {
            ui->ImagePreview->setMovie(movie);
//            qDebug() << selectImage_pre;
            movie->start();
        }
    }
    else {
        QPixmap pixmap(selectImage_pre);
        if (!pixmap.isNull()) {
            ui->ImagePreview->setPixmap(pixmap);
        }
        else {
            qDebug() << "Unable to load image from:" << selectImage_pre;
        }
    }
    ui->ImageNameInfoLabel->setText("图像名称: "+QFileInfo(selectImage_).fileName());

    QFileInfo fileInfo(selectImage_);
    QString fileSuffix = fileInfo.suffix().toLower();

    // 检查是否为视频文件
    if (videoFormats.contains(fileSuffix)) {
        QMediaPlayer* player = new QMediaPlayer(this);
        QVideoWidget* videoWidget = new QVideoWidget;

        player->setVideoOutput(videoWidget);
        player->setMedia(QUrl::fromLocalFile(selectImage_));

        connect(player, &QMediaPlayer::videoAvailableChanged, this, [=]() {
            if (player->isVideoAvailable()) {
                QSize videoSize = videoWidget->sizeHint(); // 获取视频的建议尺寸（即原始分辨率）
                ui->ImageSizeInfoLabel->setText(QString("视频分辨率: %1 x %2")
                                                .arg(videoSize.width())
                                                .arg(videoSize.height()));
            } else {
                ui->ImageSizeInfoLabel->setText("Failed to load video.");
            }
            player->deleteLater();
            videoWidget->deleteLater();
        });

        player->play();
    }
    else if(imageFormats.contains(fileSuffix)){
        // 处理图像文件
        QImage image(selectImage_);
        if (!image.isNull()) {
            int width = image.width();
            int height = image.height();
            ui->ImageSizeInfoLabel->setText(QString("图像分辨率: %1 x %2")
                                            .arg(width)
                                            .arg(height));
        } else {
            ui->ImageSizeInfoLabel->setText("Failed to load image.");
        }
    }
}



MainWidget::~MainWidget()
{
    delete trayIcon;   // 删除托盘图标对象
    delete ui;
}

void MainWidget::on_ImageListBnt_clicked() {
    openBGdir(); // 打开图库
}

void MainWidget::openBGdir()
{
    saveImageDir = "resource/saveimage";

    // 检查文件夹是否存在，如果不存在则创建
    QDir dir;
    if (!dir.exists(saveImageDir)) {
        if (!dir.mkpath(saveImageDir)) {
            qDebug() << "Failed to create directory:" << saveImageDir;
            return;
        }
    }

    // 打开文件对话框以选择要添加的图像文件
    QStringList file_paths = QFileDialog::getOpenFileNames(
                this,
                tr("添加壁纸"),
                saveImageDir,
                tr("所有格式 (*.jpg *.jpeg *.png *.bmp *.gif *.mp4 *.avi *.mov *.mkv *.flv *.wmv)\n"
                   "图像文件 (*.jpg *.jpeg *.png *.bmp *.gif)\n"
                   "视频文件 (*.mp4 *.avi *.mov *.mkv *.flv *.wmv)")
                );

    // 添加图片
    imageGroup->addImage(file_paths);
}

void MainWidget::changeBG()
{
    if (desktophide) {
        desktopWidget->restoreWindow();
        desktophide = false;
    }

    if (desktopWidget && !selectImage_.isEmpty()) {
        // 更新 status_config.txt 中的 current_background 行
        QString statusConfigFilePath = QDir::currentPath() + "/resource/status_config.txt";
        QFile statusConfigFile(statusConfigFilePath);

        QStringList lines;
        bool backgroundUpdated = false;

        // 读取现有内容
        if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&statusConfigFile);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("current_background:")) {
                    // 更新 current_background 行
                    lines.append(QString("current_background: %1").arg(selectImage_));
                    backgroundUpdated = true;
                } else {
                    lines.append(line);
                }
            }
            statusConfigFile.close();
        } else {
            qWarning() << "Failed to open status_config.txt for reading:" << statusConfigFilePath;
            return;
        }

        // 如果文件中没有 current_background 行，添加一行
        if (!backgroundUpdated) {
            lines.append(QString("current_background: %1").arg(selectImage_));
        }

        // 写回更新后的内容到 status_config.txt 文件
        if (statusConfigFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&statusConfigFile);
            for (const QString& line : lines) {
                out << line << "\n";
            }
            statusConfigFile.close();
        } else {
            qWarning() << "Failed to open status_config.txt for writing:" << statusConfigFilePath;
            return;
        }

        // 更新桌面背景和图像模式
        desktopWidget->SetfilePath(selectImage_);
//        qDebug() << selectImage_;
        QFileInfo fileInfo(selectImage_);
        QString fileSuffix = fileInfo.suffix().toLower();

        if (videoFormats.contains(fileSuffix)) {
            ui->videoVolumeSlider->show();
            ui->wallpaperMode->hide();
            now_is_video = true;
        } else if (imageFormats.contains(fileSuffix)) {
            ui->videoVolumeSlider->hide();
            ui->wallpaperMode->show();
            now_is_video = false;
        }

        desktopWidget->SetimageMode(imageMode_);
        desktopWidget->UpdateWallpaper();
    }
}



void MainWidget::on_SetDesktop_clicked() {
    changeBG();
}

void MainWidget::closeEvent(QCloseEvent* event)
{
    QMessageBox confirmBox;
    confirmBox.setWindowTitle("");
    confirmBox.setText("是否最小化到托盘?");
    confirmBox.setIcon(QMessageBox::Warning);

    // 添加按钮
    QPushButton *yesButton = confirmBox.addButton(tr("最小化到托盘"), QMessageBox::YesRole);
    confirmBox.addButton(tr("直接关闭"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() == yesButton) {
        hide();
        trayIcon->show();
        event->ignore();
    }
    else {
        trayIcon->hide();
        for (QWidget* widget : widgetsToClose) {
            if (widget&&!widget->isHidden())  // 检查指针有效性并确认未被关闭
            {
                widget->close();
            }
        }

        if (desktopWidget) {
            desktopWidget->restoreWallpaper();
            desktopWidget->close();
            delete desktopWidget;
            desktopWidget = nullptr;
        }
    }
}

// 更改软件背景
void MainWidget::change_engine_BG()
{
    QString resourceDir = "resource/engine_BG";
    QString statusConfigFilePath = "resource/status_config.txt";

    // 打开文件选择对话框
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择背景图片"), resourceDir,
                                                    tr("Image Files (*.png *.jpg *.bmp)"));

    if (!filePath.isEmpty()) {
        // 选择了图片文件
        QPixmap pixmap(filePath);
        if (pixmap.isNull()) {
            QMessageBox::warning(this, tr("错误"), tr("无法加载图片文件：") + filePath);
            return;
        }

        // 将绝对路径转换为相对路径，使得路径以 resource/engine_BG/ 开头
        QString relativePath = QDir(QDir::currentPath()).relativeFilePath(filePath);

        // 更新 status_config.txt 文件中的 current_engine_background 行
        QFile statusConfigFile(statusConfigFilePath);
        if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList lines;
            QTextStream in(&statusConfigFile);
            while (!in.atEnd()) {
                lines.append(in.readLine());
            }
            statusConfigFile.close();

            bool updated = false;
            for (int i = 0; i < lines.size(); ++i) {
                if (lines[i].startsWith("current_engine_background:")) {
                    lines[i] = QString("current_engine_background: %1").arg(relativePath);
                    updated = true;
                    break;
                }
            }

            // 如果文件中没有 current_engine_background 行，添加一行
            if (!updated) {
                lines.append(QString("current_engine_background: %1").arg(relativePath));
            }

            // 写回更新后的内容到 status_config.txt 文件
            if (statusConfigFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&statusConfigFile);
                for (const QString& line : lines) {
                    out << line << "\n";
                }
                statusConfigFile.close();
            } else {
                QMessageBox::warning(this, tr("错误"), tr("无法更新 status_config 文件：") + statusConfigFilePath);
                return;
            }
        } else {
            QMessageBox::warning(this, tr("错误"), tr("无法打开 status_config 文件进行读取：") + statusConfigFilePath);
            return;
        }

        // 更新当前背景图路径
        current_Engine_BG_Path_ = relativePath;
        pre_process_engine_BG();
        // 重绘窗口
        update();
    }
}

// 加载软件背景
void MainWidget::Load_Engine_BG() {
    // 文件路径
    QString statusConfigFilePath = "resource/status_config.txt";
    QString defaultEngineBGPath = ":/resource/background/default_bg.jpg";
    bool pathUpdated = false;

    // 确保资源目录存在
    QDir resourceDir(QDir::currentPath() + "/resource"); // 使用绝对路径
     if (!resourceDir.exists()) {
         if (!resourceDir.mkpath(".")) { // 创建目录
             QMessageBox::warning(this, tr("错误"), tr("无法创建目录：") + resourceDir.absolutePath());
             return;
         }
     }

    // 检查并创建 status_config.txt 文件
    QFile statusConfigFile(statusConfigFilePath);
    if (!statusConfigFile.exists()) {
        if (statusConfigFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&statusConfigFile);
            out << "current_background:\n";
            out << "current_imagemode:\n";
            out << "fill_color:\n";
            out << "current_engine_background:" << defaultEngineBGPath << "\n"; // 写入默认路径
            statusConfigFile.close();
            qDebug() << "status_config.txt created with default values.";
        } else {
            QMessageBox::warning(this, tr("错误"), tr("无法创建 status_config 文件：") + statusConfigFilePath);
            return;
        }
    }

    // 从 status_config.txt 文件中读取 current_engine_background 行
    QString currentEngineBGPath;
    if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&statusConfigFile);
        QStringList lines;
        while (!in.atEnd()) {
            lines.append(in.readLine());
        }
        statusConfigFile.close();

        // 更新配置文件内容
        if (lines.isEmpty()) {
            QMessageBox::warning(this, tr("错误"), tr("status_config 文件为空。"));
            return;
        }

        // 查找并更新 current_engine_background 行
        for (int i = 0; i < lines.size(); ++i) {
            if (lines[i].startsWith("current_engine_background:")) {
                currentEngineBGPath = lines[i].mid(QString("current_engine_background:").length()).trimmed();
                if (currentEngineBGPath.isEmpty()) {
                    currentEngineBGPath = defaultEngineBGPath; // 使用默认路径
                    lines[i] = QString("current_engine_background: %1").arg(currentEngineBGPath);
                    pathUpdated = true;
                }
                break;
            }
        }

        if (!currentEngineBGPath.isEmpty()) {
            // Write updated content back to the file if needed
            if (pathUpdated) {
                if (statusConfigFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&statusConfigFile);
                    for (const QString& line : lines) {
                        out << line << "\n";
                    }
                    statusConfigFile.close();
                } else {
                    QMessageBox::warning(this, tr("错误"), tr("无法更新 status_config 文件：") + statusConfigFilePath);
                    return;
                }
            }
        } else {
            QMessageBox::warning(this, tr("错误"), tr("在 status_config 文件中未找到 current_engine_background 配置。"));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("错误"), tr("无法打开 status_config 文件：") + statusConfigFilePath);
        return;
    }

    current_Engine_BG_Path_ = currentEngineBGPath;

    pre_process_engine_BG();
}

// 载入当前背景图进行预处理，调整大小至合适的值，避免缩放窗口卡顿
void MainWidget::pre_process_engine_BG()
{
    engine_BG_pixmap = QPixmap(current_Engine_BG_Path_);

    if (engine_BG_pixmap.isNull()) {
        qWarning() << "无法加载图片：" << current_Engine_BG_Path_;
        return;
    }

    // 设置宽高限制
    const int maxWidth = 1920;
    const int maxHeight = 1080;

    // 计算新的尺寸
    QSize newSize = engine_BG_pixmap.size();
    if (newSize.width() > maxWidth || newSize.height() > maxHeight) {
        // 计算新的尺寸
        qreal widthRatio = static_cast<qreal>(maxWidth) / newSize.width();
        qreal heightRatio = static_cast<qreal>(maxHeight) / newSize.height();
        qreal scaleRatio = qMax(widthRatio, heightRatio);

        newSize.setWidth(static_cast<int>(newSize.width() * scaleRatio));
        newSize.setHeight(static_cast<int>(newSize.height() * scaleRatio));
    }
//    qDebug()<<newSize.width()<<newSize.height();
    engine_BG_pixmap = engine_BG_pixmap.scaled(newSize, Qt::KeepAspectRatio);
}

// 使得软件背景随主窗口缩放而缩放
//void MainWidget::paintEvent(QPaintEvent* event) {
//    QPainter painter(this);

//    QPixmap scaledPixmap = engine_BG_pixmap.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

//    // 绘制缩放后的图片
//    painter.drawPixmap(this->rect(), scaledPixmap);
//}

void MainWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    // 原始图片大小
    QSize originalSize = engine_BG_pixmap.size();

    // 计算图片绘制的初始位置（将图片放在窗口中心）
    int x = (this->width() - originalSize.width()) / 2;
    int y = (this->height() - originalSize.height()) / 2;

    // 绘制原始大小的图片在固定位置
    painter.drawPixmap(x, y, originalSize.width(), originalSize.height(), engine_BG_pixmap);
    QWidget::paintEvent(event);
}

bool MainWidget::eventFilter(QObject *obj, QEvent *event) // 添加左键点击空白处清除选中壁纸事件
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            QListWidget *listWidget = nullptr;

            if (obj == ui->ImagelistWidget->viewport())
            {
                listWidget = ui->ImagelistWidget;
            }
            else if (obj == ui->FavoritelistWidget->viewport())
            {
                listWidget = ui->FavoritelistWidget;
            }

            if (listWidget)
            {
                QListWidgetItem *item = listWidget->itemAt(mouseEvent->pos());
                if (!item)
                {
                    listWidget->clearSelection();
                    previewImage(nullptr); // 清除预览显示
                    return false;
                }
            }
        }
    }

    // 父类处理其余的事件
    return QWidget::eventFilter(obj, event);
}


void MainWidget::setupTabWidget()
{
    QTabWidget *tabWidget = ui->tabWidget;

    if (tabWidget) {
        // 设置标签页可关闭
        tabWidget->setTabsClosable(true);
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (i < 3) {
                tabWidget->tabBar()->setTabButton(i, QTabBar::RightSide, nullptr);
            }
        }
        connect(tabWidget, &QTabWidget::tabCloseRequested, [this, tabWidget](int index) {
            if (index < 3) {
                return;
            }
            QWidget *tabItem = tabWidget->widget(index);
            tabWidget->removeTab(index);
            tabItem->deleteLater();
        });

        QWidget *thirdPage = tabWidget->widget(2);

        if (thirdPage) {
            QLineEdit *urlInput = thirdPage->findChild<QLineEdit *>("urlInput");
            urlInput->setText("https://wallhaven.cc"); // 设置预留网址
            QPushButton *openUrlBTN = thirdPage->findChild<QPushButton *>("openUrlBTN");

            if (urlInput && openUrlBTN) {
                EnterKeyEventFilter *filter = new EnterKeyEventFilter(this);

                // 确保第三页有焦点策略，允许接收键盘事件
                thirdPage->setFocusPolicy(Qt::StrongFocus);

                // 切换到第三页时，设置第三页获取焦点
                connect(tabWidget, &QTabWidget::currentChanged, this, [thirdPage](int index) {
                    if (index == 2) {
                        thirdPage->setFocus();
                    }
                });

                // 安装事件过滤器到第三页
                thirdPage->installEventFilter(filter);

                // 共享的槽函数，用于处理按钮点击和回车按键
                auto openUrl = [=]() {
                    // 获取用户输入的网址
                    QString url = urlInput->text();

                    // 创建新页面
                    QWidget *newPage = new QWidget();
                    QVBoxLayout *newLayout = new QVBoxLayout(newPage);

                    // 创建 WebEngineView 并加载网址
                    MyWebEngineView *webEngineView = new MyWebEngineView(tabWidget, newPage);
                    webEngineView->load(QUrl(url));

                    // 将 WebEngineView 添加到布局中
                    newLayout->addWidget(webEngineView);
                    newPage->setLayout(newLayout);

                    // 添加新标签页并切换到新标签页
                    int index = tabWidget->addTab(newPage, "");
                    connect(webEngineView, &MyWebEngineView::titleChanged, [this, index, tabWidget](const QString &title) {
                        if (index >= 3) {
                            tabWidget->setTabText(index, title);
                        }
                    });
                    tabWidget->setCurrentWidget(newPage);
                };

                // 连接按钮点击信号到槽函数
                connect(openUrlBTN, &QPushButton::clicked, this, openUrl);

                // 连接事件过滤器的 enterKeyPressed 信号到槽函数
                connect(filter, &EnterKeyEventFilter::enterKeyPressed, this, openUrl);
            }
        }
    }
}




void MainWidget::createMenuBar() // 创建菜单栏
{
    menuBar = new QMenuBar(this);


    // 创建菜单
    QMenu *fileMenu = menuBar->addMenu(tr("&    文件    "));
    QMenu *editMenu = menuBar->addMenu(tr("&    功能    "));

    // 添加操作到 File 菜单
    QAction *addBGAction = new QAction(tr("&添加壁纸"), this);
    connect(addBGAction, &QAction::triggered, this, [=]() {
        openBGdir();
    });
    fileMenu->addAction(addBGAction);


    QAction *openBGdirAction = new QAction(tr("&打开图库"), this);
    connect(openBGdirAction, &QAction::triggered, this, [=]() {
        saveImageDir = "resource/saveimage";

        // 检查文件夹是否存在，如果不存在则创建
        QDir dir;
        if (!dir.exists(saveImageDir)) {
            if (!dir.mkpath(saveImageDir)) {
                qDebug() << "Failed to create directory:" << saveImageDir;
                return;
            }
        }
        // 打开目录
        QDesktopServices::openUrl(QUrl::fromLocalFile(saveImageDir));
    });
    fileMenu->addAction(openBGdirAction);

    // 退出程序
    QAction *exitAction = new QAction(tr("&退出程序"), this);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    // 添加操作到 Edit 菜单
    QAction *minAction = new QAction(tr("&最小化到托盘"), this);
    QAction *selfstartAction = new QAction(tr("&开机自启动"), this);
    QAction *changeBGAction = new QAction(tr("&更换软件背景"), this);
    QAction *changeBGcolorAction = new QAction(tr("&更换壁纸填充色"), this);

    editMenu->addAction(minAction);
    editMenu->addAction(selfstartAction);
    editMenu->addAction(changeBGAction);
    editMenu->addAction(changeBGcolorAction);

    connect(minAction, &QAction::triggered, this, &MainWidget::minimizeToTray); // 最小化到托盘
    connect(selfstartAction, &QAction::triggered, this, &MainWidget::setStartup); // 开机自启动
    connect(changeBGAction, &QAction::triggered, this, &MainWidget::change_engine_BG); // 更改软件背景
    connect(changeBGcolorAction, &QAction::triggered, this, &MainWidget::changeWallpaperEmptyAreaColor); // 更换壁纸填充色

}

void MainWidget::changeWallpaperEmptyAreaColor() {
    // 创建一个颜色选择器对话框
    QColor selectedColor = globalColorDialog->getColor();

    if (!now_is_video) {
        if (selectedColor.isValid()) {
            // 设置壁纸填充色
            desktopWidget->wallpaperEmptyAreaColor = selectedColor;
//            qDebug() << "Selected color:" << selectedColor.name();

            // 保存颜色到 status_config.txt 文件
            QString statusConfigFilePath = QDir::currentPath() + "/resource/status_config.txt";
            QFile statusConfigFile(statusConfigFilePath);

            QStringList lines;
            bool updated = false;

            // 读取现有内容
            if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&statusConfigFile);
                while (!in.atEnd()) {
                    lines.append(in.readLine());
                }
                statusConfigFile.close();
            } else {
                qDebug() << "Failed to open status_config.txt for reading:" << statusConfigFilePath;
                return;
            }

            // 更新或添加 fill_color 行
            for (int i = 0; i < lines.size(); ++i) {
                if (lines[i].startsWith("fill_color:")) {
                    lines[i] = QString("fill_color: %1").arg(selectedColor.name());
                    updated = true;
                    break;
                }
            }

            // 如果文件中没有 fill_color 行，添加一行
            if (!updated) {
                lines.append(QString("fill_color: %1").arg(selectedColor.name()));
            }

            // 写回更新后的内容到 status_config.txt 文件
            if (statusConfigFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&statusConfigFile);
                for (const QString& line : lines) {
                    out << line << "\n";
                }
                statusConfigFile.close();
            } else {
                qDebug() << "Failed to open status_config.txt for writing:" << statusConfigFilePath;
                return;
            }

            desktopWidget->UpdateWallpaper();
        }
    }
}

void MainWidget::on_cancelBTN_clicked() {
    if (!desktophide) {
        desktopWidget->resetToDefault();
        desktophide = true;
    }

    // 清除 status_config.txt 文件中 current_background: 的内容
    QString statusConfigFilePath = QDir::currentPath() + "/resource/status_config.txt";
    QFile statusConfigFile(statusConfigFilePath);

    QStringList lines;
    bool backgroundCleared = false;

    // 读取现有内容并查找 current_background 行
    if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&statusConfigFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("current_background:")) {
                lines.append("current_background:");  // 清空背景路径
                backgroundCleared = true;
            } else {
                lines.append(line);
            }
        }
        statusConfigFile.close();
    } else {
        qWarning() << "Failed to open status_config.txt for reading.";
        return;
    }

    // 如果找到了 current_background 行并进行了清空操作，写回文件
    if (backgroundCleared) {
        if (statusConfigFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&statusConfigFile);
            for (const QString& line : lines) {
                out << line << "\n";
            }
            statusConfigFile.close();
        } else {
            qWarning() << "Failed to open status_config.txt for writing.";
        }
    } else {
        qDebug() << "current_background: not found or already empty in status_config.txt";
    }
}


void MainWidget::clearSelectionOnTabChange()
{
    // 清除 ImagelistWidget 的选中项
    ui->ImagelistWidget->clearSelection();

    // 清除 FavoritelistWidget 的选中项
    ui->FavoritelistWidget->clearSelection();
}

void MainWidget::minimizeToTray() { // 最小化到托盘
    hide();
    trayIcon->show();
    //    trayIcon->showMessage(tr("应用程序最小化"), tr("应用程序已最小化到托盘"), QSystemTrayIcon::Information, 1000);
}

void MainWidget::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) { // 从最小化恢复
    if (reason == QSystemTrayIcon::Trigger) {
        show();
        //        trayIcon->show();
    }
}

void MainWidget::setStartup() { // 开机自启动
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString applicationName = QApplication::applicationName();
    // 启动时在路径后面添加 --minimized 参数，指示以最小化启动
    QString applicationPath = QDir::toNativeSeparators(QApplication::applicationFilePath()) + " --minimized";
    if (settings.value(applicationName).toString().isEmpty()) {
        settings.setValue(applicationName, applicationPath);
        QMessageBox InfoBox;
        InfoBox.setWindowTitle(tr("开机自启动"));
        InfoBox.setText(tr("已成功设置开机自启动"));
        InfoBox.setIcon(QMessageBox::Information);
        InfoBox.exec();
    } else {
        settings.remove(applicationName);
        QMessageBox InfoBox;
        InfoBox.setWindowTitle(tr("开机自启动"));
        InfoBox.setText(tr("已取消开机自启动"));
        InfoBox.setIcon(QMessageBox::Information);
        InfoBox.exec();
    }
}

void MainWidget::checkAndLoadBG() {
    QString resourceDir = QDir::currentPath() + "/resource";
    QString statusConfigFilePath = resourceDir + "/status_config.txt";

    // 检查 status_config.txt 文件是否存在
    if (QFile::exists(statusConfigFilePath)) {
        QFile statusConfigFile(statusConfigFilePath);

        // 尝试打开文件进行读取
        if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&statusConfigFile);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed(); // 读取并去除首尾空白
                if (line.startsWith("current_background:")) {
                    QString content = line.mid(QString("current_background:").length()).trimmed();

                    // 检查内容是否为空
                    if (!content.isEmpty()) {
                        selectImage_ = content; // 将路径内容赋值给 selectImage_
                    }
                    break; // 找到后直接跳出循环
                }
            }
            statusConfigFile.close();
        } else {
            qDebug() << "无法打开文件进行读取：" << statusConfigFilePath;
        }
    } else {
        qDebug() << "文件不存在：" << statusConfigFilePath;
    }
    changeBG();
}


void MainWidget::load_ImageMode() // 加载 image_mode
{
    QString statusConfigFilePath = QDir::currentPath() + "/resource/status_config.txt";
    QFile statusConfigFile(statusConfigFilePath);

    bool modeFound = false;
    QString modeLine;

    // 尝试从文件中读取 imageMode 的值
    if (statusConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&statusConfigFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("current_imagemode:")) {
                modeLine = line.mid(QString("current_imagemode:").length()).trimmed();
                modeFound = true;
                break;
            }
        }
        statusConfigFile.close();
    } else {
        qWarning() << "Could not open file for reading:" << statusConfigFilePath;
        return;
    }

    // 解析读取的模式值
    if (modeFound && !modeLine.isEmpty()) {
        bool ok;
        int mode = modeLine.toInt(&ok);
        if (ok) {
            imageMode_ = mode;
        } else {
            qWarning() << "Invalid mode read from file:" << modeLine;
        }
    } else {
        qWarning() << "No valid current_imagemode found in file or file is empty.";
    }

    // 更新桌面控件和 UI
    desktopWidget->SetimageMode(imageMode_);
    ui->wallpaperMode->setCurrentIndex(imageMode_);
}

void MainWidget::ensureStatusConfigFileExists() {
    // 初始化配置文件
    QString resourceDirPath = "resource";
    QString configFilePath = resourceDirPath + "/status_config.txt";
    QString engineBGDirPath = resourceDirPath + "/engine_BG";

    // 检查资源文件夹是否存在
    QDir resourceDir(resourceDirPath);
    if (!resourceDir.exists()) {
        // 如果文件夹不存在，尝试创建
        if (resourceDir.mkpath(".")) {
            qDebug() << "Resource folder created.";
        } else {
            qDebug() << "Failed to create resource folder.";
            return; // 无法创建文件夹，退出函数
        }
    }

    // 检查 engine_BG 文件夹是否存在
    QDir engineBGDir(engineBGDirPath);
    if (!engineBGDir.exists()) {
        // 如果文件夹不存在，尝试创建
        if (engineBGDir.mkpath(".")) {
            qDebug() << "Engine_BG folder created.";
        } else {
            qDebug() << "Failed to create engine_BG folder.";
            return; // 无法创建文件夹，退出函数
        }
    }

    // 检查配置文件是否存在
    QFile configFile(configFilePath);
    if (!configFile.exists()) {
        // 如果文件不存在，尝试创建并写入初始内容
        if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&configFile);
            // 写入初始内容
            out << "current_background:\n";
            out << "current_imagemode:\n";
            out << "fill_color:\n";
            out << "current_engine_background:\n";
            configFile.close();
            qDebug() << "Configuration file created with initial content.";
        } else {
            qDebug() << "Failed to create configuration file.";
        }
    } else {
//        qDebug() << "Configuration file already exists.";
    }
}
