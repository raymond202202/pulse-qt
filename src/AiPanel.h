#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <functional>
#include "KeyValue.h"
#include "CollectionStore.h"

class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class RequestPanel;
class ResponsePanel;

// AI 助手面板：flare 网络专家（子进程 JSON Lines 协议）
// - 流式聊天 + 工具调用（http_request / pulse_* 等宿主工具）
// - 快捷按钮：解释报错 / 生成请求
class AiPanel : public QWidget {
    Q_OBJECT
public:
    explicit AiPanel(RequestPanel *request, ResponsePanel *response, QWidget *parent = nullptr);

private slots:
    void sendMessage();
    void sendPreset(const QString &prompt);
    void stopGenerating();
    void newSession();
    void onLine(const QJsonObject &obj);
    void onServerState(bool running, const QString &status);
    void onServerError(const QString &message);

private:
    void appendLog(const QString &text);
    void runChat(const QString &input);
    QString buildContext() const;
    QJsonArray toolDefinitions() const;
    // 工具执行：local 同步 / http_request 异步，完成后回调 result JSON
    void executeTool(const QString &id, const QString &name, const QJsonObject &args);
    void finishTool(const QString &id, bool success, const QString &output, const QString &error = QString());
    // 各工具实现
    void httpRequestAsync(const QJsonObject &args,
                          std::function<void(bool, QString, QString)> done);
    QJsonObject pulseListCollections() const;
    QJsonObject pulseGetRequest(const QString &requestId) const;
    QJsonObject pulseGetActiveRequest() const;
    QJsonObject pulseGetActiveResponse() const;
    QJsonObject pulseCreateRequest(const QJsonObject &args) const;
    QJsonObject pulseUpdateRequest(const QJsonObject &args) const;
    QJsonObject pulseDeleteRequest(const QString &requestId) const;
    QJsonObject pulseListEnvironments() const;
    QJsonObject pulseSetEnvVariable(const QJsonObject &args) const;
    static QString summarizeItem(const CollectionItem &item);
    static QJsonObject summaryJson(const RequestPayload &p);

    RequestPanel *m_request = nullptr;
    ResponsePanel *m_response = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QLineEdit *m_input = nullptr;
    QPushButton *m_sendBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QPushButton *m_newSessionBtn = nullptr;
    QLabel *m_statusLabel = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QHash<QNetworkReply *, std::function<void(bool, QString, QString)>> m_pendingHttp;
    QString m_sessionId;
    bool m_generating = false;
};
