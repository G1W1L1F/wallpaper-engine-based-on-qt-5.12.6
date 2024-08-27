#include "include/sliderfilter.h"
#include <QMouseEvent>

SliderFilter::SliderFilter(QSlider *slider, QObject *parent)
    : QObject(parent), slider(slider)
{
    if (slider) {
        slider->installEventFilter(this);  // 安装事件过滤器
    }
}

bool SliderFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == slider) { // 检查事件是否来自滑条
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);  // 将 QEvent 转换为 QMouseEvent
            if (mouseEvent->button() == Qt::LeftButton) {
                int sliderPos;
                int sliderRange;

                if (slider->orientation() == Qt::Horizontal) {  // 如果是水平滑条
                    sliderPos = mouseEvent->x();  // 获取点击的 X 坐标
                    sliderRange = slider->width();
                } else {  // 如果是竖直滑条
                    sliderPos = slider->height() - mouseEvent->y();  // 获取点击的 Y 坐标，并反转
                    sliderRange = slider->height();
                }

                int newValue = slider->minimum() + (sliderPos * (slider->maximum() - slider->minimum())) / sliderRange;
                slider->setValue(newValue);  // 更新滑条的值

                emit sliderClicked(newValue);  // 发射信号
                return true;  // 表示事件已被处理
            }
        }
    }

    // 继续处理其他事件
    return QObject::eventFilter(obj, event);
}
