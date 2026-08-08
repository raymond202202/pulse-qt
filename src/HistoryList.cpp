#include "HistoryList.h"
#include "HistoryStore.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QDateTime>
#include <QColor>

namespace {
QColor statusColor(int status) {
    if (status == 0) return QColor("#57606a");
    if (status < 300) return QColor("#1a7f37");
    if (status < 400) return QColor("#b35900");
    return QColor("#c93c3c");
}

QString shortTime(const QString &iso) {
    const QDateTime dt = QDateTime::fromString(iso, Qt::ISODate);
    return dt.isValid() ? dt.toString(QStringLiteral("MM-dd HH:mm:ss")) : QString();
}
} // namespace

HistoryList::HistoryList(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);

    auto *header = new QHBoxLayout;
    header->addWidget(new QLabel(QStringLiteral("历史"), this));
    header->addStretch(1);
    auto *clearBtn = new QPushButton(QStringLiteral("清空"), this);
    clearBtn->setObjectName("copyBtn");
    header->addWidget(clearBtn);
    layout->addLayout(header);

    m_list = new QListWidget(this);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setWordWrap(false);
    layout->addWidget(m_list, 1);

    connect(m_list, &QListWidget::itemClicked, this, &HistoryList::onItemClicked);
    connect(m_list, &QListWidget::customContextMenuRequested,
            this, &HistoryList::onCustomContextMenu);
    connect(clearBtn, &QPushButton::clicked, this, &HistoryList::clearAll);
    connect(HistoryStore::instance(), &HistoryStore::changed, this, &HistoryList::reload);

    reload();
}

void HistoryList::reload() {
    m_list->clear();
    const QVector<HistoryEntry> entries = HistoryStore::instance()->entries(200);
    for (const HistoryEntry &e : entries) {
        QString status = e.status > 0 ? QString::number(e.status) : QStringLiteral("—");
        const QString text = QStringLiteral("%1  %2\n%3  %4")
                                 .arg(e.payload.method)
                                 .arg(e.payload.url)
                                 .arg(shortTime(e.createdAt))
                                 .arg(QStringLiteral("状态 %1 · %2 ms").arg(status).arg(e.msec));
        auto *item = new QListWidgetItem(text);
        item->setForeground(statusColor(e.status));
        item->setData(Qt::UserRole, e.id);
        item->setData(Qt::UserRole + 1, QVariant::fromValue(e.payload));
        item->setToolTip(e.payload.url);
        m_list->addItem(item);
    }
    if (entries.isEmpty())
        m_list->addItem(QStringLiteral("（暂无历史，发送请求后自动记录）"));
}

void HistoryList::onItemClicked(QListWidgetItem *item) {
    if (!item) return;
    const RequestPayload req = item->data(Qt::UserRole + 1).value<RequestPayload>();
    if (!req.url.isEmpty()) emit requestActivated(req);
}

void HistoryList::onCustomContextMenu(const QPoint &pos) {
    QListWidgetItem *item = m_list->itemAt(pos);
    if (!item) return;
    const qint64 id = item->data(Qt::UserRole).toLongLong();
    QMenu menu(this);
    QAction *del = menu.addAction(QStringLiteral("删除该条"));
    QAction *clear = menu.addAction(QStringLiteral("清空全部"));
    QAction *chosen = menu.exec(m_list->viewport()->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == del) HistoryStore::instance()->removeEntry(id);
    else if (chosen == clear) HistoryStore::instance()->clearHistory();
}

void HistoryList::clearAll() {
    HistoryStore::instance()->clearHistory();
}
