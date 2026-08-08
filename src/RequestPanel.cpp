#include "RequestPanel.h"
#include "EnvironmentStore.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QHeaderView>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
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

    // ── 页签：Body / Headers / Params ──
    m_tabs = new QTabWidget(this);

    m_body = new QPlainTextEdit(this);
    m_body->setPlaceholderText(QStringLiteral("JSON body (POST/PUT/PATCH)，支持 {{变量}}"));
    m_tabs->addTab(m_body, QStringLiteral("Body"));

    auto *headersPage = new QWidget(this);
    auto *headersLayout = new QVBoxLayout(headersPage);
    headersLayout->setContentsMargins(0, 0, 0, 0);
    m_headersTable = new QTableWidget(0, 3, this);
    m_headersTable->setHorizontalHeaderLabels(
        {QStringLiteral("启用"), QStringLiteral("Key"), QStringLiteral("Value")});
    m_headersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_headersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_headersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_headersTable->verticalHeader()->setVisible(false);
    m_headersTable->setMinimumHeight(120);
    headersLayout->addWidget(m_headersTable, 1);
    auto *headersBtnRow = new QHBoxLayout;
    auto *addHeader = new QPushButton(QStringLiteral("添加行"), this);
    auto *delHeader = new QPushButton(QStringLiteral("删除选中行"), this);
    addHeader->setObjectName("copyBtn");
    delHeader->setObjectName("copyBtn");
    headersBtnRow->addWidget(addHeader);
    headersBtnRow->addWidget(delHeader);
    headersBtnRow->addStretch(1);
    headersLayout->addLayout(headersBtnRow);
    m_tabs->addTab(headersPage, QStringLiteral("Headers"));

    auto *paramsPage = new QWidget(this);
    auto *paramsLayout = new QVBoxLayout(paramsPage);
    paramsLayout->setContentsMargins(0, 0, 0, 0);
    m_paramsTable = new QTableWidget(0, 3, this);
    m_paramsTable->setHorizontalHeaderLabels(
        {QStringLiteral("启用"), QStringLiteral("Key"), QStringLiteral("Value")});
    m_paramsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_paramsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_paramsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_paramsTable->verticalHeader()->setVisible(false);
    m_paramsTable->setMinimumHeight(120);
    paramsLayout->addWidget(m_paramsTable, 1);
    auto *paramsBtnRow = new QHBoxLayout;
    auto *addParam = new QPushButton(QStringLiteral("添加行"), this);
    auto *delParam = new QPushButton(QStringLiteral("删除选中行"), this);
    addParam->setObjectName("copyBtn");
    delParam->setObjectName("copyBtn");
    paramsBtnRow->addWidget(addParam);
    paramsBtnRow->addWidget(delParam);
    paramsBtnRow->addStretch(1);
    paramsLayout->addLayout(paramsBtnRow);
    m_tabs->addTab(paramsPage, QStringLiteral("Params"));

    layout->addWidget(m_tabs, 1);

    connect(addHeader, &QPushButton::clicked, this, &RequestPanel::addHeaderRow);
    connect(addParam, &QPushButton::clicked, this, &RequestPanel::addParamRow);
    connect(delHeader, &QPushButton::clicked, this, &RequestPanel::removeSelectedRow);
    connect(delParam, &QPushButton::clicked, this, &RequestPanel::removeSelectedRow);

    m_nam = new QNetworkAccessManager(this);
}

RequestPanel::~RequestPanel() = default;

QVector<KeyValueRow> RequestPanel::rowsFromTable(const QTableWidget *table) {
    QVector<KeyValueRow> rows;
    for (int i = 0; i < table->rowCount(); ++i) {
        KeyValueRow r;
        r.enabled = table->item(i, 0) && table->item(i, 0)->checkState() == Qt::Checked;
        r.key = table->item(i, 1) ? table->item(i, 1)->text().trimmed() : QString();
        r.value = table->item(i, 2) ? table->item(i, 2)->text() : QString();
        rows.append(r);
    }
    return rows;
}

void RequestPanel::tableFromRows(QTableWidget *table, const QVector<KeyValueRow> &rows) {
    table->setRowCount(0);
    table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const KeyValueRow &r = rows[i];
        auto *check = new QTableWidgetItem();
        check->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        check->setCheckState(r.enabled ? Qt::Checked : Qt::Unchecked);
        table->setItem(i, 0, check);
        table->setItem(i, 1, new QTableWidgetItem(r.key));
        table->setItem(i, 2, new QTableWidgetItem(r.value));
    }
}

RequestPayload RequestPanel::payload() const {
    RequestPayload p;
    p.method = m_method->currentText().toUpper();
    p.url = m_url->text().trimmed();
    p.body = m_body->toPlainText();
    p.headers = rowsFromTable(m_headersTable);
    p.params = rowsFromTable(m_paramsTable);
    return p;
}

void RequestPanel::loadRequest(const RequestPayload &req) {
    const int idx = m_method->findText(req.method, Qt::MatchFixedString);
    if (idx >= 0) m_method->setCurrentIndex(idx);
    m_url->setText(req.url);
    m_body->setPlainText(req.body);
    tableFromRows(m_headersTable, req.headers);
    tableFromRows(m_paramsTable, req.params);
}

void RequestPanel::addHeaderRow() {
    const int row = m_headersTable->rowCount();
    m_headersTable->insertRow(row);
    auto *check = new QTableWidgetItem();
    check->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    check->setCheckState(Qt::Checked);
    m_headersTable->setItem(row, 0, check);
    m_headersTable->setItem(row, 1, new QTableWidgetItem());
    m_headersTable->setItem(row, 2, new QTableWidgetItem());
    m_headersTable->setCurrentCell(row, 1);
}

void RequestPanel::addParamRow() {
    const int row = m_paramsTable->rowCount();
    m_paramsTable->insertRow(row);
    auto *check = new QTableWidgetItem();
    check->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    check->setCheckState(Qt::Checked);
    m_paramsTable->setItem(row, 0, check);
    m_paramsTable->setItem(row, 1, new QTableWidgetItem());
    m_paramsTable->setItem(row, 2, new QTableWidgetItem());
    m_paramsTable->setCurrentCell(row, 1);
}

void RequestPanel::removeSelectedRow() {
    QTableWidget *table = nullptr;
    if (m_tabs->currentIndex() == 1) table = m_headersTable;
    else if (m_tabs->currentIndex() == 2) table = m_paramsTable;
    if (!table) return;
    const int row = table->currentRow();
    if (row >= 0) table->removeRow(row);
}

void RequestPanel::saveToCollection() {
    emit saveToCollectionRequested();
}

void RequestPanel::sendRequest() {
    EnvironmentStore *env = EnvironmentStore::instance();
    m_sent = payload();

    const QString resolvedUrl = env->resolve(m_sent.url);
    if (resolvedUrl.isEmpty()) return;
    const QString resolvedBody = env->resolve(m_sent.body);

    QUrl url(resolvedUrl);

    // Params 表自动拼到 URL（保留 URL 里已有的 query）
    QUrlQuery query(url);
    for (const KeyValueRow &r : m_sent.params) {
        if (r.enabled && !r.key.isEmpty())
            query.addQueryItem(env->resolve(r.key), env->resolve(r.value));
    }
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("pulse-qt"));

    // Headers 表（启用且 Key 非空才生效，值支持 {{var}}）
    for (const KeyValueRow &r : m_sent.headers) {
        if (r.enabled && !r.key.isEmpty())
            req.setRawHeader(env->resolve(r.key).toUtf8(), env->resolve(r.value).toUtf8());
    }

    const QByteArray method = m_sent.method.toUtf8();

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
