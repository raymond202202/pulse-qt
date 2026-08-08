#include "SaveToCollectionDialog.h"
#include "CollectionStore.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QUrl>

namespace {
// 递归收集文件夹路径，扁平化为 "集合名 / 文件夹 / 子文件夹"
void collectFolders(const QVector<CollectionItem> &items, const QString &prefix,
                    QComboBox *combo, const QString &collectionId) {
    for (const CollectionItem &it : items) {
        if (!it.isFolder) continue;
        const QString label = prefix.isEmpty() ? it.name : prefix + " / " + it.name;
        combo->addItem("📁 " + label, QVariantList{collectionId, it.id});
        collectFolders(it.children, label, combo, collectionId);
    }
}
} // namespace

SaveToCollectionDialog::SaveToCollectionDialog(const QString &method, const QString &url,
                                               const QString &body, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("保存到集合"));
    setMinimumWidth(420);

    auto *layout = new QVBoxLayout(this);

    auto *form = new QFormLayout;
    m_targetCombo = new QComboBox(this);
    const QVector<Collection> &cols = CollectionStore::instance()->collections();
    for (const Collection &col : cols) {
        m_targetCombo->addItem("📁 " + col.name, QVariantList{col.id, QString()});
        collectFolders(col.items, col.name, m_targetCombo, col.id);
    }
    m_nameEdit = new QLineEdit(this);
    Q_UNUSED(method);
    QString hint = url;
    if (hint.isEmpty()) hint = QStringLiteral("新请求");
    else {
        const QUrl u(hint);
        if (u.isValid() && !u.path().isEmpty())
            hint = u.path().section('/', -1);
        if (hint.isEmpty()) hint = u.host();
        if (hint.isEmpty()) hint = QStringLiteral("新请求");
    }
    m_nameEdit->setText(hint);
    form->addRow(QStringLiteral("目标位置："), m_targetCombo);
    form->addRow(QStringLiteral("请求名称："), m_nameEdit);
    layout->addLayout(form);

    if (cols.isEmpty()) {
        auto *tip = new QLabel(QStringLiteral("还没有集合，请先在左侧新建集合"), this);
        tip->setWordWrap(true);
        layout->addWidget(tip);
    }

    auto *btnRow = new QHBoxLayout;
    auto *ok = new QPushButton(QStringLiteral("保存"), this);
    auto *cancel = new QPushButton(QStringLiteral("取消"), this);
    ok->setObjectName("sendButton");
    ok->setEnabled(!cols.isEmpty());
    btnRow->addStretch(1);
    btnRow->addWidget(ok);
    btnRow->addWidget(cancel);
    layout->addLayout(btnRow);

    connect(ok, &QPushButton::clicked, this, [this]() {
        const QVariantList data = m_targetCombo->currentData().toList();
        if (data.size() == 2)
            m_target = qMakePair(data[0].toString(), data[1].toString());
        accept();
    });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
}

QString SaveToCollectionDialog::requestName() const {
    const QString name = m_nameEdit->text().trimmed();
    return name.isEmpty() ? QStringLiteral("新请求") : name;
}
