#include "ResponsePanel.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QJsonDocument>

ResponsePanel::ResponsePanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    m_meta = new QLabel(QStringLiteral("—"), this);
    layout->addWidget(m_meta);
    m_body = new QPlainTextEdit(this);
    m_body->setReadOnly(true);
    layout->addWidget(m_body, 1);
}

void ResponsePanel::showResponse(int status, qint64 msec, const QByteArray &body) {
    m_meta->setText(QStringLiteral("状态: %1 · 耗时: %2 ms").arg(status).arg(msec));
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        m_body->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
    } else {
        m_body->setPlainText(QString::fromUtf8(body));
    }
}
