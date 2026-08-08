#include "RequestPanel.h"
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
    auto *row = new QHBoxLayout;

    m_method = new QComboBox(this);
    m_method->addItems({QStringLiteral("GET"), QStringLiteral("POST"), QStringLiteral("PUT"),
                         QStringLiteral("PATCH"), QStringLiteral("DELETE")});
    row->addWidget(m_method);

    m_url = new QLineEdit(this);
    m_url->setPlaceholderText(QStringLiteral("https://api.example.com/users"));
    row->addWidget(m_url, 1);

    auto *send = new QPushButton(QStringLiteral("发送"), this);
    connect(send, &QPushButton::clicked, this, &RequestPanel::sendRequest);
    row->addWidget(send);

    layout->addLayout(row);

    auto *bodyLabel = new QLabel(QStringLiteral("Body"), this);
    layout->addWidget(bodyLabel);
    m_body = new QPlainTextEdit(this);
    m_body->setPlaceholderText(QStringLiteral("JSON body (POST/PUT/PATCH)"));
    layout->addWidget(m_body, 1);

    m_nam = new QNetworkAccessManager(this);
}

RequestPanel::~RequestPanel() = default;

void RequestPanel::sendRequest() {
    const QString urlStr = m_url->text().trimmed();
    if (urlStr.isEmpty()) return;
    const QUrl url(urlStr);
    const QByteArray method = m_method->currentText().toUpper().toUtf8();

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("pulse-qt"));

    QElapsedTimer timer;
    timer.start();

    QNetworkReply *reply = m_nam->sendCustomRequest(req, method, m_body->toPlainText().toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        const qint64 msec = timer.nsecsElapsed() / 1000000;
        reply->deleteLater();
        emit responseReceived(status, msec, body);
    });
}
