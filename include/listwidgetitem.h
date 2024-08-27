#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QLabel>
#include <QMovie>
#include <QFileDialog>
#include <QDebug>

class ListWidgetItem : public QWidget
{
    Q_OBJECT

public:
    ListWidgetItem(const QString& filePath, QWidget* parent = nullptr);

    ~ListWidgetItem();

    QString getFilePath() const; // 获取文件路径

private:
    QLabel* label;       // 用于显示图片
    QMovie* movie;       // 用于处理动画
    QString filePath;    // 存储文件路径
};
