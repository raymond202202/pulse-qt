#include "FlareServer.h"

#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>

namespace {
const char *kProfileName = "network-expert.json";
const char *kStorageName = "flare-ai.db";
}

FlareServer *FlareServer::instance() {
    static FlareServer s;
    return &s;
}

FlareServer::FlareServer(QObject *parent) : QObject(parent) {
    m_flareBin = findFlareBin();
    m_proc = new QProcess(this);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &FlareServer::onReadyRead);
    connect(m_proc, &QProcess::errorOccurred, this, &FlareServer::onProcessError);
}

QString FlareServer::flareBin() const { return m_flareBin; }
QString FlareServer::lastError() const { return m_lastError; }
bool FlareServer::isRunning() const { return m_proc && m_proc->state() == QProcess::Running; }

bool FlareServer::hasApiKey() {
    // 1) 环境变量
    if (!qEnvironmentVariableIsEmpty("DEEPSEEK_API_KEY")
        || !qEnvironmentVariableIsEmpty("OPENAI_API_KEY")
        || !qEnvironmentVariableIsEmpty("ANTHROPIC_API_KEY")) {
        return true;
    }
    // 2) ~/.flare/.env（只检查键名是否存在，不读取值）
    const QString envPath = QDir::homePath() + QStringLiteral("/.flare/.env");
    QFile f(envPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    static const QRegularExpression re(
        QStringLiteral(R"(^\s*(?:DEEPSEEK|OPENAI|ANTHROPIC)_API_KEY\s*=\s*\S+)"));
    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine());
        if (re.match(line).hasMatch()) return true;
    }
    return false;
}

QString FlareServer::findFlareBin() {
    // 1) 显式覆盖（测试/自定义安装）
    const QByteArray overrideBin = qgetenv("PULSE_QT_FLARE_BIN");
    if (!overrideBin.isEmpty() && QFileInfo::exists(QString::fromLocal8Bit(overrideBin)))
        return QString::fromLocal8Bit(overrideBin);
    // 2) 仓库开发版（当前用户环境）
    const QString repoBin = QDir::homePath() + QStringLiteral("/hermes-projects/flare/bin/flare");
    if (QFileInfo::exists(repoBin)) return repoBin;
    // 3) PATH 里的 flare（可能为旧版无 server 命令，启动时会报错提示）
    return QStandardPaths::findExecutable(QStringLiteral("flare"));
}

QString FlareServer::storagePath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/") + QString::fromLatin1(kStorageName);
}

QString FlareServer::ensureProfileFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    const QString path = dir + QStringLiteral("/") + QString::fromLatin1(kProfileName);

    QJsonObject profile;
    profile.insert("name", QStringLiteral("pulse-qt 网络专家"));
    profile.insert("identity", QStringLiteral("我是 pulse-qt 助手，是集成到 pulse-qt 的 flare 网络专家"));
    profile.insert("flareIntro",
        QStringLiteral("flare 是一款由我的作者开发的通用型 AI agent，pulse-qt 助手集成并深度定制了它的网络专家能力。"));
    profile.insert("systemPrompt", QStringLiteral(
        "你是 pulse-qt 助手，集成在 pulse-qt 应用中的网络专家。\n"
        "你专注网络请求调试、API 联调、URL 分析、HTTP 响应诊断。\n"
        "工作原则：\n"
        "1. 用户给出 URL 或请求需求时，用 http_request 实际发请求验证，不要只描述\n"
        "2. 请求失败时，用 response_analyze 分析状态码/耗时/头部，给出排查建议\n"
        "3. 涉及 Pulse 内的数据（集合/请求/环境变量），用 pulse_* 工具读取或修改；修改前先读取确认\n"
        "4. 涉及 API key 的请求，用环境变量引用（如 {{API_KEY}}），不要明文写死\n"
        "5. 用户询问当前状态时，优先用 pulse_get_active_request / pulse_list_environments 获取实时数据\n"
        "用中文回答用户的问题。"));

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(profile).toJson(QJsonDocument::Compact));
    }
    return path;
}

void FlareServer::start() {
    if (isRunning()) return;
    if (m_flareBin.isEmpty()) {
        m_lastError = QStringLiteral("未找到 flare 引擎（可用环境变量 PULSE_QT_FLARE_BIN 指定路径）");
        emit stateChanged(false, m_lastError);
        emit errorOccurred(m_lastError);
        return;
    }
    // 用 node 启动（bin/flare 是 ESM 脚本）
    const QString node = QStandardPaths::findExecutable(QStringLiteral("node"));
    if (node.isEmpty()) {
        m_lastError = QStringLiteral("未找到 node 运行时");
        emit stateChanged(false, m_lastError);
        emit errorOccurred(m_lastError);
        return;
    }

    QStringList args;
    args << m_flareBin
         << QStringLiteral("server")
         << QStringLiteral("--profile") << ensureProfileFile()
         << QStringLiteral("--storage") << storagePath()
         << QStringLiteral("--namespace") << QStringLiteral("pulse-qt");

    m_proc->start(node, args);
    m_lastError.clear();
    emit stateChanged(true, QStringLiteral("flare 已连接"));
}

void FlareServer::writeLine(const QJsonObject &obj) {
    if (!isRunning()) return;
    m_proc->write(QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n");
}

void FlareServer::sendChat(const QString &sessionId, const QString &input,
                           const QString &context, const QJsonArray &tools) {
    QJsonObject msg;
    msg.insert("type", QStringLiteral("chat"));
    msg.insert("sessionId", sessionId);
    msg.insert("input", input);
    if (!context.isEmpty()) msg.insert("context", context);
    if (!tools.isEmpty()) msg.insert("tools", tools);
    writeLine(msg);
}

void FlareServer::sendCancel(const QString &sessionId) {
    QJsonObject msg;
    msg.insert("type", QStringLiteral("cancel"));
    msg.insert("sessionId", sessionId);
    writeLine(msg);
}

void FlareServer::sendToolResult(const QString &id, const QJsonObject &result) {
    QJsonObject msg;
    msg.insert("type", QStringLiteral("tool_result"));
    msg.insert("id", id);
    msg.insert("result", result);
    writeLine(msg);
}

void FlareServer::onReadyRead() {
    m_buffer += m_proc->readAllStandardOutput();
    int idx;
    while ((idx = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);
        if (line.isEmpty()) continue;
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;
        emit lineReceived(doc.object());
    }
}

void FlareServer::onProcessError() {
    if (m_proc->error() == QProcess::FailedToStart) {
        m_lastError = QStringLiteral("flare 引擎启动失败：%1").arg(m_flareBin);
        emit stateChanged(false, m_lastError);
        emit errorOccurred(m_lastError);
    } else if (m_proc->state() == QProcess::NotRunning) {
        m_lastError = QStringLiteral("flare 引擎已退出（exit %1）").arg(m_proc->exitCode());
        emit stateChanged(false, m_lastError);
        emit errorOccurred(m_lastError);
    }
}
