#pragma once

#include <QWidget>
#include "KeyValue.h"

class QListWidget;
class QListWidgetItem;

// 历史记录列表：显示 方法/URL/状态/时间，点击回填请求区，右键删除单条
class HistoryList : public QWidget {
    Q_OBJECT
public:
    explicit HistoryList(QWidget *parent = nullptr);

signals:
    void requestActivated(const RequestPayload &req);

private slots:
    void reload();
    void onItemClicked(QListWidgetItem *item);
    void onCustomContextMenu(const QPoint &pos);
    void clearAll();

private:
    QListWidget *m_list = nullptr;
};
