#include "AiPanel.h"
#include "FlareServer.h"
#include "RequestPanel.h"
#include "ResponsePanel.h"
#include "CollectionStore.h"
#include "EnvironmentStore.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QTextCursor>
#include <functional>

namespace {

QString jsonText(const QJsonValue &v) {
    QJsonDocument doc;
    if (v.isObject()) doc = QJsonDocument(v.toObject());
    else if (v.isArray()) doc = QJsonDocument(v.toArray());
    else doc = QJsonDocument(QJsonArray{v});
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

} // namespace

AiPanel::AiPanel(RequestPanel *request, ResponsePanel *response, QWidget *parent)
    : QWidget(parent), m_request(request), m_response(response) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // ── 顶部：状态 + 快捷操作 ──
    auto *topRow = new QHBoxLayout;
    m_statusLabel = new QLabel(QStringLiteral("flare: 连接中…"), this);
    auto *explainBtn = new QPushButton(QStringLiteral("解释报错"), this);
    auto *genReqBtn = new QPushButton(QStringLiteral("生成请求"), this);
    explainBtn->setObjectName("copyBtn");
    genReqBtn->setObjectName("copyBtn");
    topRow->addWidget(m_statusLabel, 1);
    topRow->addWidget(explainBtn);
    topRow->addWidget(genReqBtn);
    layout->addLayout(topRow);

    // ── 会话日志 ──
    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    layout->addWidget(m_log, 1);

    // ── 输入行 ──
    auto *inputRow = new QHBoxLayout;
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(QStringLiteral("问网络专家…（可要求它发请求、分析报错、生成请求）"));
    m_sendBtn = new QPushButton(QStringLiteral("发送"), this);
    m_sendBtn->setObjectName("sendButton");
    m_sendBtn->setEnabled(false); // 等 flare 连接后再启用
    m_stopBtn = new QPushButton(QStringLiteral("停止"), this);
    m_stopBtn->setObjectName("copyBtn");
    m_stopBtn->setEnabled(false);
    m_newSessionBtn = new QPushButton(QStringLiteral("新会话"), this);
    m_newSessionBtn->setObjectName("copyBtn");
    inputRow->addWidget(m_input, 1);
    inputRow->addWidget(m_sendBtn);
    inputRow->addWidget(m_stopBtn);
    inputRow->addWidget(m_newSessionBtn);
    layout->addLayout(inputRow);

    connect(m_sendBtn, &QPushButton::clicked, this, &AiPanel::sendMessage);
    connect(m_input, &QLineEdit::returnPressed, this, &AiPanel::sendMessage);
    connect(m_stopBtn, &QPushButton::clicked, this, &AiPanel::stopGenerating);
    connect(m_newSessionBtn, &QPushButton::clicked, this, &AiPanel::newSession);
    connect(explainBtn, &QPushButton::clicked, this, [this]() {
        sendPreset(QStringLiteral("请分析当前请求的响应/报错，给出排查建议"));
    });
    connect(genReqBtn, &QPushButton::clicked, this, [this]() {
        sendPreset(QStringLiteral("请帮我生成一个 API 请求示例（告诉我接口用途即可）"));
    });

    connect(FlareServer::instance(), &FlareServer::lineReceived, this, &AiPanel::onLine);
    connect(FlareServer::instance(), &FlareServer::stateChanged, this, &AiPanel::onServerState);
    connect(FlareServer::instance(), &FlareServer::errorOccurred, this, &AiPanel::onServerError);

    m_nam = new QNetworkAccessManager(this);
    m_sessionId = QStringLiteral("pulse-qt-%1").arg(
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));

    // 启动 flare 引擎
    FlareServer::instance()->start();
    if (FlareServer::instance()->isRunning()) {
        m_statusLabel->setText(QStringLiteral("flare: 已连接"));
        m_sendBtn->setEnabled(true);
    } else if (!FlareServer::instance()->flareBin().isEmpty()) {
        m_statusLabel->setText(QStringLiteral("flare: 启动中…"));
    } else {
        m_statusLabel->setText(QStringLiteral("flare: 未找到引擎"));
    }
    if (!FlareServer::hasApiKey()) {
        appendLog(QStringLiteral("⚠ 未检测到 API 密钥。请配置环境变量（DEEPSEEK_API_KEY / OPENAI_API_KEY）"
                                 "或 ~/.flare/.env 后重启应用。密钥由 flare 自行读取，应用内不保存。"));
    }
}

void AiPanel::appendLog(const QString &text) {
    m_log->appendPlainText(text);
    m_log->moveCursor(QTextCursor::End);
}

void AiPanel::sendMessage() {
    const QString text = m_input->text().trimmed();
    if (text.isEmpty() || m_generating) return;
    m_input->clear();
    runChat(text);
}

void AiPanel::sendPreset(const QString &prompt) {
    if (m_generating) return;
    runChat(prompt);
}

void AiPanel::runChat(const QString &input) {
    if (!FlareServer::instance()->isRunning()) {
        appendLog(QStringLiteral("⚠ flare 引擎未连接，无法发送。"));
        return;
    }
    m_generating = true;
    m_sendBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    appendLog(QStringLiteral("🧑 %1").arg(input));
    FlareServer::instance()->sendChat(
        m_sessionId, input, buildContext(), toolDefinitions());
}

void AiPanel::stopGenerating() {
    if (!m_generating) return;
    FlareServer::instance()->sendCancel(m_sessionId);
}

void AiPanel::newSession() {
    if (m_generating) FlareServer::instance()->sendCancel(m_sessionId);
    m_sessionId = QStringLiteral("pulse-qt-%1").arg(
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    m_log->clear();
    appendLog(QStringLiteral("── 新会话 ──"));
}

void AiPanel::onServerState(bool running, const QString &status) {
    m_statusLabel->setText(running ? QStringLiteral("flare: 已连接")
                                   : QStringLiteral("flare: %1").arg(status));
    if (running) m_sendBtn->setEnabled(true);
}

void AiPanel::onServerError(const QString &message) {
    appendLog(QStringLiteral("⚠ %1").arg(message));
}

// ── 事件流处理 ──
void AiPanel::onLine(const QJsonObject &obj) {
    const QString type = obj.value("type").toString();
    if (type == QStringLiteral("text")) {
        appendLog(QStringLiteral("🤖 %1").arg(obj.value("content").toString()));
    } else if (type == QStringLiteral("tool_call")) {
        appendLog(QStringLiteral("🔧 调用工具: %1 %2")
                      .arg(obj.value("name").toString())
                      .arg(jsonText(obj.value("args"))));
    } else if (type == QStringLiteral("tool_execute")) {
        const QString id = obj.value("id").toString();
        const QString name = obj.value("name").toString();
        const QJsonObject args = obj.value("args").toObject();
        appendLog(QStringLiteral("⚙ 执行工具: %1").arg(name));
        executeTool(id, name, args);
    } else if (type == QStringLiteral("tool_result")) {
        appendLog(QStringLiteral("📄 工具结果: %1").arg(
            QString::fromUtf8(obj.value("content").toString().left(300).toUtf8())));
    } else if (type == QStringLiteral("done") || type == QStringLiteral("cancelled")) {
        if (m_generating) {
            m_generating = false;
            m_sendBtn->setEnabled(true);
            m_stopBtn->setEnabled(false);
            if (type == QStringLiteral("cancelled"))
                appendLog(QStringLiteral("⏹ 已停止"));
            else
                appendLog(QString());
        }
    } else if (type == QStringLiteral("error")) {
        appendLog(QStringLiteral("⚠ %1").arg(obj.value("message").toString()));
        m_generating = false;
        m_sendBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
    }
}

// ── 上下文快照（AI 能"看到"应用状态）──
QString AiPanel::buildContext() const {
    QStringList parts;

    const QVector<Collection> &cols = CollectionStore::instance()->collections();
    if (!cols.isEmpty()) {
        QStringList lines;
        lines << QStringLiteral("【集合与请求】");
        for (const Collection &c : cols) {
            lines << QStringLiteral("- %1 (id: %2)").arg(c.name).arg(c.id);
            for (const CollectionItem &it : c.items) {
                if (it.isFolder) {
                    lines << QStringLiteral("  [文件夹] %1 (id: %2)").arg(it.name).arg(it.id);
                    for (const CollectionItem &sub : it.children)
                        lines << QStringLiteral("    - %1").arg(summarizeItem(sub));
                } else {
                    lines << QStringLiteral("  - %1").arg(summarizeItem(it));
                }
            }
        }
        parts << lines.join(QLatin1Char('\n'));
    }

    const QVector<Environment> &envs = EnvironmentStore::instance()->environments();
    if (!envs.isEmpty()) {
        QStringList lines;
        lines << QStringLiteral("【环境变量】");
        const QString active = EnvironmentStore::instance()->activeEnvironmentId();
        for (const Environment &e : envs) {
            QStringList vars;
            for (const EnvVar &v : e.variables)
                if (v.enabled && !v.key.isEmpty()) vars << QStringLiteral("%1=%2").arg(v.key).arg(v.value);
            lines << QStringLiteral("- %1%2: %3")
                         .arg(e.name)
                         .arg(e.id == active ? QStringLiteral("（当前活动）") : QString())
                         .arg(vars.isEmpty() ? QStringLiteral("（空）") : vars.join(QStringLiteral(", ")));
        }
        parts << lines.join(QLatin1Char('\n'));
    }

    const RequestPayload p = m_request->payload();
    if (!p.url.isEmpty()) {
        QStringList lines;
        lines << QStringLiteral("【当前活动请求】");
        lines << QStringLiteral("  %1 %2").arg(p.method).arg(p.url);
        if (!p.body.isEmpty()) lines << QStringLiteral("  Body: %1").arg(p.body.left(300));
        if (m_response->hasResponse()) {
            lines << QStringLiteral("【当前请求响应】");
            lines << QStringLiteral("  HTTP %1（%2 ms，%3 字节）")
                         .arg(m_response->lastStatus())
                         .arg(m_response->lastMsec())
                         .arg(m_response->lastBody().size());
            lines << QStringLiteral("  Body 预览: %1")
                         .arg(QString::fromUtf8(m_response->lastBody().left(1500)));
        }
        parts << lines.join(QLatin1Char('\n'));
    }

    return parts.join(QStringLiteral("\n\n"));
}

QString AiPanel::summarizeItem(const CollectionItem &item) {
    if (item.isFolder) return QStringLiteral("[文件夹] %1 (id: %2)").arg(item.name).arg(item.id);
    return QStringLiteral("%1 %2 — %3 (id: %4)")
               .arg(item.payload.method).arg(item.payload.url)
               .arg(item.name).arg(item.id);
}

QJsonObject AiPanel::summaryJson(const RequestPayload &p) {
    QJsonObject o;
    o.insert("method", p.method);
    o.insert("url", p.url);
    o.insert("body", p.body);
    QJsonArray headers;
    for (const KeyValueRow &r : p.headers)
        if (r.enabled && !r.key.isEmpty()) headers.append(QJsonObject{{"key", r.key}, {"value", r.value}});
    QJsonArray params;
    for (const KeyValueRow &r : p.params)
        if (r.enabled && !r.key.isEmpty()) params.append(QJsonObject{{"key", r.key}, {"value", r.value}});
    o.insert("headers", headers);
    o.insert("params", params);
    return o;
}

// ── 工具定义 ──
QJsonArray AiPanel::toolDefinitions() const {
    const auto func = [](const QString &name, const QString &desc,
                         const QJsonObject &props, const QStringList &required) {
        QJsonObject fn;
        fn.insert("name", name);
        fn.insert("description", desc);
        QJsonObject params;
        params.insert("type", QStringLiteral("object"));
        params.insert("properties", props);
        params.insert("required", QJsonArray::fromStringList(required));
        fn.insert("parameters", params);
        QJsonObject def;
        def.insert("type", QStringLiteral("function"));
        def.insert("function", fn);
        return def;
    };
    const auto strProp = [](const QString &desc) {
        return QJsonObject{{"type", QStringLiteral("string")}, {"description", desc}};
    };

    QJsonArray tools;
    tools.append(func(QStringLiteral("http_request"),
        QStringLiteral("发送 HTTP 请求（GET/POST/PUT/DELETE 等），支持自定义 headers 和 body。用于网络请求调试、API 联调。"),
        QJsonObject{
            {"url", strProp(QStringLiteral("请求 URL（只支持 http/https）"))},
            {"method", strProp(QStringLiteral("请求方法，默认 GET"))},
            {"headers", QJsonObject{{"type", QStringLiteral("object")}, {"description", QStringLiteral("请求头 {key: value}")}}},
            {"body", strProp(QStringLiteral("请求体（POST/PUT 用）"))},
            {"timeout", QJsonObject{{"type", QStringLiteral("number")}, {"description", QStringLiteral("超时毫秒，默认 30000")}}},
        },
        {QStringLiteral("url")}));
    tools.append(func(QStringLiteral("url_parse"),
        QStringLiteral("解析 URL 的协议/主机/路径/查询参数等结构。"),
        QJsonObject{{"url", strProp(QStringLiteral("要解析的 URL"))}},
        {QStringLiteral("url")}));
    tools.append(func(QStringLiteral("response_analyze"),
        QStringLiteral("获取当前活动请求的响应结果（状态/耗时/大小/body）或报错信息，用于分析。无参数。"),
        QJsonObject{}, {}));
    tools.append(func(QStringLiteral("pulse_list_collections"),
        QStringLiteral("列出 Pulse 中所有集合、文件夹与请求（树形，含 id）。无参数。"),
        QJsonObject{}, {}));
    tools.append(func(QStringLiteral("pulse_get_request"),
        QStringLiteral("获取集合中某个请求的完整详情。需要 requestId。"),
        QJsonObject{{"requestId", strProp(QStringLiteral("请求 ID"))}},
        {QStringLiteral("requestId")}));
    tools.append(func(QStringLiteral("pulse_get_active_request"),
        QStringLiteral("获取请求区当前编辑的请求（method/url/body/headers/params）。无参数。"),
        QJsonObject{}, {}));
    tools.append(func(QStringLiteral("pulse_get_active_response"),
        QStringLiteral("获取请求区当前请求的响应结果（status/耗时/body）或报错信息。无参数。"),
        QJsonObject{}, {}));
    tools.append(func(QStringLiteral("pulse_create_request"),
        QStringLiteral("在指定集合/文件夹下创建请求。需要 collectionName（或文件夹名）与 url。"),
        QJsonObject{
            {"collectionName", strProp(QStringLiteral("目标集合名称或 ID"))},
            {"folderName", strProp(QStringLiteral("可选：目标文件夹名称或 ID"))},
            {"name", strProp(QStringLiteral("请求名称"))},
            {"method", strProp(QStringLiteral("HTTP 方法，默认 GET"))},
            {"url", strProp(QStringLiteral("请求 URL"))},
            {"headers", QJsonObject{{"type", QStringLiteral("object")}, {"description", QStringLiteral("请求头 {key: value}")}}},
            {"body", strProp(QStringLiteral("请求体原始文本"))},
        },
        {QStringLiteral("collectionName"), QStringLiteral("name"), QStringLiteral("url")}));
    tools.append(func(QStringLiteral("pulse_update_request"),
        QStringLiteral("更新集合中某个请求的字段（name/method/url/headers/body）。需要 requestId。"),
        QJsonObject{
            {"requestId", strProp(QStringLiteral("请求 ID"))},
            {"name", strProp(QStringLiteral("新名称"))},
            {"method", strProp(QStringLiteral("HTTP 方法"))},
            {"url", strProp(QStringLiteral("请求 URL"))},
            {"headers", QJsonObject{{"type", QStringLiteral("object")}, {"description", QStringLiteral("请求头 {key: value}")}}},
            {"body", strProp(QStringLiteral("请求体原始文本"))},
        },
        {QStringLiteral("requestId")}));
    tools.append(func(QStringLiteral("pulse_delete_request"),
        QStringLiteral("从集合中删除一个请求。需要 requestId。"),
        QJsonObject{{"requestId", strProp(QStringLiteral("请求 ID"))}},
        {QStringLiteral("requestId")}));
    tools.append(func(QStringLiteral("pulse_list_environments"),
        QStringLiteral("列出所有环境及其变量（key/value/enabled）。无参数。"),
        QJsonObject{}, {}));
    tools.append(func(QStringLiteral("pulse_set_env_variable"),
        QStringLiteral("设置环境变量。envName 为环境名称（不填则用当前活动环境）。"),
        QJsonObject{
            {"envName", strProp(QStringLiteral("环境名称（可选）"))},
            {"key", strProp(QStringLiteral("变量名"))},
            {"value", strProp(QStringLiteral("变量值"))},
        },
        {QStringLiteral("key"), QStringLiteral("value")}));
    return tools;
}

// ── 工具执行入口 ──
void AiPanel::executeTool(const QString &id, const QString &name, const QJsonObject &args) {
    if (name == QStringLiteral("http_request")) {
        httpRequestAsync(args, [this, id](bool ok, QString output, QString error) {
            finishTool(id, ok, output, error);
        });
        return;
    }
    QJsonObject result;
    if (name == QStringLiteral("url_parse")) {
        const QUrl u(args.value("url").toString());
        result = QJsonObject{
            {"success", u.isValid()},
            {"output", u.isValid()
                ? QStringLiteral("协议: %1\n主机: %2\n端口: %3\n路径: %4\n查询: %5")
                      .arg(u.scheme()).arg(u.host()).arg(u.port())
                      .arg(u.path()).arg(u.query())
                : QStringLiteral("无效 URL: %1").arg(args.value("url").toString())},
        };
    } else if (name == QStringLiteral("response_analyze")) {
        result = pulseGetActiveResponse();
    } else if (name == QStringLiteral("pulse_list_collections")) {
        result = pulseListCollections();
    } else if (name == QStringLiteral("pulse_get_request")) {
        result = pulseGetRequest(args.value("requestId").toString());
    } else if (name == QStringLiteral("pulse_get_active_request")) {
        result = pulseGetActiveRequest();
    } else if (name == QStringLiteral("pulse_get_active_response")) {
        result = pulseGetActiveResponse();
    } else if (name == QStringLiteral("pulse_create_request")) {
        result = pulseCreateRequest(args);
    } else if (name == QStringLiteral("pulse_update_request")) {
        result = pulseUpdateRequest(args);
    } else if (name == QStringLiteral("pulse_delete_request")) {
        result = pulseDeleteRequest(args.value("requestId").toString());
    } else if (name == QStringLiteral("pulse_list_environments")) {
        result = pulseListEnvironments();
    } else if (name == QStringLiteral("pulse_set_env_variable")) {
        result = pulseSetEnvVariable(args);
    } else {
        finishTool(id, false, QString(), QStringLiteral("未知工具: %1").arg(name));
        return;
    }
    finishTool(id, result.value("success").toBool(),
               result.value("output").toString(),
               result.value("error").toString());
}

void AiPanel::finishTool(const QString &id, bool success, const QString &output,
                         const QString &error) {
    QJsonObject result;
    result.insert("success", success);
    result.insert("output", output);
    if (!error.isEmpty()) result.insert("error", error);
    FlareServer::instance()->sendToolResult(id, result);
}

// ── http_request（异步）──
void AiPanel::httpRequestAsync(const QJsonObject &args,
                               std::function<void(bool, QString, QString)> done) {
    const QString rawUrl = args.value("url").toString();
    const QUrl url(rawUrl);
    if (!url.isValid()
        || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        done(false, QString(), QStringLiteral("无效或不允许的 URL: %1（只支持 http/https）").arg(rawUrl));
        return;
    }
    const QString method = args.value("method").toString(QStringLiteral("GET")).toUpper();
    const QByteArray body = args.value("body").toString().toUtf8();

    QNetworkRequest req(url);
    const QJsonObject headers = args.value("headers").toObject();
    for (auto it = headers.begin(); it != headers.end(); ++it)
        req.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    const int timeoutMs = qBound(1000, args.value("timeout").toInt(30000), 60000);
    req.setTransferTimeout(timeoutMs);

    QElapsedTimer timer;
    timer.start();
    QNetworkReply *reply = m_nam->sendCustomRequest(req, method.toUtf8(), body);
    m_pendingHttp.insert(reply, done);
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, timeoutMs]() {
        const auto cb = m_pendingHttp.take(reply);
        if (!cb) return;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray data = reply->readAll();
        const qint64 elapsed = timer.elapsed();
        reply->deleteLater();
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            cb(false, QString(), QStringLiteral("请求超时（>%1ms）").arg(timeoutMs));
            return;
        }
        if (reply->error() != QNetworkReply::NoError && status == 0) {
            cb(false, QString(), QStringLiteral("请求失败: %1").arg(reply->errorString()));
            return;
        }
        QStringList headLines;
        const auto rawHeaders = reply->rawHeaderPairs();
        for (const auto &h : rawHeaders)
            headLines << QStringLiteral("  %1: %2").arg(QString::fromUtf8(h.first), QString::fromUtf8(h.second));
        const QString bodyPreview = QString::fromUtf8(data.left(3000));
        const QString out = QStringLiteral("状态码: %1\n耗时: %2ms\n响应头:\n%3\n响应体(%4字符):\n%5")
                                .arg(status)
                                .arg(elapsed)
                                .arg(headLines.isEmpty() ? QStringLiteral("  （无）") : headLines.join(QLatin1Char('\n')))
                                .arg(data.size())
                                .arg(bodyPreview);
        cb(true, out, QString());
    });
}

// ── pulse_* 工具实现 ──
QJsonObject AiPanel::pulseListCollections() const {
    QStringList lines;
    lines << QStringLiteral("【集合】");
    const QVector<Collection> &cols = CollectionStore::instance()->collections();
    if (cols.isEmpty()) {
        return QJsonObject{{"success", true}, {"output", QStringLiteral("（暂无集合）")}};
    }
    for (const Collection &c : cols) {
        lines << QStringLiteral("- %1 (id: %2)").arg(c.name).arg(c.id);
        for (const CollectionItem &it : c.items)
            lines << QStringLiteral("    ") + summarizeItem(it);
    }
    return QJsonObject{{"success", true}, {"output", lines.join(QLatin1Char('\n'))}};
}

QJsonObject AiPanel::pulseGetRequest(const QString &requestId) const {
    QString colId;
    if (!CollectionStore::instance()->findItemLocation(requestId, &colId, nullptr))
        return QJsonObject{{"success", false},
                           {"output", QStringLiteral("未找到请求 %1（先 pulse_list_collections 查 ID）").arg(requestId)}};
    const CollectionItem *item = CollectionStore::instance()->findItem(colId, requestId);
    if (!item || item->isFolder)
        return QJsonObject{{"success", false}, {"output", QStringLiteral("未找到请求 %1").arg(requestId)}};
    return QJsonObject{{"success", true},
                       {"output", QStringLiteral("请求 %1（%2）:\n%3")
                             .arg(item->name, requestId, jsonText(summaryJson(item->payload)))}};
}

QJsonObject AiPanel::pulseGetActiveRequest() const {
    const RequestPayload p = m_request->payload();
    if (p.url.isEmpty())
        return QJsonObject{{"success", false}, {"output", QStringLiteral("当前没有活动的请求")}};
    return QJsonObject{{"success", true}, {"output", jsonText(summaryJson(p))}};
}

QJsonObject AiPanel::pulseGetActiveResponse() const {
    if (!m_response->hasResponse()) {
        return QJsonObject{{"success", false},
                           {"output", QStringLiteral("当前请求还没有响应（先发送请求）")}};
    }
    QJsonObject o;
    o.insert("success", true);
    o.insert("status", m_response->lastStatus());
    o.insert("timeMs", m_response->lastMsec());
    o.insert("sizeBytes", m_response->lastBody().size());
    o.insert("body", QString::fromUtf8(m_response->lastBody().left(3000)));
    const RequestPayload p = m_request->payload();
    o.insert("request", QJsonObject{{"method", p.method}, {"url", p.url}});
    return QJsonObject{{"success", true}, {"output", jsonText(o)}};
}

QJsonObject AiPanel::pulseCreateRequest(const QJsonObject &args) const {
    const QString colName = args.value("collectionName").toString();
    const QVector<Collection> &cols = CollectionStore::instance()->collections();
    const Collection *col = nullptr;
    for (const Collection &c : cols)
        if (c.name == colName || c.id == colName) { col = &c; break; }
    if (!col)
        return QJsonObject{{"success", false},
                           {"output", QStringLiteral("未找到集合 %1（先 pulse_list_collections 查看）").arg(colName)}};

    RequestPayload p;
    p.method = args.value("method").toString(QStringLiteral("GET")).toUpper();
    p.url = args.value("url").toString();
    p.body = args.value("body").toString();
    const QJsonObject headers = args.value("headers").toObject();
    for (auto it = headers.begin(); it != headers.end(); ++it)
        p.headers.append({it.key(), it.value().toString(), true});

    // 目标文件夹：folderName 匹配集合内文件夹
    QString parentId;
    if (args.contains(QStringLiteral("folderName"))) {
        const QString folderName = args.value("folderName").toString();
        std::function<const CollectionItem *(const QVector<CollectionItem> &)> findFolder =
            [&](const QVector<CollectionItem> &items) -> const CollectionItem * {
            for (const CollectionItem &it : items) {
                if (it.isFolder && (it.name == folderName || it.id == folderName)) return &it;
                if (const CollectionItem *sub = findFolder(it.children)) return sub;
            }
            return nullptr;
        };
        if (const CollectionItem *folder = findFolder(col->items)) parentId = folder->id;
    }

    const QString newId = CollectionStore::instance()->addRequest(
        col->id, parentId, args.value("name").toString(QStringLiteral("新建请求")), p);
    return QJsonObject{{"success", true},
                       {"output", QStringLiteral("已创建请求 %1 (id: %2)").arg(args.value("name").toString(QStringLiteral("新建请求")), newId)},
                       {"requestId", newId}};
}

QJsonObject AiPanel::pulseUpdateRequest(const QJsonObject &args) const {
    const QString requestId = args.value("requestId").toString();
    QString colId;
    if (!CollectionStore::instance()->findItemLocation(requestId, &colId, nullptr))
        return QJsonObject{{"success", false}, {"output", QStringLiteral("未找到请求 %1").arg(requestId)}};
    const CollectionItem *item = CollectionStore::instance()->findItem(colId, requestId);
    if (!item || item->isFolder)
        return QJsonObject{{"success", false}, {"output", QStringLiteral("未找到请求 %1").arg(requestId)}};

    RequestPayload p = item->payload;
    const QString oldName = item->name; // 在修改前读取，避免悬垂
    if (args.contains(QStringLiteral("method"))) p.method = args.value("method").toString().toUpper();
    if (args.contains(QStringLiteral("url"))) p.url = args.value("url").toString();
    if (args.contains(QStringLiteral("body"))) p.body = args.value("body").toString();
    if (args.contains(QStringLiteral("headers")) && args.value("headers").isObject()) {
        p.headers.clear();
        const QJsonObject headers = args.value("headers").toObject();
        for (auto it = headers.begin(); it != headers.end(); ++it)
            p.headers.append({it.key(), it.value().toString(), true});
    }
    CollectionStore::instance()->updateRequest(colId, requestId, p);
    QString newName = oldName;
    if (args.contains(QStringLiteral("name"))) {
        newName = args.value("name").toString();
        CollectionStore::instance()->renameItem(colId, requestId, newName);
    }
    return QJsonObject{{"success", true}, {"output", QStringLiteral("已更新请求 %1").arg(requestId)}};
}

QJsonObject AiPanel::pulseDeleteRequest(const QString &requestId) const {
    QString colId;
    if (!CollectionStore::instance()->findItemLocation(requestId, &colId, nullptr))
        return QJsonObject{{"success", false}, {"output", QStringLiteral("未找到请求 %1").arg(requestId)}};
    CollectionStore::instance()->removeItem(colId, requestId);
    return QJsonObject{{"success", true}, {"output", QStringLiteral("已删除请求 %1").arg(requestId)}};
}

QJsonObject AiPanel::pulseListEnvironments() const {
    QStringList lines;
    lines << QStringLiteral("【环境】");
    const QVector<Environment> &envs = EnvironmentStore::instance()->environments();
    const QString active = EnvironmentStore::instance()->activeEnvironmentId();
    if (envs.isEmpty()) {
        return QJsonObject{{"success", true}, {"output", QStringLiteral("（暂无环境）")}};
    }
    for (const Environment &e : envs) {
        QStringList vars;
        for (const EnvVar &v : e.variables)
            vars << QStringLiteral("%1=%2%3").arg(v.key, v.value)
                        .arg(v.enabled ? QString() : QStringLiteral("(禁用)"));
        lines << QStringLiteral("- %1%2: %3")
                     .arg(e.name)
                     .arg(e.id == active ? QStringLiteral("（当前活动）") : QString())
                     .arg(vars.isEmpty() ? QStringLiteral("（空）") : vars.join(QStringLiteral(", ")));
    }
    return QJsonObject{{"success", true}, {"output", lines.join(QLatin1Char('\n'))}};
}

QJsonObject AiPanel::pulseSetEnvVariable(const QJsonObject &args) const {
    EnvironmentStore *store = EnvironmentStore::instance();
    QString envId = store->activeEnvironmentId();
    const QString envName = args.value("envName").toString();
    if (!envName.isEmpty()) {
        envId.clear();
        for (const Environment &e : store->environments())
            if (e.name == envName || e.id == envName) { envId = e.id; break; }
        if (envId.isEmpty())
            return QJsonObject{{"success", false}, {"output", QStringLiteral("未找到环境 %1").arg(envName)}};
    }
    if (envId.isEmpty())
        return QJsonObject{{"success", false}, {"output", QStringLiteral("当前没有活动环境")}};

    const Environment *env = store->environmentById(envId);
    const QString key = args.value("key").toString();
    const QString value = args.value("value").toString();
    if (!env) return QJsonObject{{"success", false}, {"output", QStringLiteral("环境不存在")}};
    int idx = -1;
    for (int i = 0; i < env->variables.size(); ++i)
        if (env->variables[i].key == key) { idx = i; break; }
    if (idx >= 0) {
        store->updateVariable(envId, idx, key, value, true);
    } else {
        store->addVariable(envId);
        const Environment *e2 = store->environmentById(envId);
        if (!e2) return QJsonObject{{"success", false}, {"output", QStringLiteral("环境不存在")}};
        store->updateVariable(envId, e2->variables.size() - 1, key, value, true);
    }
    return QJsonObject{{"success", true}, {"output", QStringLiteral("已设置 %1=%2").arg(key, value)}};
}
