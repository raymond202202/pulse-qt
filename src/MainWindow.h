#pragma once

#include <QMainWindow>

class RequestPanel;
class ResponsePanel;
class CollectionTree;
class HistoryList;
class AiPanel;
class SaveToCollectionDialog;
class QTabWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void openSaveToCollection();

private:
    RequestPanel *m_request = nullptr;
    ResponsePanel *m_response = nullptr;
    CollectionTree *m_collections = nullptr;
    HistoryList *m_history = nullptr;
    AiPanel *m_ai = nullptr;
    QTabWidget *m_leftTabs = nullptr;
    QTabWidget *m_rightTabs = nullptr;
    SaveToCollectionDialog *m_saveDialog = nullptr;
};
