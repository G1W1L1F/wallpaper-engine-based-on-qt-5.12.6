#pragma once

#include <QObject>
#include <QSlider>

class SliderFilter : public QObject
{
    Q_OBJECT

public:
    SliderFilter(QSlider *slider, QObject *parent = nullptr); // 重写SliderFilter，使其能够直接跳转到鼠标点击位置

signals:
    void sliderClicked(int value);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;  //事件过滤器

private:
    QSlider *slider;
};
