#include "KeyValue.h"

#include <QJsonObject>

QJsonArray KeyValueRow::toJson(const QVector<KeyValueRow> &rows) {
    QJsonArray arr;
    for (const KeyValueRow &r : rows) {
        QJsonObject o;
        o.insert("key", r.key);
        o.insert("value", r.value);
        o.insert("enabled", r.enabled);
        arr.append(o);
    }
    return arr;
}

QVector<KeyValueRow> KeyValueRow::fromJson(const QJsonArray &arr) {
    QVector<KeyValueRow> rows;
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        KeyValueRow r;
        r.key = o.value("key").toString();
        r.value = o.value("value").toString();
        r.enabled = o.value("enabled").toBool(true);
        rows.append(r);
    }
    return rows;
}
