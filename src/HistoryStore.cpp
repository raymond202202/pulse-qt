#include "HistoryStore.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDebug>

namespace {
constexpr int kMaxEntries = 500; // 保留上限
}

HistoryStore *HistoryStore::instance() {
    static HistoryStore s;
    return &s;
}

HistoryStore::HistoryStore(QObject *parent) : QObject(parent) {
    openDatabase();
}

bool HistoryStore::openDatabase() {
    if (m_db.isOpen()) return true;
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("history"));
    m_db.setDatabaseName(dir + QStringLiteral("/history.db"));
    if (!m_db.open()) {
        qWarning() << "history db open failed:" << m_db.lastError().text();
        return false;
    }
    QSqlQuery q(m_db);
    q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS history ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " method TEXT NOT NULL,"
        " url TEXT NOT NULL,"
        " body TEXT DEFAULT '',"
        " status INTEGER DEFAULT 0,"
        " msec INTEGER DEFAULT 0,"
        " created_at TEXT NOT NULL)"));
    return true;
}

QVector<HistoryEntry> HistoryStore::entries(int limit) const {
    QVector<HistoryEntry> out;
    if (!m_db.isOpen()) return out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT id, method, url, body, status, msec, created_at"
        " FROM history ORDER BY id DESC LIMIT ?"));
    q.addBindValue(limit);
    if (!q.exec()) return out;
    while (q.next()) {
        HistoryEntry e;
        e.id = q.value(0).toLongLong();
        e.method = q.value(1).toString();
        e.url = q.value(2).toString();
        e.body = q.value(3).toString();
        e.status = q.value(4).toInt();
        e.msec = q.value(5).toLongLong();
        e.createdAt = q.value(6).toString();
        out.append(e);
    }
    return out;
}

void HistoryStore::addEntry(const QString &method, const QString &url, const QString &body,
                            int status, qint64 msec) {
    if (!m_db.isOpen()) return;
    if (url.trimmed().isEmpty()) return;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO history (method, url, body, status, msec, created_at)"
        " VALUES (?, ?, ?, ?, ?, ?)"));
    q.addBindValue(method.toUpper());
    q.addBindValue(url);
    q.addBindValue(body);
    q.addBindValue(status);
    q.addBindValue(msec);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.exec();

    // 裁剪超过上限的旧记录
    QSqlQuery prune(m_db);
    prune.prepare(QStringLiteral("DELETE FROM history WHERE id NOT IN"
                                 " (SELECT id FROM history ORDER BY id DESC LIMIT ?)"));
    prune.addBindValue(kMaxEntries);
    prune.exec();

    emit changed();
}

void HistoryStore::removeEntry(qint64 id) {
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM history WHERE id = ?"));
    q.addBindValue(id);
    q.exec();
    emit changed();
}

void HistoryStore::clearHistory() {
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("DELETE FROM history"));
    emit changed();
}
