#include "JsonTreeModel.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

JsonTreeModel::JsonTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

JsonTreeModel::~JsonTreeModel() = default;

void JsonTreeModel::setJson(const QJsonValue &root)
{
    beginResetModel();
    m_nodes.clear();
    m_nextId = 2;
    m_hasJson = true;
    m_rootValue = root;

    Node rootNode;
    rootNode.id = m_rootId;
    rootNode.parentId = 0;
    rootNode.key = QString();
    rootNode.type = Root;
    rootNode.value = root;
    rootNode.path = QString();
    rootNode.childCount = 0;
    rootNode.childrenLoaded = false;
    if (root.isObject()) rootNode.childCount = root.toObject().size();
    else if (root.isArray()) rootNode.childCount = root.toArray().size();
    m_nodes[m_rootId] = rootNode;

    // 根节点立即加载第一层，保证顶层可见
    loadChildren(m_rootId);
    computeStats();
    endResetModel();
}

void JsonTreeModel::clearData()
{
    beginResetModel();
    m_nodes.clear();
    m_nextId = 2;
    m_hasJson = false;
    m_rootValue = QJsonValue();
    m_totalNodes = m_keyCount = m_leafCount = 0;
    m_isHAR = false;
    m_harRequests = m_harDomains = 0;
    m_harMethodSummary.clear();
    endResetModel();
}

void JsonTreeModel::computeStats()
{
    m_totalNodes = 1;
    m_keyCount = 0;
    m_leafCount = 0;
    m_isHAR = false;
    m_harRequests = 0;
    m_harDomains = 0;
    m_harMethodSummary.clear();
    if (!m_hasJson) return;

    QHash<QString, int> methods;
    QSet<QString> domains;

    std::function<void(const QJsonValue &, bool)> walk = [&](const QJsonValue &v, bool isRoot) {
        if (v.isObject()) {
            const QJsonObject o = v.toObject();
            for (auto it = o.begin(); it != o.end(); ++it) {
                m_keyCount++;
                const QJsonValue child = it.value();
                if (child.isObject() || child.isArray()) {
                    m_totalNodes++;
                    walk(child, false);
                } else {
                    m_leafCount++;
                    m_totalNodes++;
                }
            }
            // HAR 检测：根节点 log.entries 数组
            if (isRoot && o.contains("log")) {
                const QJsonValue log = o.value("log");
                if (log.isObject() && log.toObject().value("entries").isArray()) {
                    const QJsonArray entries = log.toObject().value("entries").toArray();
                    m_isHAR = true;
                    m_harRequests = entries.size();
                    for (const QJsonValue &e : entries) {
                        const QJsonObject entry = e.toObject();
                        const QJsonObject req = entry.value("request").toObject();
                        const QString m = req.value("method").toString("UNKNOWN");
                        methods[m]++;
                        const QString url = req.value("url").toString();
                        if (!url.isEmpty()) {
                            const QUrl u(url);
                            if (u.isValid() && !u.host().isEmpty()) domains.insert(u.host());
                        }
                    }
                    QStringList parts;
                    for (auto it = methods.begin(); it != methods.end(); ++it)
                        parts << QString("%1×%2").arg(it.key()).arg(it.value());
                    m_harMethodSummary = parts.join(" · ");
                    m_harDomains = domains.size();
                }
            }
        } else if (v.isArray()) {
            const QJsonArray a = v.toArray();
            for (const QJsonValue &child : a) {
                if (child.isObject() || child.isArray()) {
                    m_totalNodes++;
                    walk(child, false);
                } else {
                    m_leafCount++;
                    m_totalNodes++;
                }
            }
        }
    };
    walk(m_rootValue, true);
}

QString JsonTreeModel::valueText(const QJsonValue &v) const
{
    switch (v.type()) {
    case QJsonValue::String:
        return escapeJsonString(v.toString());
    case QJsonValue::Double:
        return QString::number(v.toDouble(), 'g', 17);
    case QJsonValue::Bool:
        return v.toBool() ? "true" : "false";
    case QJsonValue::Null:
        return "null";
    default:
        return QString();
    }
}

// 紧凑 JSON 字符串转义（比 QJsonDocument 包装序列化更快，避免临时对象）
QString JsonTreeModel::escapeJsonString(const QString &s)
{
    QString out;
    out.reserve(s.size() + 8);
    out += '"';
    for (const QChar &c : s) {
        switch (c.unicode()) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c.unicode() < 0x20)
                out += QString("\\u%1").arg(int(c.unicode()), 4, 16, QLatin1Char('0'));
            else
                out += c;
        }
    }
    out += '"';
    return out;
}

quint64 JsonTreeModel::allocNode(quint64 parentId, const QString &key, NodeType type,
                                 const QJsonValue &value, const QString &path)
{
    Node n;
    n.id = m_nextId++;
    n.parentId = parentId;
    n.key = key;
    n.type = type;
    n.value = value;
    n.path = path;
    n.childCount = 0;
    n.childrenLoaded = true;
    if (type == Object) n.childCount = value.toObject().size();
    else if (type == Array) n.childCount = value.toArray().size();
    if ((type == Object || type == Array) && n.childCount > 0) n.childrenLoaded = false;
    m_nodes.insert(n.id, n);
    return n.id;
}

void JsonTreeModel::loadChildren(quint64 id)
{
    // 注意：不要持有 Node& 跨 m_nodes.insert() —— insert 可能触发 rehash 使引用失效
    if (m_nodes.value(id).childrenLoaded) return;

    const Node src = m_nodes.value(id);   // 拷贝，后续 insert 不影响
    QVector<quint64> ids;
    const auto childType = [](const QJsonValue &v) -> NodeType {
        if (v.isObject()) return Object;
        if (v.isArray()) return Array;
        if (v.isString()) return String;
        if (v.isDouble()) return Number;
        if (v.isBool()) return Boolean;
        return Null;
    };
    const auto childPath = [](const QString &parentPath, const QString &key) {
        return parentPath.isEmpty() ? key : parentPath + "." + key;
    };

    if (src.type == Object || (src.type == Root && src.value.isObject())) {
        const QJsonObject o = src.value.toObject();
        QStringList keys = o.keys();
        const int shown = qMin(keys.size(), MaxChildren);
        for (int i = 0; i < shown; ++i) {
            const QString &k = keys[i];
            ids.append(allocNode(id, k, childType(o.value(k)), o.value(k), childPath(src.path, k)));
        }
        if (keys.size() > MaxChildren) {
            const QString more = QString("… 还有 %1 项已隐藏（共 %2 项）")
                                     .arg(keys.size() - MaxChildren).arg(keys.size());
            ids.append(allocNode(id, more, Truncated, QJsonValue(), childPath(src.path, more)));
        }
    } else if (src.type == Array || (src.type == Root && src.value.isArray())) {
        const QJsonArray a = src.value.toArray();
        const int shown = qMin(a.size(), MaxChildren);
        for (int i = 0; i < shown; ++i) {
            const QJsonValue v = a.at(i);
            ids.append(allocNode(id, QString::number(i), childType(v), v, childPath(src.path, QString::number(i))));
        }
        if (a.size() > MaxChildren) {
            const QString more = QString("… 还有 %1 项已隐藏（共 %2 项）")
                                     .arg(a.size() - MaxChildren).arg(a.size());
            ids.append(allocNode(id, more, Truncated, QJsonValue(), childPath(src.path, more)));
        }
    }

    m_nodes[id].children = ids;   // 重新获取引用，写入最终结果
    m_nodes[id].childrenLoaded = true;  // 全部构建成功后才置位
}

QModelIndex JsonTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    const quint64 pid = idOf(parent);
    if (!m_nodes.contains(pid)) return QModelIndex();
    const Node &p = m_nodes.value(pid);
    if (row >= p.children.size()) return QModelIndex();
    return createIndex(row, column, quintptr(p.children.at(row)));
}

QModelIndex JsonTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return QModelIndex();
    const quint64 cid = quint64(child.internalId());
    if (!m_nodes.contains(cid)) return QModelIndex();
    const quint64 pid = m_nodes.value(cid).parentId;
    if (pid == 0 || pid == m_rootId) return QModelIndex();
    if (!m_nodes.contains(pid)) return QModelIndex();
    const Node &p = m_nodes.value(pid);
    const quint64 gid = p.parentId;
    if (!m_nodes.contains(gid)) return QModelIndex();
    const Node &g = m_nodes.value(gid);
    const int row = g.children.indexOf(pid);
    if (row < 0) return QModelIndex();
    return createIndex(row, 0, quintptr(pid));
}

int JsonTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    const quint64 pid = idOf(parent);
    if (!m_nodes.contains(pid)) return 0;
    return m_nodes.value(pid).children.size();
}

int JsonTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant JsonTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const quint64 cid = quint64(index.internalId());
    if (!m_nodes.contains(cid)) return QVariant();
    const Node &n = m_nodes.value(cid);

    switch (role) {
    case Qt::DisplayRole: {
        QString keyPart;
        if (!n.key.isEmpty() && n.type != Truncated)
            keyPart = "\"" + n.key + "\": ";
        QString valPart = valueText(n.value);
        if (n.type == Object || n.type == Array) {
            valPart = (n.type == Object ? "{" : "[") + QString(" %1 项 ").arg(n.childCount)
                    + (n.type == Object ? "}" : "]");
        }
        if (n.type == Truncated) return n.key;
        return keyPart + valPart;
    }
    case KeyRole:
    case KeyNameRole:
        return n.key;
    case TypeRole:
        return int(n.type);
    case ValueTextRole:
        return valueText(n.value);
    case PathRole:
        return n.path;
    case RawRole:
        return QVariant::fromValue(n.value);
    case ChildCountRole:
        return n.childCount;
    default:
        return QVariant();
    }
}

bool JsonTreeModel::hasChildren(const QModelIndex &parent) const
{
    const quint64 pid = idOf(parent);
    if (!m_nodes.contains(pid)) return false;
    const Node &n = m_nodes.value(pid);
    const bool container = n.type == Object || n.type == Array || n.type == Root;
    return container && n.childCount > 0;
}

bool JsonTreeModel::canFetchMore(const QModelIndex &parent) const
{
    const quint64 pid = idOf(parent);
    if (!m_nodes.contains(pid)) return false;
    const Node &n = m_nodes.value(pid);
    const bool container = n.type == Object || n.type == Array || n.type == Root;
    return container && !n.childrenLoaded && n.childCount > 0;
}

void JsonTreeModel::fetchMore(const QModelIndex &parent)
{
    const quint64 pid = idOf(parent);
    if (!m_nodes.contains(pid)) return;
    if (m_nodes.value(pid).childrenLoaded) return;
    const int old = m_nodes.value(pid).children.size();
    loadChildren(pid);
    const int added = m_nodes.value(pid).children.size() - old;
    if (added > 0) {
        beginInsertRows(parent, old, old + added - 1);
        endInsertRows();
    }
}

Qt::ItemFlags JsonTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex JsonTreeModel::indexForPath(const QString &path)
{
    if (path.isEmpty()) return QModelIndex();
    const QStringList parts = path.split('.');
    QModelIndex parent;
    for (const QString &part : parts) {
        if (canFetchMore(parent)) fetchMore(parent);
        const int rows = rowCount(parent);
        bool found = false;
        for (int r = 0; r < rows; ++r) {
            const QModelIndex idx = index(r, 0, parent);
            if (data(idx, KeyRole).toString() == part) {
                parent = idx;
                found = true;
                break;
            }
        }
        if (!found) return QModelIndex();
    }
    return parent;
}
