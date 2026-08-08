#include "RequestPanel.h"
#include "EnvironmentStore.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QElapsedTimer>
#include <QJsonDocument>

RequestPanel::RequestPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *row = new QHBoxLayout;

    m_method = new QComboBox(this);
    m_method->addItems({QStringLiteral("GET"), QStringLiteral("POST"), QStringLiteral("PUT"),
                         QStringLiteral("PATCH"), QStringLiteral("DELETE")});
    row->addWidget(m_method);

    m_url = new QLineEdit(this);
    m_url->setPlaceholderText(QStringLiteral("https://api.example.com/users（支持 {{变量}}）"));
    row->addWidget(m_url, 1);

    auto *save = new QPushButton(QStringLiteral("保存到集合"), this);
    save->setObjectName("copyBtn");
    connect(save, &QPushButton::clicked, this, &RequestPanel::saveToCollection);
    row->addWidget(save);

    auto *send = new QPushButton(QStringLiteral("发送"), this);
    send->setObjectName("sendButton");
    connect(send, &QPushButton::clicked, this, &RequestPanel::sendRequest);
    row->addWidget(send);

    layout->addLayout(row);

    auto *bodyLabel = new QLabel(QStringLiteral("Body"), this);
    layout->addWidget(bodyLabel);
    m_body = new QPlainTextEdit(this);
    m_body->setPlaceholderText(QStringLiteral("JSON body (POST/PUT/PATCH)，支持 {{变量}}"));
    layout->addWidget(m_body, 1);

    m_nam = new QNetworkAccessManager(this);
}

RequestPanel::~RequestPanel() = default;

QString RequestPanel::method() const { return m_method->currentText(); }
QString RequestPanel::url() const { return m_url->text(); }
QString RequestPanel::bodyText() const { return m_body->toPlainText(); }

void RequestPanel::loadRequest(const QString &method, const QString &url, const QString &body) {
    const int idx = m_method->findText(method, Qt::MatchFixedString);
    if (idx >= 0) m_method->setCurrentIndex(idx);
    m_url->setText(url);
    m_body->setPlainText(body);
}

void RequestPanel::saveToCollection() {
    emit saveToCollectionRequested();
}

void RequestPanel::sendRequest() {
    // 发送前用当前环境变量解析 {{var}} 占位符
    m_sentMethod = m_method->currentText().toUpper();
    m_sentUrl = m_url->text().trimmed();
    m_sentBody = m_body->toPlainText();
    const QString resolvedUrl = EnvironmentStore::instance()->resolve(m_sentUrl);
    const QString resolvedBody = EnvironmentStore::instance()->resolve(m_sentBody);
    if (resolvedUrl.isEmpty()) return;
    const QUrl url(resolvedUrl);
    const QByteArray method = m_method->currentText().toUpper().toUtf8();

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("pulse-qt"));

    QElapsedTimer timer;
    timer.start();

    QNetworkReply *reply = m_nam->sendCustomRequest(req, method, resolvedBody.toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        const qint64 msec = timer.nsecsElapsed() / 1000000;
        reply->deleteLater();
        emit responseReceived(status, msec, body);
    });
}
