#include "ResponsePanel.h"
#include "JsonTreeModel.h"
#include "JsonTreeDelegate.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeView>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QClipboard>
#include <QGuiApplication>

namespace {

QString formatBytes(qint64 bytes) {
    if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

QString statusColor(int status) {
    if (status >= 200 && status < 300) return QStringLiteral("#1a7f37");
    if (status >= 300 && status < 400) return QStringLiteral("#b35900");
    if (status >= 400) return QStringLiteral("#c93c3c");
    return QStringLiteral("#57606a");
}

} // namespace

ResponsePanel::ResponsePanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // ── 元信息行：状态（按码着色）· 耗时 · 大小 · 复制 ──
    auto *metaRow = new QHBoxLayout;
    m_statusLabel = new QLabel(QStringLiteral("—"), this);
    m_metaLabel = new QLabel(QStringLiteral(""), this);
    auto *copyBtn = new QPushButton(QStringLiteral("复制"), this);
    copyBtn->setObjectName("copyBtn");
    connect(copyBtn, &QPushButton::clicked, this, &ResponsePanel::copyBody);
    metaRow->addWidget(m_statusLabel);
    metaRow->addWidget(m_metaLabel);
    metaRow->addStretch(1);
    metaRow->addWidget(copyBtn);
    layout->addLayout(metaRow);

    // ── 视图切换：树形 / 文本 ──
    auto *viewRow = new QHBoxLayout;
    auto *treeBtn = new QPushButton(QStringLiteral("树形"), this);
    auto *textBtn = new QPushButton(QStringLiteral("文本"), this);
    treeBtn->setCheckable(true);
    textBtn->setCheckable(true);
    treeBtn->setObjectName("viewBtn");
    textBtn->setObjectName("viewBtn");
    m_viewGroup = new QButtonGroup(this);
    m_viewGroup->setExclusive(true);
    m_viewGroup->addButton(treeBtn, 0);
    m_viewGroup->addButton(textBtn, 1);
    connect(m_viewGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &ResponsePanel::switchView);
    viewRow->addWidget(treeBtn);
    viewRow->addWidget(textBtn);
    viewRow->addStretch(1);
    layout->addLayout(viewRow);

    // ── 内容栈：树（懒加载）/ 纯文本 ──
    m_stack = new QStackedWidget(this);

    m_tree = new QTreeView(this);
    m_model = new JsonTreeModel(this);
    m_delegate = new JsonTreeDelegate(this);
    m_delegate->setThemeColors(false); // 浅色主题
    m_tree->setModel(m_model);
    m_tree->setItemDelegate(m_delegate);
    m_tree->setHeaderHidden(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setExpandsOnDoubleClick(true);
    m_stack->addWidget(m_tree);

    m_text = new QPlainTextEdit(this);
    m_text->setReadOnly(true);
    m_text->setPlaceholderText(QStringLiteral("响应将显示在这里"));
    m_stack->addWidget(m_text);

    layout->addWidget(m_stack, 1);

    textBtn->setChecked(true);
    treeBtn->setEnabled(false);
}

void ResponsePanel::showResponse(int status, qint64 msec, const QByteArray &body) {
    m_lastBody = body;
    m_lastStatus = status;
    m_lastMsec = msec;
    m_hasResponse = true;

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    const bool isJson = err.error == QJsonParseError::NoError && (doc.isObject() || doc.isArray());
    m_lastIsJson = isJson;

    m_statusLabel->setText(QStringLiteral("状态: %1").arg(status));
    m_statusLabel->setStyleSheet(
        QStringLiteral("color: %1; font-weight: 600;").arg(statusColor(status)));

    QString meta = QStringLiteral("耗时: %1 ms").arg(msec);
    meta += QStringLiteral(" · 大小: %1").arg(formatBytes(body.size()));

    if (isJson) {
        m_model->setJson(doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object()));
        meta += QStringLiteral(" · 节点: %1").arg(m_model->totalNodes());
        m_tree->expandToDepth(1); // 展开根 → 顶层键
        m_lastPrettyJson = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    } else {
        m_model->clearData();
        m_lastPrettyJson.clear();
    }
    m_metaLabel->setText(meta);
    m_text->setPlainText(isJson ? m_lastPrettyJson : QString::fromUtf8(body));

    setViewEnabled(isJson);
    if (isJson) {
        m_viewGroup->button(0)->setChecked(true); // 树形
        m_stack->setCurrentIndex(0);
    } else {
        m_viewGroup->button(1)->setChecked(true); // 文本
        m_stack->setCurrentIndex(1);
    }
}

void ResponsePanel::switchView(int id) {
    m_stack->setCurrentIndex(id);
}

void ResponsePanel::setViewEnabled(bool treeEnabled) {
    if (QAbstractButton *btn = m_viewGroup->button(0)) btn->setEnabled(treeEnabled);
}

void ResponsePanel::copyBody() {
    const QString text = m_lastIsJson ? m_lastPrettyJson : QString::fromUtf8(m_lastBody);
    if (!text.isEmpty()) QGuiApplication::clipboard()->setText(text);
}
