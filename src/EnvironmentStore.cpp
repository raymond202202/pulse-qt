#include "EnvironmentStore.h"

#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QUuid>

namespace {
const QString kGroup = QStringLiteral("environments");
const QString kDataKey = QStringLiteral("data");
const QString kActiveKey = QStringLiteral("active");

QString newId() {
    return QStringLiteral("env-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}
} // namespace

EnvironmentStore *EnvironmentStore::instance() {
    static EnvironmentStore s;
    return &s;
}

EnvironmentStore::EnvironmentStore(QObject *parent) : QObject(parent) {
    load();
}

void EnvironmentStore::load() {
    QSettings settings;
    settings.beginGroup(kGroup);
    m_environments.clear();
    m_activeId = settings.value(kActiveKey).toString();

    const QJsonDocument doc = QJsonDocument::fromJson(settings.value(kDataKey).toByteArray());
    if (!doc.isArray()) return;
    const QJsonArray arr = doc.array();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        Environment env;
        env.id = o.value("id").toString();
        env.name = o.value("name").toString();
        const QJsonArray vars = o.value("variables").toArray();
        for (const QJsonValue &vv : vars) {
            const QJsonObject vo = vv.toObject();
            EnvVar var;
            var.key = vo.value("key").toString();
            var.value = vo.value("value").toString();
            var.enabled = vo.value("enabled").toBool(true);
            env.variables.append(var);
        }
        m_environments.append(env);
    }
}

void EnvironmentStore::save() {
    QJsonArray arr;
    for (const Environment &env : m_environments) {
        QJsonObject o;
        o.insert("id", env.id);
        o.insert("name", env.name);
        QJsonArray vars;
        for (const EnvVar &v : env.variables) {
            QJsonObject vo;
            vo.insert("key", v.key);
            vo.insert("value", v.value);
            vo.insert("enabled", v.enabled);
            vars.append(vo);
        }
        o.insert("variables", vars);
        arr.append(o);
    }
    QSettings settings;
    settings.beginGroup(kGroup);
    settings.setValue(kDataKey, QJsonDocument(arr).toJson(QJsonDocument::Compact));
    settings.setValue(kActiveKey, m_activeId);
}

const Environment *EnvironmentStore::environmentById(const QString &id) const {
    for (const Environment &env : m_environments)
        if (env.id == id) return &env;
    return nullptr;
}

const Environment *EnvironmentStore::activeEnvironment() const {
    return environmentById(m_activeId);
}

QString EnvironmentStore::resolve(const QString &templateText) const {
    if (templateText.isEmpty() || m_activeId.isEmpty()) return templateText;
    const Environment *env = activeEnvironment();
    if (!env) return templateText;

    static const QRegularExpression re(QStringLiteral(R"(\{\{(\w+)\}\})"));
    QString out;
    out.reserve(templateText.size() + 16);
    int last = 0;
    QRegularExpressionMatchIterator it = re.globalMatch(templateText);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        out += templateText.mid(last, m.capturedStart() - last);
        const QString key = m.captured(1);
        QString replacement = m.captured(0); // 未定义保留 {{key}}
        for (const EnvVar &v : env->variables) {
            if (v.enabled && !v.key.isEmpty() && v.key == key) {
                replacement = v.value;
                break;
            }
        }
        out += replacement;
        last = m.capturedEnd();
    }
    out += templateText.mid(last);
    return out;
}

void EnvironmentStore::addEnvironment(const QString &name) {
    Environment env;
    env.id = newId();
    env.name = name;
    m_environments.append(env);
    save();
    emit changed();
}

void EnvironmentStore::renameEnvironment(const QString &id, const QString &name) {
    for (Environment &env : m_environments) {
        if (env.id == id) {
            env.name = name;
            save();
            emit changed();
            return;
        }
    }
}

void EnvironmentStore::removeEnvironment(const QString &id) {
    for (int i = 0; i < m_environments.size(); ++i) {
        if (m_environments[i].id == id) {
            m_environments.removeAt(i);
            if (m_activeId == id) {
                m_activeId.clear();
                emit activeChanged();
            }
            save();
            emit changed();
            return;
        }
    }
}

void EnvironmentStore::setActiveEnvironment(const QString &id) {
    if (m_activeId == id) return;
    m_activeId = id;
    save();
    emit activeChanged();
}

void EnvironmentStore::addVariable(const QString &envId) {
    for (Environment &env : m_environments) {
        if (env.id == envId) {
            env.variables.append({QString(), QString(), true});
            save();
            emit changed();
            return;
        }
    }
}

void EnvironmentStore::updateVariable(const QString &envId, int index,
                                      const QString &key, const QString &value, bool enabled) {
    for (Environment &env : m_environments) {
        if (env.id == envId && index >= 0 && index < env.variables.size()) {
            env.variables[index] = {key, value, enabled};
            save();
            emit changed();
            return;
        }
    }
}

void EnvironmentStore::removeVariable(const QString &envId, int index) {
    for (Environment &env : m_environments) {
        if (env.id == envId && index >= 0 && index < env.variables.size()) {
            env.variables.removeAt(index);
            save();
            emit changed();
            return;
        }
    }
}
