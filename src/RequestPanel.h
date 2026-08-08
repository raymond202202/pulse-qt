#pragma once

#include <QWidget>

class QLineEdit;
class QComboBox;
class QPlainTextEdit;
class QNetworkAccessManager;
class QNetworkReply;

class RequestPanel : public QWidget {
    Q_OBJECT
public:
    explicit RequestPanel(QWidget *parent = nullptr);
    ~RequestPanel() override;

signals:
    void responseReceived(int status, qint64 msec, const QByteArray &body);

private slots:
    void sendRequest();

private:
    QComboBox *m_method = nullptr;
    QLineEdit *m_url = nullptr;
    QPlainTextEdit *m_body = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
};
