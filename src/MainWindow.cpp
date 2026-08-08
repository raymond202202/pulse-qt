#include "MainWindow.h"
#include "RequestPanel.h"
#include "ResponsePanel.h"
#include <QSplitter>
#include <QStatusBar>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("pulse-qt"));
    resize(1200, 800);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    m_request = new RequestPanel(this);
    m_response = new ResponsePanel(this);
    splitter->addWidget(m_request);
    splitter->addWidget(m_response);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    setCentralWidget(splitter);

    statusBar()->addWidget(new QLabel(QStringLiteral("pulse-qt v%1").arg(APP_VERSION), this));
}
