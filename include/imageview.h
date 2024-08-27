#pragma once

#include <QWidget>
#include <QListWidget>
#include <QRect>

class ImageView  : public QWidget
{
    Q_OBJECT

public:
    ImageView(QWidget* parent = 0);
    ~ImageView();

    void SetImage(const QString& img_path); //从文件路径加载图片
    void ResetTransform(); //重新设置缩放比例
    void resizeEvent(QResizeEvent* event);

private:
    QPixmap pix_ori_; //原始pixmap
    QPixmap pix_display_; //用来展示的pixmap
    QString img_path_; //图片路径
    float zoom_scale_; //鼠标滚轮控制的缩放比例
    float min_zoom_scale_;
    float max_zoom_scale_;
    QPointF move_step_; // 图像左上角坐标
    QPoint mouse_point_; // 鼠标位置
    bool move_start_;
    bool is_moving_;

protected:
    void paintEvent(QPaintEvent* event)override;
    void wheelEvent(QWheelEvent* event)override;
    void mousePressEvent(QMouseEvent* event)override;
    void mouseReleaseEvent(QMouseEvent* event)override;
    void mouseMoveEvent(QMouseEvent* event)override;
};
