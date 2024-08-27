#include "include/ImageGroup.h"
#include <qdebug.h>
#include <qpainter.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <QProcess>
#include <qset.h>
#include <QLabel>
#include <QMovie>
#include <QBitmap>

ImageGroup::ImageGroup(QObject* parent)
    : QObject(parent)
{
    QFile configFile("resource/config.txt");
    if (!configFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qDebug() << "Unable to open config file for reading.";
        qDebug() << "Current Path:" << QDir::currentPath(); // 当前工作目录的路径
        qDebug() << "Current Path:" << QCoreApplication::applicationDirPath(); // 包含应用程序可执行文件的目录路径
        return;
    }
    QTextStream in(&configFile);
    QString line;

    while (in.readLineInto(&line)) {
        QFileInfo checkFile(line);
        if (checkFile.exists() && checkFile.isFile()) {
            // 检查缩略图是否存在
            QString previewPath_jpg = checkFile.absoluteDir().absolutePath() + "/" + checkFile.completeBaseName() + "_preview.jpg"; // 构建缩略图路径
            QString previewPath_gif = checkFile.absoluteDir().absolutePath() + "/" + checkFile.completeBaseName() + "_preview.gif"; // 构建GIF格式缩略图路径
            if (!QFile::exists(previewPath_jpg) && !QFile::exists(previewPath_gif)) {
                // 如果缩略图不存在，则生成
                QString fileSuffix = checkFile.suffix().toLower();
                if(videoFormats.contains(fileSuffix))
                {
                    createGifFromVideo(line, checkFile.baseName(), checkFile.dir().path());//生成gif
                }
                else if(imageFormats.contains(fileSuffix))
                {
                    createThumbnail(line, checkFile.baseName(), checkFile.dir().path());
                }
            }
            new_images_.append(line);
            all_images_.append(line);
        }
    }
    configFile.close();
}

ImageGroup::~ImageGroup()
{
}

// 向图片数组中添加新增图片
bool ImageGroup::addImage(const QStringList& img_paths)
{
    //清空新增图片数组
    new_images_.clear();
    new_images_ = img_paths;
    if (new_images_.empty()) return false;
    //从新添加的图片列表中提出源列表中已经存在的图片路径
    QSet<QString> pathsSet = QSet<QString>::fromList(all_images_); // 将QStringList转为QSet进行高效去重
    QStringList filteredNewPaths;
    for (const QString& path : new_images_) {
        if (!pathsSet.contains(path)) {
            filteredNewPaths.append(path);
//            qDebug() << "Add New Path:" << path;
        }
    }
    new_images_ = filteredNewPaths; // 新加入的图像数组经过过滤去掉全部图片数组中已经包含的文件
    processImages(new_images_);
    //将新增图片添加至全部图片数组
    all_images_.reserve(all_images_.size() + new_images_.size()); // 预先分配足够内存
    all_images_.append(new_images_); // 向全部图像数组追加新加入图像数组
    return true;
}

// 将路径下的图片拷贝到本地文件夹并创建缩略图
void ImageGroup::processImages(QStringList& fileNames) {
    QFile configFile("resource\\config.txt");
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Unable to open config file for writing.";
        qDebug() << "Error:" << configFile.errorString();
        qDebug() << "Current Path:" << QDir::currentPath();
        return;
    }
    QTextStream out(&configFile);

    foreach(const QString & filePath, fileNames) {
        QFileInfo fileInfo(filePath);
        QString baseName = fileInfo.baseName();
        QString dirPath = "resource/data/" + baseName;

        QString fileSuffix = fileInfo.suffix().toLower(); // 后缀名

        // 创建目录
        if (!QDir().mkpath(dirPath)) {  // 使用mkpath创建文件夹
            qDebug() << "Failed to create directory:" << dirPath;
            continue;  // 如果创建失败，则跳过这个文件
        }

        // 复制文件
        QString newFilePath = dirPath + "/" + fileInfo.fileName();
        bool success = QFile::copy(filePath, newFilePath);
        if (!success) {
            continue;  // 如果拷贝失败，则跳过这个文件
        }
        // 创建缩略图
        if(videoFormats.contains(fileSuffix))
        {
            createGifFromVideo(newFilePath, baseName, dirPath);//生成gif
        }
        else if(imageFormats.contains(fileSuffix))
        {
            createThumbnail(newFilePath, baseName, dirPath);
        }
        // 保存图片路径到配置文件
        out << newFilePath << "\n";
    }
    configFile.close();
}

// 通过线程池创建缩略图
void ImageGroup::createThumbnail(const QString& filePath, const QString& baseName, const QString& dirPath) {    
    QtConcurrent::run([=]() { // 异步执行创建缩略图线程
        QImage image(filePath);
        if (image.isNull()) {
            return;  // 如果图片加载失败，返回
        }

        const QSize targetSize(320, 320);
        qreal scale = qMax(
                    static_cast<qreal>(targetSize.width()) / image.width(),
                    static_cast<qreal>(targetSize.height()) / image.height()
                    );

        QImage scaled = image.scaled(
                    image.width() * scale,
                    image.height() * scale,
                    Qt::KeepAspectRatio,//保持图像的宽高比
                    Qt::SmoothTransformation//使用平滑插值算法来进行缩放
                    );

        int x = (scaled.width() - targetSize.width()) / 2;
        int y = (scaled.height() - targetSize.height()) / 2;

        QImage cropped = scaled.copy(x, y, targetSize.width(), targetSize.height());//将缩略图裁剪为320x320尺寸
        cropped.save(dirPath + "/" + baseName + "_preview.jpg");//保存至原图路径下
        emit sendImage(filePath);  // 发出一个名为 sendImage 的信号，并传递一个参数 filePath 确保信号与槽通过 Qt::QueuedConnection 连接
    });
}

// 通过线程池创建缩略GIF
void ImageGroup::createGifFromVideo(const QString& filePath, const QString& baseName, const QString& dirPath) {
    QtConcurrent::run([=]() {
        QProcess probeProcess;
        // 启动 ffprobe 命令
        probeProcess.start("ffprobe", QStringList() << "-v" << "error"
                           << "-select_streams" << "v:0"
                           << "-show_entries" << "stream=width,height,nb_frames"
                           << "-of" << "default=noprint_wrappers=1:nokey=1"
                           << filePath);
        probeProcess.waitForFinished();

        QByteArray probeOutput = probeProcess.readAllStandardOutput();
        QByteArray probeError = probeProcess.readAllStandardError();

        if (probeProcess.exitCode() != 0) {
            qDebug() << "ffprobe failed to analyze video:" << probeError;
            return;
        }

        QStringList probeLines = QString(probeOutput).split("\n");
        if (probeLines.size() < 3) {
            qDebug() << "Failed to retrieve necessary video information.";
            return;
        }

        bool ok;
        int videoWidth = probeLines[0].toInt(&ok); // 字符串转换为整数值
        int videoHeight = probeLines[1].toInt(&ok);
        int frameCount = probeLines[2].toInt(&ok);

        if (!ok) {
            qDebug() << "Invalid video information retrieved.";
            return;
        }

        int squareSize = qMin(videoWidth, videoHeight);
        int maxFrames = qMin(frameCount, 200);

        QString gifPath = dirPath + "/" + baseName + "_temp.gif";
        QString finalGifPath = dirPath + "/" + baseName + "_preview.gif";

        // 生成初始 GIF 文件
        QStringList arguments;
        if(videoWidth>videoHeight)
        {
            arguments << "-i" << filePath
                      << "-vf" << QString("fps=25,scale=-1:%1:flags=lanczos,crop=%2:%2:(in_w-%2)/2:(in_h-%2)/2")
                         .arg(squareSize)
                         .arg(squareSize)
                      << "-frames:v" << QString::number(maxFrames)
                      << "-gifflags" << "+transdiff"
                      << "-y" << gifPath;
        }
        else
        {
            arguments << "-i" << filePath
                      << "-vf" << QString("fps=25,scale=%1:-1:flags=lanczos,crop=%2:%2:(in_w-%2)/2:(in_h-%2)/2")
                         .arg(squareSize)
                         .arg(squareSize)
                      << "-frames:v" << QString::number(maxFrames)
                      << "-gifflags" << "+transdiff"
                      << "-y" << gifPath;
        }

        qDebug() << "FFmpeg command (create initial GIF):" << arguments.join(" ");

        QProcess ffmpegProcess;
        ffmpegProcess.start("ffmpeg", arguments);
        ffmpegProcess.waitForFinished();

        QByteArray ffmpegOutput = ffmpegProcess.readAllStandardOutput();
        QByteArray ffmpegError = ffmpegProcess.readAllStandardError();

        if (ffmpegProcess.exitCode() != 0) {
            qDebug() << "FFmpeg failed to create GIF:" << ffmpegError;
            return;
        }

        // 进一步调整 GIF 大小
        QStringList resizeArguments;
        resizeArguments << "-i" << gifPath
                        << "-vf" << "scale=320:320:force_original_aspect_ratio=decrease,pad=320:320:(ow-iw)/2:(oh-ih)/2"
                        << "-y" << finalGifPath;

        qDebug() << "FFmpeg command (resize GIF):" << resizeArguments.join(" ");

        QProcess resizeProcess;
        resizeProcess.start("ffmpeg", resizeArguments);
        resizeProcess.waitForFinished();

//        QByteArray resizeOutput = resizeProcess.readAllStandardOutput();
        QByteArray resizeError = resizeProcess.readAllStandardError();

        if (resizeProcess.exitCode() != 0) {
            qDebug() << "FFmpeg failed to resize GIF:" << resizeError;
            return;
        }

        // 确保生成的 GIF 文件存在且有效
        if (QFile::exists(finalGifPath)) {
            qDebug() << "GIF created and resized successfully:" << finalGifPath;
            emit sendImage(filePath);
        } else {
            qDebug() << "GIF creation and resizing reported success but file does not exist:" << finalGifPath;
            qDebug() << "FFmpeg output:" << ffmpegOutput;
            qDebug() << "FFmpeg error:" << ffmpegError;
        }

        // 清理临时 GIF 文件
        if (QFile::exists(gifPath)) {
            QFile::remove(gifPath);
        }
    });
}

void ImageGroup::creatPreviewPixmap() // 载入保存的图标
{
    for(QString & filePath : new_images_) {
//        qDebug() << "creatPreviewPixmap singal" << filePath;
        emit sendImage(filePath);
    }
}

void ImageGroup::delete_config_line(const QString& filePath) // 删除目录文件中的文件路径
{
    QFile configFile("resource\\config.txt");

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open config file for reading.";
        return;
    }

    QList<QString> lines;
    while (!configFile.atEnd()) {
        QString line = configFile.readLine().trimmed();
        if (!line.contains(filePath)) {
            lines.append(line);
        }
    }

    configFile.close();

    // 重新打开文件用于写入，清空文件内容
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug() << "Failed to open config file for writing.";
        return;
    }

    // 将保留下来的行写回到文件中
    QTextStream out(&configFile);
    for (const QString& line : lines) {
        out << line << "\n";
    }

    configFile.close();
    qDebug() << "Deleted line with filePath:" << filePath;
}
