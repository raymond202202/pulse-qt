#pragma once

#include <QWidget>
#include <QHash>
#include <QPair>
#include "CollectionStore.h"

class QTreeWidget;
class QTreeWidgetItem;
class QComboBox;

// 左栏：环境选择 + 集合树（集合 → 文件夹 → 请求）
// 点击请求 → requestActivated；右键菜单管理集合/文件夹/请求
class CollectionTree : public QWidget {
    Q_OBJECT
public:
    explicit CollectionTree(QWidget *parent = nullptr);

signals:
    void requestActivated(const QString &method, const QString &url, const QString &body);

private slots:
    void reload();
    void reloadEnvironments();
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onCustomContextMenu(const QPoint &pos);
    void onEnvChanged(int index);
    void manageEnvironments();
    void addCollection();

private:
    void populateChildren(QTreeWidgetItem *parent, const QVector<CollectionItem> &items,
                          const QString &collectionId);
    void showContextMenu(QTreeWidgetItem *item, const QPoint &globalPos);

    // item → (collectionId, itemId)
    QHash<QTreeWidgetItem *, QPair<QString, QString>> m_keyOf;

    QComboBox *m_envCombo = nullptr;
    QTreeWidget *m_tree = nullptr;
};
