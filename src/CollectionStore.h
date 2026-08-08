#pragma once

#include <QObject>
#include <QVector>
#include "KeyValue.h"

// 集合项：文件夹（isFolder=true，children 递归）或请求（payload）
struct CollectionItem {
    QString id;
    QString name;
    bool isFolder = false;
    RequestPayload payload;
    QVector<CollectionItem> children;
};

// 集合：顶层容器，items 可含文件夹/请求
struct Collection {
    QString id;
    QString name;
    QVector<CollectionItem> items;
};

// 集合存储（单例）：集合 → 文件夹 → 请求 树，QSettings 持久化
class CollectionStore : public QObject {
    Q_OBJECT
public:
    static CollectionStore *instance();

    const QVector<Collection> &collections() const { return m_collections; }
    Collection *collectionById(const QString &id);
    const CollectionItem *findItem(const QString &collectionId, const QString &itemId) const;

public slots:
    void addCollection(const QString &name);
    void renameCollection(const QString &id, const QString &name);
    void removeCollection(const QString &id);
    // parentItemId 为空 = 集合顶层；返回新项 id（失败返回空串）
    QString addFolder(const QString &collectionId, const QString &parentItemId, const QString &name);
    QString addRequest(const QString &collectionId, const QString &parentItemId,
                       const QString &name, const RequestPayload &req);
    void removeItem(const QString &collectionId, const QString &itemId);
    void renameItem(const QString &collectionId, const QString &itemId, const QString &name);
    // 全局按 itemId 定位所属 (collectionId, parentItemId)
    bool findItemLocation(const QString &itemId,
                          QString *collectionIdOut, QString *parentItemIdOut) const;
    // 整包更新请求字段（AI 工具用）
    void updateRequest(const QString &collectionId, const QString &itemId,
                       const RequestPayload &payload);

signals:
    void changed();

private:
    explicit CollectionStore(QObject *parent = nullptr);
    void load();
    void save();
    static QJsonObject itemToJson(const CollectionItem &item);
    static CollectionItem itemFromJson(const QJsonObject &o);

    QVector<Collection> m_collections;
};
