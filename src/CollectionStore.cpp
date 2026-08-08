#include "CollectionStore.h"

#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUuid>

namespace {
const QString kGroup = QStringLiteral("collections");
const QString kDataKey = QStringLiteral("data");

QString newId() {
    return QStringLiteral("col-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}
} // namespace

CollectionStore *CollectionStore::instance() {
    static CollectionStore s;
    return &s;
}

CollectionStore::CollectionStore(QObject *parent) : QObject(parent) {
    load();
}

QJsonObject CollectionStore::itemToJson(const CollectionItem &item) {
    QJsonObject o;
    o.insert("id", item.id);
    o.insert("name", item.name);
    o.insert("isFolder", item.isFolder);
    if (!item.isFolder) {
        o.insert("method", item.method);
        o.insert("url", item.url);
        o.insert("body", item.body);
    }
    QJsonArray children;
    for (const CollectionItem &c : item.children) children.append(itemToJson(c));
    o.insert("children", children);
    return o;
}

CollectionItem CollectionStore::itemFromJson(const QJsonObject &o) {
    CollectionItem item;
    item.id = o.value("id").toString();
    item.name = o.value("name").toString();
    item.isFolder = o.value("isFolder").toBool(false);
    if (!item.isFolder) {
        item.method = o.value("method").toString(QStringLiteral("GET"));
        item.url = o.value("url").toString();
        item.body = o.value("body").toString();
    }
    const QJsonArray children = o.value("children").toArray();
    for (const QJsonValue &v : children) item.children.append(itemFromJson(v.toObject()));
    return item;
}

void CollectionStore::load() {
    QSettings settings;
    settings.beginGroup(kGroup);
    m_collections.clear();
    const QJsonDocument doc = QJsonDocument::fromJson(settings.value(kDataKey).toByteArray());
    if (!doc.isArray()) return;
    for (const QJsonValue &v : doc.array()) {
        const QJsonObject o = v.toObject();
        Collection col;
        col.id = o.value("id").toString();
        col.name = o.value("name").toString();
        const QJsonArray items = o.value("items").toArray();
        for (const QJsonValue &iv : items) col.items.append(itemFromJson(iv.toObject()));
        m_collections.append(col);
    }
}

void CollectionStore::save() {
    QJsonArray arr;
    for (const Collection &col : m_collections) {
        QJsonObject o;
        o.insert("id", col.id);
        o.insert("name", col.name);
        QJsonArray items;
        for (const CollectionItem &it : col.items) items.append(itemToJson(it));
        o.insert("items", items);
        arr.append(o);
    }
    QSettings settings;
    settings.beginGroup(kGroup);
    settings.setValue(kDataKey, QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

Collection *CollectionStore::collectionById(const QString &id) {
    for (Collection &col : m_collections)
        if (col.id == id) return &col;
    return nullptr;
}

namespace {
// 递归查找：items 里找 itemId；返回 true 表示找到
bool findInItems(const QVector<CollectionItem> &items, const QString &itemId,
                 const CollectionItem **out) {
    for (const CollectionItem &it : items) {
        if (it.id == itemId) {
            if (out) *out = &it;
            return true;
        }
        if (findInItems(it.children, itemId, out)) return true;
    }
    return false;
}
} // namespace

const CollectionItem *CollectionStore::findItem(const QString &collectionId,
                                                const QString &itemId) const {
    for (const Collection &col : m_collections) {
        if (col.id != collectionId) continue;
        const CollectionItem *out = nullptr;
        findInItems(col.items, itemId, &out);
        return out;
    }
    return nullptr;
}

void CollectionStore::addCollection(const QString &name) {
    Collection col;
    col.id = newId();
    col.name = name;
    m_collections.append(col);
    save();
    emit changed();
}

void CollectionStore::renameCollection(const QString &id, const QString &name) {
    for (Collection &col : m_collections) {
        if (col.id == id) {
            col.name = name;
            save();
            emit changed();
            return;
        }
    }
}

void CollectionStore::removeCollection(const QString &id) {
    for (int i = 0; i < m_collections.size(); ++i) {
        if (m_collections[i].id == id) {
            m_collections.removeAt(i);
            save();
            emit changed();
            return;
        }
    }
}

namespace {
// 递归向 parentId 指向的文件夹插入 item；返回是否成功
bool insertIntoItems(QVector<CollectionItem> &items, const QString &parentId,
                     const CollectionItem &item) {
    for (CollectionItem &it : items) {
        if (it.id == parentId && it.isFolder) {
            it.children.append(item);
            return true;
        }
        if (insertIntoItems(it.children, parentId, item)) return true;
    }
    return false;
}

// 递归删除 itemId；返回是否成功
bool eraseFromItems(QVector<CollectionItem> &items, const QString &itemId) {
    for (int i = 0; i < items.size(); ++i) {
        if (items[i].id == itemId) {
            items.removeAt(i);
            return true;
        }
        if (eraseFromItems(items[i].children, itemId)) return true;
    }
    return false;
}

// 递归重命名 itemId；返回是否成功
bool renameInItems(QVector<CollectionItem> &items, const QString &itemId, const QString &name) {
    for (CollectionItem &it : items) {
        if (it.id == itemId) {
            it.name = name;
            return true;
        }
        if (renameInItems(it.children, itemId, name)) return true;
    }
    return false;
}
} // namespace

QString CollectionStore::addFolder(const QString &collectionId, const QString &parentItemId,
                                   const QString &name) {
    Collection *col = collectionById(collectionId);
    if (!col) return QString();
    CollectionItem folder;
    folder.id = newId();
    folder.name = name;
    folder.isFolder = true;

    bool ok = false;
    if (parentItemId.isEmpty()) {
        col->items.append(folder);
        ok = true;
    } else {
        ok = insertIntoItems(col->items, parentItemId, folder);
    }
    if (!ok) return QString();
    save();
    emit changed();
    return folder.id;
}

QString CollectionStore::addRequest(const QString &collectionId, const QString &parentItemId,
                                    const QString &name, const RequestData &req) {
    Collection *col = collectionById(collectionId);
    if (!col) return QString();
    CollectionItem item;
    item.id = newId();
    item.name = name;
    item.isFolder = false;
    item.method = req.method;
    item.url = req.url;
    item.body = req.body;

    bool ok = false;
    if (parentItemId.isEmpty()) {
        col->items.append(item);
        ok = true;
    } else {
        ok = insertIntoItems(col->items, parentItemId, item);
    }
    if (!ok) return QString();
    save();
    emit changed();
    return item.id;
}

void CollectionStore::removeItem(const QString &collectionId, const QString &itemId) {
    Collection *col = collectionById(collectionId);
    if (!col) return;
    if (!eraseFromItems(col->items, itemId)) return;
    save();
    emit changed();
}

void CollectionStore::renameItem(const QString &collectionId, const QString &itemId,
                                 const QString &name) {
    Collection *col = collectionById(collectionId);
    if (!col) return;
    if (!renameInItems(col->items, itemId, name)) return;
    save();
    emit changed();
}
