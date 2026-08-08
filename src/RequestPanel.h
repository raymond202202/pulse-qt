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
    // 最近一次发送时的原始值（未做变量解析，供历史记录/保存用）
    QString sentMethod() const { return m_sentMethod; }
    QString sentUrl() const { return m_sentUrl; }
    QString sentBody() const { return m_sentBody; }

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
    QString m_sentMethod, m_sentUrl, m_sentBody;
};
