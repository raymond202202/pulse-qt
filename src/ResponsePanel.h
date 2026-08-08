#pragma once

#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTreeView;
class QButtonGroup;
class QStackedWidget;
class JsonTreeModel;
class JsonTreeDelegate;

// 响应面板：元信息（状态/耗时/大小）+ 树形/文本双视图
// JSON 响应走 JsonTreeModel/Delegate 懒加载树，非 JSON 回落纯文本
class ResponsePanel : public QWidget {
    Q_OBJECT
public:
    explicit ResponsePanel(QWidget *parent = nullptr);

public slots:
    void showResponse(int status, qint64 msec, const QByteArray &body);

private slots:
    void switchView(int id);
    void copyBody();

private:
    void setViewEnabled(bool treeEnabled);

    QLabel *m_statusLabel = nullptr;
    QLabel *m_metaLabel = nullptr;
    QTreeView *m_tree = nullptr;
    QPlainTextEdit *m_text = nullptr;
    QStackedWidget *m_stack = nullptr;
    QButtonGroup *m_viewGroup = nullptr;
    JsonTreeModel *m_model = nullptr;
    JsonTreeDelegate *m_delegate = nullptr;
    QByteArray m_lastBody;
    QString m_lastPrettyJson;
    bool m_lastIsJson = false;
};
