#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVector>
#include "KeyValue.h"

// 单条历史记录
struct HistoryEntry {
    qint64 id = 0;
    RequestPayload payload;
    int status = 0;
    qint64 msec = 0;
    QString createdAt; // ISO 8601
};

// 请求历史存储（单例）：SQLite 持久化（~/.local/share/pulse-qt/history.db）
// 发送完成后记录（方法/URL/Body/Headers/Params/状态/耗时/时间），点击回填请求区
class HistoryStore : public QObject {
    Q_OBJECT
public:
    static HistoryStore *instance();

    QVector<HistoryEntry> entries(int limit = 200) const;

public slots:
    void addEntry(const RequestPayload &payload, int status, qint64 msec);
    void removeEntry(qint64 id);
    void clearHistory();

signals:
    void changed();

private:
    explicit HistoryStore(QObject *parent = nullptr);
    bool openDatabase();

    QSqlDatabase m_db;
};
