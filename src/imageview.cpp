#include "include/imageview.h"
#include <QEvent>
#include <QApplication>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QWheelEvent>
#include <QDebug>
#include <cmath>

ImageView::ImageView(QWidget* parent)
    : QWidget(parent),
      zoom_scale_(1.0f),
      move_start_(false),
      is_moving_(false),
      min_zoom_scale_(1.0f),
      max_zoom_scale_(8.0f),
      move_step_(0, 0)
{
    this->setWindowTitle("查看图片");
    resize(1200, 800);
    setMinimumSize(600, 400); // 设置窗口最小宽度为600，最小高度为400
}

ImageView::~ImageView()
{
}

void ImageView::SetImage(const QString& img_path) // 加载图像
{
    ResetTransform();
    img_path_ = img_path;
    pix_ori_.load(img_path_);

    // 计算图像缩放后的尺寸
    pix_display_ = pix_ori_.scaled(zoom_scale_ * size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 重新计算move_step_，使图像居中
    move_step_ = QPoint((width() - pix_display_.width()) / 2, (height() - pix_display_.height()) / 2);
    update();
}

void ImageView::ResetTransform()
{
    // 重新设置缩放比例
    zoom_scale_ = 1.0f;
}

void ImageView::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    int offset_x = move_step_.x();
    int offset_y = move_step_.y();

    painter.drawPixmap(offset_x, offset_y, pix_display_); // 使用调整后的move_step_绘制图像
    QWidget::paintEvent(event);
}

void ImageView::wheelEvent(QWheelEvent* event) // 滚轮控制图像大小
{
    // 计算窗口中心的相对位置
    QPointF viewCenter(width() / 2.0, height() / 2.0);

    // 计算视口中心在图像坐标系中的位置
    QPointF imageCenterRelativeToView = (viewCenter - move_step_) / zoom_scale_;

    // 随滚轮缩放
    if (event->delta() > 0)
    {
        zoom_scale_ *= 1.1f;
    }
    else
    {
        zoom_scale_ *= 1/1.1f;
    }

    // 限制缩放比例在最小和最大值之间
    if (zoom_scale_ < min_zoom_scale_)
    {
        zoom_scale_ = min_zoom_scale_;
    }
    if (zoom_scale_ > max_zoom_scale_)
    {
        zoom_scale_ = max_zoom_scale_;
    }

    // 重新计算图像显示尺寸
    QSize newSize = pix_ori_.scaled(zoom_scale_ * size(), Qt::KeepAspectRatio).size();

    // 更新移动位置
    if (zoom_scale_ > 1.0f)
    {
        // 图像开始占满窗口，计算移动位置
        move_step_ = viewCenter - (imageCenterRelativeToView * zoom_scale_);

        // 更新后的图像矩形
        QRect updatedImageRect(QPoint(move_step_.x(), move_step_.y()), newSize);
        QRect windowRect(0, 0, width(), height());

        if ((updatedImageRect.left() > windowRect.left()) || (updatedImageRect.right() < windowRect.right()))
        {
            if((updatedImageRect.left() > windowRect.left()) && (updatedImageRect.right() < windowRect.right()))
            {
                move_step_.setX((width() - newSize.width()) / 2);
            }
            else if(updatedImageRect.left() > windowRect.left())
            {
                move_step_.setX(windowRect.left());
            }
            else if(updatedImageRect.right() < windowRect.right())
            {
                move_step_.setX(windowRect.right() - updatedImageRect.width());
            }
        } // 图像在Y轴未占满窗口，控制图像Y轴居中
        if ((updatedImageRect.top() > windowRect.top()) || (updatedImageRect.bottom() < windowRect.bottom()))
        {
            if ((updatedImageRect.top() > windowRect.top()) && (updatedImageRect.bottom() < windowRect.bottom()))
            {
                move_step_.setY((height() - newSize.height()) / 2);
            }
            else if(updatedImageRect.top() > windowRect.top())
            {
                move_step_.setY(windowRect.top());
            }
            else if(updatedImageRect.bottom() < windowRect.bottom())
            {
                move_step_.setY(windowRect.bottom() - updatedImageRect.height());
            }
        } // 图像在Y轴未占满窗口，控制图像Y轴居中
    }
    else
    {
        // 图像X,Y轴均未占满窗口时，保持居中
        move_step_ = QPoint((width() - newSize.width()) / 2, (height() - newSize.height()) / 2);
    }

    // 更新图像显示
    pix_display_ = pix_ori_.scaled(zoom_scale_ * size(), Qt::KeepAspectRatio);
    update();
}

void ImageView::mousePressEvent(QMouseEvent* event) // 鼠标拖动图片
{
    if (event->button() == Qt::LeftButton)
    {
        if (zoom_scale_ > 1.0f) // 只有在图像放大时允许拖动
        {
            move_start_ = true;
            is_moving_ = false;
            mouse_point_ = event->globalPos();
        }
    }
}

void ImageView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (move_start_)
        {
            move_start_ = false;
            is_moving_ = false;
        }
    }
}

void ImageView::mouseMoveEvent(QMouseEvent* event)
{
    if (move_start_)
    {
        const QPoint mos_pt = event->globalPos();
        move_step_ += mos_pt - mouse_point_;
        is_moving_ = true;
        mouse_point_ = mos_pt;

        // 计算图像实际在窗口中的边界
        QRect imageRect(QPoint(move_step_.x(), move_step_.y()), pix_display_.size());
        QRect windowRect(0, 0, width(), height());


        // 限制图像边界在窗口内
        if (zoom_scale_ > 1.0f) // 只有在图像放大时允许拖动
        {
            if(
                    ((imageRect.left() < windowRect.left())&&(imageRect.top() < windowRect.top()))||
                    ((imageRect.right() > windowRect.right())&&(imageRect.bottom() > windowRect.bottom()))||
                    ((imageRect.left() < windowRect.left())&&(imageRect.bottom() > windowRect.bottom()))||
                    ((imageRect.right() > windowRect.right())&&(imageRect.top() < windowRect.top())))
            {
                if (imageRect.top() > windowRect.top()) // 控制图片四个方向边界不在拖动过程超过窗口
                    move_step_.setY(windowRect.top());

                if (imageRect.bottom() < windowRect.bottom())
                    move_step_.setY(windowRect.bottom() - pix_display_.height());

                if (imageRect.left() > windowRect.left())
                    move_step_.setX(windowRect.left());

                if (imageRect.right() < windowRect.right())
                    move_step_.setX(windowRect.right() - pix_display_.width());
            } // 图像占满窗口，允许随意拖动
            else
            {
                if ((imageRect.left() > windowRect.left())&&(imageRect.right() < windowRect.right()))
                {
                    move_step_.setX((width() - pix_display_.width()) / 2);
                    if (imageRect.top() > windowRect.top())
                        move_step_.setY(windowRect.top());

                    if (imageRect.bottom() < windowRect.bottom())
                        move_step_.setY(windowRect.bottom() - pix_display_.height());
                } // 图像在X轴未占满窗口，控制图像X轴居中
                if((imageRect.top() > windowRect.top())&&(imageRect.bottom() < windowRect.bottom()))
                {
                    move_step_.setY((height() - pix_display_.height()) / 2);
                    if (imageRect.left() > windowRect.left())
                        move_step_.setX(windowRect.left());

                    if (imageRect.right() < windowRect.right())
                        move_step_.setX(windowRect.right() - pix_display_.width());
                } // 图像在Y轴未占满窗口，控制图像Y轴居中
            }
        }
        else
        {
            // 如果图像未放大，始终保持居中
            move_step_ = QPoint((width() - pix_display_.width()) / 2, (height() - pix_display_.height()) / 2);
        }

        repaint();
    }
}

void ImageView::resizeEvent(QResizeEvent* event)
{
    QSize newSize = pix_ori_.scaled(zoom_scale_ * size(), Qt::KeepAspectRatio, Qt::SmoothTransformation).size();
    pix_display_ = pix_ori_.scaled(zoom_scale_ * size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 确保图像在窗口中居中
    if (zoom_scale_ <= 1.0f)
    {
        move_step_ = QPoint((width() - newSize.width()) / 2, (height() - newSize.height()) / 2);
    }

    update();
    QWidget::resizeEvent(event);
}

