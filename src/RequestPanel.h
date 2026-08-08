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

    QString method() const;
    QString url() const;
    QString bodyText() const;

public slots:
    void loadRequest(const QString &method, const QString &url, const QString &body);

signals:
    void responseReceived(int status, qint64 msec, const QByteArray &body);
    void saveToCollectionRequested();

private slots:
    void sendRequest();
    void saveToCollection();

private:
    QComboBox *m_method = nullptr;
    QLineEdit *m_url = nullptr;
    QPlainTextEdit *m_body = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
};
