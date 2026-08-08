#pragma once

#include <QAbstractItemModel>
#include <QJsonValue>
#include <QHash>
#include <QVector>

// 懒加载 JSON 树模型：QJsonDocument 一次解析，子节点按需创建（fetchMore）
class JsonTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    enum NodeType { Root, Object, Array, String, Number, Boolean, Null, Truncated };
    enum Roles {
        KeyRole = Qt::UserRole + 1,   // 键名（root 为空）
        TypeRole,                     // NodeType
        ValueTextRole,                // 值显示文本（叶子）
        KeyNameRole,                  // 键名（联动定位用，与 KeyRole 相同）
        PathRole,                     // 点分路径（root.a.b.0）
        RawRole,                      // QJsonValue
        ChildCountRole                // 真实子项数（含未加载/截断部分）
    };

    static constexpr int MaxChildren = 1000;

    explicit JsonTreeModel(QObject *parent = nullptr);
    ~JsonTreeModel() override;

    void setJson(const QJsonValue &root);
    void clearData();
    bool hasJson() const { return m_hasJson; }

    // 统计（走 QJsonValue 全量遍历，不依赖模型加载）
    void computeStats();
    int totalNodes() const { return m_totalNodes; }
    int keyCount() const { return m_keyCount; }
    int leafCount() const { return m_leafCount; }
    bool isHAR() const { return m_isHAR; }
    int harRequestCount() const { return m_harRequests; }
    int harDomainCount() const { return m_harDomains; }
    QString harMethodSummary() const { return m_harMethodSummary; }

    // 工具：按路径定位索引（自动触发沿路径的懒加载）
    QModelIndex indexForPath(const QString &path);

    // 紧凑 JSON 字符串转义（供 MainWindow 搜索显示共用）
    static QString escapeJsonString(const QString &s);

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    struct Node {
        quint64 id = 0;
        quint64 parentId = 0;
        QString key;                // 空 = root
        NodeType type = Root;
        QJsonValue value;           // 叶子显示 / 容器解析用
        bool childrenLoaded = true;
        int childCount = 0;         // 真实子节点数（可能大于已建数）
        QString path;
        QVector<quint64> children;  // 已建子节点 id
    };

    quint64 allocNode(quint64 parentId, const QString &key, NodeType type,
                      const QJsonValue &value, const QString &path);
    void loadChildren(quint64 id);
    quint64 idOf(const QModelIndex &index) const
    { return index.isValid() ? quint64(index.internalId()) : m_rootId; }
    Node &nodeRef(quint64 id) const { return m_nodes[id]; }
    QString valueText(const QJsonValue &v) const;

    mutable QHash<quint64, Node> m_nodes;
    quint64 m_rootId = 1;
    quint64 m_nextId = 2;
    bool m_hasJson = false;
    QJsonValue m_rootValue;

    int m_totalNodes = 0, m_keyCount = 0, m_leafCount = 0;
    bool m_isHAR = false;
    int m_harRequests = 0, m_harDomains = 0;
    QString m_harMethodSummary;
};
