#pragma once

#include <QDialog>

class QListWidget;
class QTableWidget;
class QPushButton;

// 环境管理对话框：左侧环境列表（增/删/改名），右侧变量表（启用/Key/Value）
class EnvironmentDialog : public QDialog {
    Q_OBJECT
public:
    explicit EnvironmentDialog(QWidget *parent = nullptr);

private slots:
    void addEnvironment();
    void renameEnvironment();
    void removeEnvironment();
    void onEnvSelected();
    void addVariable();
    void removeVariable();
    void onCellChanged(int row, int column);

private:
    void reloadEnvironments();
    void loadVariables(const QString &envId);
    void saveVariablesFromTable(const QString &envId);

    QListWidget *m_envList = nullptr;
    QTableWidget *m_varTable = nullptr;
    QString m_currentEnvId;
    bool m_loading = false;
};
