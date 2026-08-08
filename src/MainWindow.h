#pragma once

#include <QMainWindow>

class RequestPanel;
class ResponsePanel;
class CollectionTree;
class SaveToCollectionDialog;

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
    SaveToCollectionDialog *m_saveDialog = nullptr;
};
