#include "include/listwidgetitem.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QPixmap>

ListWidgetItem::ListWidgetItem(const QString& filePath, QWidget* parent)
    : QWidget(parent), filePath(filePath)  // 初始化filePath
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(5);
    layout->setSpacing(5);
    label = new QLabel;
    //    label->setStyleSheet("border-radius: 0px; border: 0px solid #333333");

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    bool isAnimated = (suffix == "gif");
    if (isAnimated) {
        movie = new QMovie(filePath);
        if (movie->isValid()) {
            label->setMovie(movie);
            movie->start();
        } else {
            QPixmap pixmap(filePath);
            label->setPixmap(pixmap);
        }
    } else {
        QPixmap pixmap(filePath);
        label->setPixmap(pixmap);
    }

    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label->setScaledContents(true);
    label->setFixedSize(195, 195);

    layout->addWidget(label);
    layout->setAlignment(Qt::AlignCenter);
    this->setLayout(layout);
}

ListWidgetItem::~ListWidgetItem() {
    if(label->movie())
    {
        QMovie* currentMovie = label->movie();
        label->setMovie(nullptr);  // 将 QLabel 的 movie 设为 nullptr
        currentMovie->stop();  // 停止当前的 QMovie
        delete currentMovie;   // 删除当前的 QMovie
    }
    QLayout* layout = this->layout();
    if (layout) {
        delete layout;  // 删除 layout 会自动删除其管理的所有小部件
    }

    delete label;  // 删除 label 对象
}

QString ListWidgetItem::getFilePath() const {
    return filePath;  // 返回文件路径
}
