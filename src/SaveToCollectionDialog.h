#pragma once

#include <QDialog>
#include <QPair>

class QComboBox;
class QLineEdit;

// 保存当前请求到集合：选择目标（集合或集合内文件夹）+ 请求名称
class SaveToCollectionDialog : public QDialog {
    Q_OBJECT
public:
    SaveToCollectionDialog(const QString &method, const QString &url, const QString &body,
                           QWidget *parent = nullptr);

    // 返回 (collectionId, parentItemId)，parentItemId 空 = 集合顶层
    QPair<QString, QString> target() const { return m_target; }
    QString requestName() const;

private:
    QComboBox *m_targetCombo = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QPair<QString, QString> m_target;
};
