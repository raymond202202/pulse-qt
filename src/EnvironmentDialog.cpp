#include "EnvironmentDialog.h"
#include "EnvironmentStore.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QInputDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>

EnvironmentDialog::EnvironmentDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("环境管理"));
    resize(620, 420);

    auto *layout = new QHBoxLayout(this);

    // ── 左侧：环境列表 ──
    auto *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(new QLabel(QStringLiteral("环境"), this));
    m_envList = new QListWidget(this);
    leftLayout->addWidget(m_envList, 1);
    auto *envAdd = new QPushButton(QStringLiteral("新建环境"), this);
    auto *envRename = new QPushButton(QStringLiteral("重命名"), this);
    auto *envDel = new QPushButton(QStringLiteral("删除环境"), this);
    envAdd->setObjectName("copyBtn");
    envRename->setObjectName("copyBtn");
    envDel->setObjectName("copyBtn");
    leftLayout->addWidget(envAdd);
    leftLayout->addWidget(envRename);
    leftLayout->addWidget(envDel);
    layout->addLayout(leftLayout, 2);

    // ── 右侧：变量表 ──
    auto *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(new QLabel(QStringLiteral("变量（URL/Body 中可用 {{变量名}} 引用）"), this));
    m_varTable = new QTableWidget(0, 3, this);
    m_varTable->setHorizontalHeaderLabels({QStringLiteral("启用"), QStringLiteral("变量名"), QStringLiteral("值")});
    m_varTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_varTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_varTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_varTable->verticalHeader()->setVisible(false);
    rightLayout->addWidget(m_varTable, 1);
    auto *varRow = new QHBoxLayout;
    auto *varAdd = new QPushButton(QStringLiteral("添加变量"), this);
    auto *varDel = new QPushButton(QStringLiteral("删除选中行"), this);
    varAdd->setObjectName("copyBtn");
    varDel->setObjectName("copyBtn");
    varRow->addWidget(varAdd);
    varRow->addWidget(varDel);
    varRow->addStretch(1);
    rightLayout->addLayout(varRow);
    layout->addLayout(rightLayout, 3);

    connect(envAdd, &QPushButton::clicked, this, &EnvironmentDialog::addEnvironment);
    connect(envRename, &QPushButton::clicked, this, &EnvironmentDialog::renameEnvironment);
    connect(envDel, &QPushButton::clicked, this, &EnvironmentDialog::removeEnvironment);
    connect(m_envList, &QListWidget::currentRowChanged,
            this, [this](int) { onEnvSelected(); });
    connect(varAdd, &QPushButton::clicked, this, &EnvironmentDialog::addVariable);
    connect(varDel, &QPushButton::clicked, this, &EnvironmentDialog::removeVariable);
    connect(m_varTable, &QTableWidget::cellChanged, this, &EnvironmentDialog::onCellChanged);

    reloadEnvironments();
}

void EnvironmentDialog::reloadEnvironments() {
    const QSignalBlocker blocker(m_envList);
    m_envList->clear();
    for (const Environment &env : EnvironmentStore::instance()->environments())
        m_envList->addItem(env.name);
    if (m_envList->count() > 0) {
        m_envList->setCurrentRow(0);
        onEnvSelected();
    } else {
        m_currentEnvId.clear();
        m_varTable->setRowCount(0);
    }
}

void EnvironmentDialog::onEnvSelected() {
    const int row = m_envList->currentRow();
    if (row < 0) {
        m_currentEnvId.clear();
        m_varTable->setRowCount(0);
        return;
    }
    const QVector<Environment> &envs = EnvironmentStore::instance()->environments();
    if (row >= envs.size()) return;
    m_currentEnvId = envs[row].id;
    loadVariables(m_currentEnvId);
}

void EnvironmentDialog::loadVariables(const QString &envId) {
    const Environment *env = EnvironmentStore::instance()->environmentById(envId);
    m_loading = true;
    m_varTable->setRowCount(0);
    if (env) {
        m_varTable->setRowCount(env->variables.size());
        for (int i = 0; i < env->variables.size(); ++i) {
            const EnvVar &v = env->variables[i];
            auto *check = new QTableWidgetItem();
            check->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            check->setCheckState(v.enabled ? Qt::Checked : Qt::Unchecked);
            m_varTable->setItem(i, 0, check);
            m_varTable->setItem(i, 1, new QTableWidgetItem(v.key));
            m_varTable->setItem(i, 2, new QTableWidgetItem(v.value));
        }
    }
    m_loading = false;
}

void EnvironmentDialog::saveVariablesFromTable(const QString &envId) {
    if (m_loading || envId.isEmpty()) return;
    const int rows = m_varTable->rowCount();
    EnvironmentStore *store = EnvironmentStore::instance();
    // 先清空该环境所有变量，再按表重建（保证行序一致）
    const Environment *env = store->environmentById(envId);
    if (!env) return;
    for (int i = env->variables.size() - 1; i >= 0; --i) store->removeVariable(envId, i);
    for (int i = 0; i < rows; ++i) {
        store->addVariable(envId);
        const QString key = m_varTable->item(i, 1) ? m_varTable->item(i, 1)->text() : QString();
        const QString value = m_varTable->item(i, 2) ? m_varTable->item(i, 2)->text() : QString();
        const bool enabled = m_varTable->item(i, 0)
            ? m_varTable->item(i, 0)->checkState() == Qt::Checked : true;
        store->updateVariable(envId, i, key, value, enabled);
    }
}

void EnvironmentDialog::addEnvironment() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("新建环境"),
        QStringLiteral("环境名称："), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    EnvironmentStore::instance()->addEnvironment(name.trimmed());
    reloadEnvironments();
    const QVector<Environment> &envs = EnvironmentStore::instance()->environments();
    for (int i = envs.size() - 1; i >= 0; --i) {
        if (envs[i].name == name.trimmed()) { m_envList->setCurrentRow(i); break; }
    }
}

void EnvironmentDialog::renameEnvironment() {
    const int row = m_envList->currentRow();
    if (row < 0) return;
    const QVector<Environment> &envs = EnvironmentStore::instance()->environments();
    if (row >= envs.size()) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("重命名环境"),
        QStringLiteral("环境名称："), QLineEdit::Normal, envs[row].name, &ok);
    if (ok && !name.trimmed().isEmpty()) {
        EnvironmentStore::instance()->renameEnvironment(envs[row].id, name.trimmed());
        reloadEnvironments();
    }
}

void EnvironmentDialog::removeEnvironment() {
    const int row = m_envList->currentRow();
    if (row < 0) return;
    const QVector<Environment> &envs = EnvironmentStore::instance()->environments();
    if (row >= envs.size()) return;
    const auto ret = QMessageBox::question(this, QStringLiteral("确认删除"),
        QStringLiteral("确定删除环境「%1」？").arg(envs[row].name),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        EnvironmentStore::instance()->removeEnvironment(envs[row].id);
        reloadEnvironments();
    }
}

void EnvironmentDialog::addVariable() {
    if (m_currentEnvId.isEmpty()) return;
    saveVariablesFromTable(m_currentEnvId);
    EnvironmentStore::instance()->addVariable(m_currentEnvId);
    loadVariables(m_currentEnvId);
    m_varTable->setCurrentCell(m_varTable->rowCount() - 1, 1);
}

void EnvironmentDialog::removeVariable() {
    if (m_currentEnvId.isEmpty()) return;
    const int row = m_varTable->currentRow();
    if (row < 0) return;
    saveVariablesFromTable(m_currentEnvId);
    EnvironmentStore::instance()->removeVariable(m_currentEnvId, row);
    loadVariables(m_currentEnvId);
}

void EnvironmentDialog::onCellChanged(int row, int column) {
    Q_UNUSED(row);
    Q_UNUSED(column);
    saveVariablesFromTable(m_currentEnvId);
}
