#include "CollectionTree.h"
#include "CollectionStore.h"
#include "EnvironmentStore.h"
#include "EnvironmentDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QComboBox>
#include <QPushButton>
#include <QMenu>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>

namespace {
QColor methodColor(const QString &method) {
    const QString m = method.toUpper();
    if (m == "GET") return QColor("#1a7f37");
    if (m == "POST") return QColor("#6d4aff");
    if (m == "PUT") return QColor("#b35900");
    if (m == "PATCH") return QColor("#6e3fa0");
    if (m == "DELETE") return QColor("#c93c3c");
    return QColor("#57606a");
}
} // namespace

CollectionTree::CollectionTree(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);

    // ── 环境选择行 ──
    auto *envRow = new QHBoxLayout;
    auto *envLabel = new QLabel(QStringLiteral("环境"), this);
    m_envCombo = new QComboBox(this);
    auto *envBtn = new QPushButton(QStringLiteral("管理"), this);
    envBtn->setObjectName("copyBtn");
    envRow->addWidget(envLabel);
    envRow->addWidget(m_envCombo, 1);
    envRow->addWidget(envBtn);
    layout->addLayout(envRow);

    // ── 集合标题行 ──
    auto *colRow = new QHBoxLayout;
    auto *colTitle = new QLabel(QStringLiteral("集合"), this);
    auto *addBtn = new QPushButton(QStringLiteral("新建集合"), this);
    addBtn->setObjectName("copyBtn");
    colRow->addWidget(colTitle);
    colRow->addStretch(1);
    colRow->addWidget(addBtn);
    layout->addLayout(colRow);

    // ── 集合树 ──
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_tree, 1);

    connect(m_tree, &QTreeWidget::itemClicked, this, &CollectionTree::onItemClicked);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &CollectionTree::onCustomContextMenu);
    connect(envBtn, &QPushButton::clicked, this, &CollectionTree::manageEnvironments);
    connect(addBtn, &QPushButton::clicked, this, &CollectionTree::addCollection);
    connect(m_envCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CollectionTree::onEnvChanged);

    connect(CollectionStore::instance(), &CollectionStore::changed, this, &CollectionTree::reload);
    connect(EnvironmentStore::instance(), &EnvironmentStore::changed,
            this, &CollectionTree::reloadEnvironments);
    connect(EnvironmentStore::instance(), &EnvironmentStore::activeChanged,
            this, &CollectionTree::reloadEnvironments);

    reload();
    reloadEnvironments();
}

void CollectionTree::populateChildren(QTreeWidgetItem *parent,
                                      const QVector<CollectionItem> &items,
                                      const QString &collectionId) {
    for (const CollectionItem &it : items) {
        auto *item = new QTreeWidgetItem(parent);
        if (it.isFolder) {
            item->setText(0, it.name);
            QFont f = item->font(0);
            f.setBold(true);
            item->setFont(0, f);
            m_keyOf.insert(item, qMakePair(collectionId, it.id));
            populateChildren(item, it.children, collectionId);
        } else {
            item->setText(0, QStringLiteral("%1  %2").arg(it.method).arg(it.name));
            item->setForeground(0, methodColor(it.method));
            item->setData(0, Qt::UserRole, it.method); // 请求行标记
            m_keyOf.insert(item, qMakePair(collectionId, it.id));
        }
    }
}

void CollectionTree::reload() {
    m_keyOf.clear();
    m_tree->clear();
    const QVector<Collection> &cols = CollectionStore::instance()->collections();
    for (const Collection &col : cols) {
        auto *top = new QTreeWidgetItem(m_tree);
        top->setText(0, col.name);
        QFont f = top->font(0);
        f.setBold(true);
        f.setPointSize(f.pointSize() + 1);
        top->setFont(0, f);
        m_keyOf.insert(top, qMakePair(col.id, QString()));
        populateChildren(top, col.items, col.id);
        top->setExpanded(true);
    }
}

void CollectionTree::reloadEnvironments() {
    const QString active = EnvironmentStore::instance()->activeEnvironmentId();
    const QSignalBlocker blocker(m_envCombo);
    m_envCombo->clear();
    m_envCombo->addItem(QStringLiteral("（无环境）"), QString());
    for (const Environment &env : EnvironmentStore::instance()->environments())
        m_envCombo->addItem(env.name, env.id);
    const int idx = m_envCombo->findData(active);
    m_envCombo->setCurrentIndex(idx < 0 ? 0 : idx);
}

void CollectionTree::onEnvChanged(int index) {
    EnvironmentStore::instance()->setActiveEnvironment(m_envCombo->itemData(index).toString());
}

void CollectionTree::onItemClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item || !m_keyOf.contains(item)) return;
    const auto key = m_keyOf.value(item);
    const CollectionItem *ci = CollectionStore::instance()->findItem(key.first, key.second);
    if (!ci || ci->isFolder) return;
    emit requestActivated(ci->method, ci->url, ci->body);
}

void CollectionTree::onCustomContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = m_tree->itemAt(pos);
    if (item) showContextMenu(item, m_tree->viewport()->mapToGlobal(pos));
}

void CollectionTree::showContextMenu(QTreeWidgetItem *item, const QPoint &globalPos) {
    if (!m_keyOf.contains(item)) return;
    const auto key = m_keyOf.value(item);
    const QString collectionId = key.first;
    const QString itemId = key.second;
    CollectionStore *store = CollectionStore::instance();
    const bool isRequest = !itemId.isEmpty()
        && store->findItem(collectionId, itemId)
        && !store->findItem(collectionId, itemId)->isFolder;
    const bool isFolder = !itemId.isEmpty() && !isRequest;

    QMenu menu(this);
    if (!itemId.isEmpty()) {
        QAction *newFolder = menu.addAction(QStringLiteral("新建文件夹"));
        QAction *newReq = menu.addAction(QStringLiteral("新建请求"));
        QAction *rename = menu.addAction(QStringLiteral("重命名"));
        QAction *del = menu.addAction(QStringLiteral("删除"));
        QAction *chosen = menu.exec(globalPos);
        if (!chosen) return;
        if (chosen == newFolder) {
            bool ok = false;
            const QString name = QInputDialog::getText(this, QStringLiteral("新建文件夹"),
                QStringLiteral("文件夹名称："), QLineEdit::Normal, QString(), &ok);
            if (ok && !name.trimmed().isEmpty())
                store->addFolder(collectionId, itemId, name.trimmed());
        } else if (chosen == newReq) {
            bool ok = false;
            const QString name = QInputDialog::getText(this, QStringLiteral("新建请求"),
                QStringLiteral("请求名称："), QLineEdit::Normal, QStringLiteral("新请求"), &ok);
            if (ok && !name.trimmed().isEmpty())
                store->addRequest(collectionId, itemId, name.trimmed(),
                                  {QStringLiteral("GET"), QString(), QString()});
        } else if (chosen == rename) {
            const QString cur = store->findItem(collectionId, itemId)->name;
            bool ok = false;
            const QString name = QInputDialog::getText(this, QStringLiteral("重命名"),
                QStringLiteral("新名称："), QLineEdit::Normal, cur, &ok);
            if (ok && !name.trimmed().isEmpty())
                store->renameItem(collectionId, itemId, name.trimmed());
        } else if (chosen == del) {
            const auto ret = QMessageBox::question(this, QStringLiteral("确认删除"),
                QStringLiteral("确定删除「%1」？").arg(item->text(0)),
                QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) store->removeItem(collectionId, itemId);
        }
        return;
    }

    // 集合节点：新建文件夹 / 新建请求 / 重命名 / 删除集合
    QAction *newFolder = menu.addAction(QStringLiteral("新建文件夹"));
    QAction *newReq = menu.addAction(QStringLiteral("新建请求"));
    QAction *rename = menu.addAction(QStringLiteral("重命名"));
    QAction *del = menu.addAction(QStringLiteral("删除集合"));
    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;
    if (chosen == newFolder) {
        bool ok = false;
        const QString name = QInputDialog::getText(this, QStringLiteral("新建文件夹"),
            QStringLiteral("文件夹名称："), QLineEdit::Normal, QString(), &ok);
        if (ok && !name.trimmed().isEmpty())
            store->addFolder(collectionId, QString(), name.trimmed());
    } else if (chosen == newReq) {
        bool ok = false;
        const QString name = QInputDialog::getText(this, QStringLiteral("新建请求"),
            QStringLiteral("请求名称："), QLineEdit::Normal, QStringLiteral("新请求"), &ok);
        if (ok && !name.trimmed().isEmpty())
            store->addRequest(collectionId, QString(), name.trimmed(),
                              {QStringLiteral("GET"), QString(), QString()});
    } else if (chosen == rename) {
        bool ok = false;
        const QString name = QInputDialog::getText(this, QStringLiteral("重命名"),
            QStringLiteral("集合名称："), QLineEdit::Normal, item->text(0), &ok);
        if (ok && !name.trimmed().isEmpty())
            store->renameCollection(collectionId, name.trimmed());
    } else if (chosen == del) {
        const auto ret = QMessageBox::question(this, QStringLiteral("确认删除"),
            QStringLiteral("确定删除集合「%1」及其全部内容？").arg(item->text(0)),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) store->removeCollection(collectionId);
    }
}

void CollectionTree::manageEnvironments() {
    EnvironmentDialog dlg(this);
    dlg.exec();
    reloadEnvironments();
}

void CollectionTree::addCollection() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("新建集合"),
        QStringLiteral("集合名称："), QLineEdit::Normal, QString(), &ok);
    if (ok && !name.trimmed().isEmpty())
        CollectionStore::instance()->addCollection(name.trimmed());
}
