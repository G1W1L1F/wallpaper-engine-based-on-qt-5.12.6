#pragma once
#include <qvector.h>
#include <qstringlist.h>
#include <qpixmap.h>
#include <qlistWidget.h>
#include "include/listwidgetitem.h"
class ImageGroup : public QObject
{
    Q_OBJECT
public:
    ImageGroup(QObject* parent = nullptr);
    ~ImageGroup();
    QStringList all_images_; //所有图片数组
    const QStringList& GetAllImage() { return all_images_; }
    const QStringList& GetNewImage() { return new_images_; }
    bool addImage(const QStringList& img_names); // 添加新增图片数组
    void delete_config_line(const QString& filePath); // 删除config.txt文件中对应的文件路径

private:
    QStringList videoFormats = {"mp4", "avi", "mov", "mkv", "flv", "wmv"};
    QStringList imageFormats = { "jpg", "jpeg", "png", "bmp", "gif" }; // 支持的文件格式
    QStringList new_images_; //新增图片数组

    void processImages(QStringList& fileNames); // 将添加的图片拷贝到本地文件夹并创建缩略图
    void createThumbnail(const QString& filePath, const QString& baseName, const QString& dirPath); // 创建缩略图
    void createGifFromVideo(const QString& filePath, const QString& baseName, const QString& dirPath); // 创建缩略GIF

signals:
    void sendImage(QString filePath); // 通知主程序将图标加入界面

public slots:
    void creatPreviewPixmap(); // 载入保存的图标
};

