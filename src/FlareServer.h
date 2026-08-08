#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

class QProcess;

// flare 引擎宿主协议客户端（单例）
// spawn `node <flare> server --profile ... --storage ...`，stdin/stdout JSON Lines
// 协议见 ~/hermes-projects/flare/docs/host-protocol.md
// API key 绝不硬编码：flare 自行从环境变量 / ~/.flare/.env 读取
class FlareServer : public QObject {
    Q_OBJECT
public:
    static FlareServer *instance();

    bool isRunning() const;
    QString flareBin() const;
    QString lastError() const;
    // 检测是否配置了可用 key（只读检查环境变量 / ~/.flare/.env 键名）
    static bool hasApiKey();

public slots:
    // 启动子进程（幂等；已运行则忽略）。失败会发 stateChanged(false, 原因)
    void start();
    void sendChat(const QString &sessionId, const QString &input,
                  const QString &context, const QJsonArray &tools);
    void sendCancel(const QString &sessionId);
    void sendToolResult(const QString &id, const QJsonObject &result);

signals:
    // 每行 stdout JSON（text/done/error/tool_execute/...）
    void lineReceived(const QJsonObject &obj);
    void stateChanged(bool running, const QString &status);
    void errorOccurred(const QString &message);

private:
    explicit FlareServer(QObject *parent = nullptr);
    void writeLine(const QJsonObject &obj);
    void onReadyRead();
    void onProcessError();
    static QString findFlareBin();
    static QString ensureProfileFile();
    static QString storagePath();

    QProcess *m_proc = nullptr;
    QByteArray m_buffer;
    QString m_flareBin;
    QString m_lastError;
};
