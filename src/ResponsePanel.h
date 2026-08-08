#pragma once

#include <QWidget>

class QLabel;
class QPlainTextEdit;

class ResponsePanel : public QWidget {
    Q_OBJECT
public:
    explicit ResponsePanel(QWidget *parent = nullptr);

public slots:
    void showResponse(int status, qint64 msec, const QByteArray &body);

private:
    QLabel *m_meta = nullptr;
    QPlainTextEdit *m_body = nullptr;
};
