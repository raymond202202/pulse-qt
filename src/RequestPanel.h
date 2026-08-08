#pragma once

#include <QWidget>
#include "KeyValue.h"

class QLineEdit;
class QComboBox;
class QPlainTextEdit;
class QTableWidget;
class QTabWidget;
class QNetworkAccessManager;
class QNetworkReply;

class RequestPanel : public QWidget {
    Q_OBJECT
public:
    explicit RequestPanel(QWidget *parent = nullptr);
    ~RequestPanel() override;

    // 当前编辑区快照
    RequestPayload payload() const;
    // 最近一次发送时的原始值（未做变量解析，供历史/保存用）
    RequestPayload sentPayload() const { return m_sent; }

public slots:
    void loadRequest(const RequestPayload &req);

signals:
    void responseReceived(int status, qint64 msec, const QByteArray &body);
    void saveToCollectionRequested();

private slots:
    void sendRequest();
    void saveToCollection();
    void addHeaderRow();
    void addParamRow();
    void removeSelectedRow();

private:
    static QVector<KeyValueRow> rowsFromTable(const QTableWidget *table);
    static void tableFromRows(QTableWidget *table, const QVector<KeyValueRow> &rows);

    QComboBox *m_method = nullptr;
    QLineEdit *m_url = nullptr;
    QTabWidget *m_tabs = nullptr;
    QPlainTextEdit *m_body = nullptr;
    QTableWidget *m_headersTable = nullptr;
    QTableWidget *m_paramsTable = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    RequestPayload m_sent;
};
