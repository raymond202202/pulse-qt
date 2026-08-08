#pragma once

#include <QString>
#include <QVector>
#include <QJsonArray>

// Key/Value 行（Headers / Params 共用，enabled 控制是否生效）
struct KeyValueRow {
    QString key;
    QString value;
    bool enabled = true;

    static QJsonArray toJson(const QVector<KeyValueRow> &rows);
    static QVector<KeyValueRow> fromJson(const QJsonArray &arr);
};

// 请求载荷快照（集合/历史/回填共用）
struct RequestPayload {
    QString method;
    QString url;
    QString body;
    QVector<KeyValueRow> headers;
    QVector<KeyValueRow> params;
};

Q_DECLARE_METATYPE(RequestPayload)
