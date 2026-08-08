#pragma once

#include <QMainWindow>

class RequestPanel;
class ResponsePanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    RequestPanel *m_request = nullptr;
    ResponsePanel *m_response = nullptr;
};
