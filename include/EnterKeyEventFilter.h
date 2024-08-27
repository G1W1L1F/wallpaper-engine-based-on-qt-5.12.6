#pragma once
#include <QKeyEvent>

// 事件过滤器类 使得网页页面能够直接按下enter访问网址
class EnterKeyEventFilter : public QObject {
    Q_OBJECT

public:
    EnterKeyEventFilter(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void enterKeyPressed();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                emit enterKeyPressed();
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};
