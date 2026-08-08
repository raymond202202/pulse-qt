#pragma once

#include <QObject>
#include <QVector>

// 环境变量项
struct EnvVar {
    QString key;
    QString value;
    bool enabled = true;
};

// 单个环境
struct Environment {
    QString id;
    QString name;
    QVector<EnvVar> variables;
};

// 环境存储（单例）：多环境管理 + {{var}} 占位符解析
// 持久化：QSettings（~/.config/pulse-qt/pulse-qt.conf）
class EnvironmentStore : public QObject {
    Q_OBJECT
public:
    static EnvironmentStore *instance();

    const QVector<Environment> &environments() const { return m_environments; }
    QString activeEnvironmentId() const { return m_activeId; }
    const Environment *environmentById(const QString &id) const;
    const Environment *activeEnvironment() const;

    // 解析 {{key}} → 当前环境值；未定义/未启用保留原样
    QString resolve(const QString &templateText) const;

public slots:
    void addEnvironment(const QString &name);
    void renameEnvironment(const QString &id, const QString &name);
    void removeEnvironment(const QString &id);
    void setActiveEnvironment(const QString &id); // 空串 = 无环境
    void addVariable(const QString &envId);
    void updateVariable(const QString &envId, int index,
                        const QString &key, const QString &value, bool enabled);
    void removeVariable(const QString &envId, int index);

signals:
    void changed();
    void activeChanged();

private:
    explicit EnvironmentStore(QObject *parent = nullptr);
    void load();
    void save();

    QVector<Environment> m_environments;
    QString m_activeId;
};
