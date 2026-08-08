#include "MainWindow.h"
#include "RequestPanel.h"
#include "ResponsePanel.h"
#include "CollectionTree.h"
#include "HistoryList.h"
#include "CollectionStore.h"
#include "HistoryStore.h"
#include "SaveToCollectionDialog.h"
#include <QSplitter>
#include <QTabWidget>
#include <QStatusBar>
#include <QLabel>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("pulse-qt"));
    resize(1400, 850);

    // ── 浅色主题：白底紫配 #6d4aff ──
    qApp->setStyleSheet(QStringLiteral(R"(
        QMainWindow, QDialog { background-color: #fafafa; }
        QLabel { background: transparent; }
        QSplitter::handle { background: #e6e6f2; width: 1px; }
        QLineEdit, QComboBox, QPlainTextEdit, QTreeView, QTableView, QListWidget {
            background: #ffffff; color: #2c2c2c;
            border: 1px solid #d8d8ea; border-radius: 4px; padding: 3px;
            selection-background-color: #6d4aff; selection-color: #ffffff;
        }
        QLineEdit:focus, QComboBox:focus, QPlainTextEdit:focus, QTreeView:focus {
            border-color: #6d4aff;
        }
        QTreeWidget::item { padding: 2px; }
        QTreeWidget::item:selected { background: #ede9ff; color: #2c2c2c; border-radius: 3px; }
        QPushButton {
            background: #ffffff; color: #2c2c2c;
            border: 1px solid #d0d0e8; border-radius: 4px; padding: 4px 14px;
        }
        QPushButton:hover { background: #f0eeff; border-color: #6d4aff; }
        QPushButton:pressed { background: #e4e0ff; }
        QPushButton:checked {
            background: #6d4aff; color: #ffffff; border-color: #6d4aff; font-weight: 600;
        }
        QPushButton:disabled { color: #a8a8b8; background: #f2f2f6; }
        QPushButton#sendButton {
            background: #6d4aff; color: #ffffff; border: none; border-radius: 4px;
            padding: 5px 18px; font-weight: 600;
        }
        QPushButton#sendButton:hover { background: #5a3ce0; }
        QPushButton#copyBtn { padding: 2px 10px; }
        QPushButton#viewBtn { padding: 2px 12px; }
        QStatusBar { background: #f4f3fb; color: #555555; }
        QStatusBar QLabel { color: #555555; }
        QComboBox::drop-down { border: none; width: 18px; }
        QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
        QScrollBar::handle:vertical { background: #c9c9de; border-radius: 5px; min-height: 24px; }
        QScrollBar::handle:vertical:hover { background: #6d4aff; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }
        QScrollBar::handle:horizontal { background: #c9c9de; border-radius: 5px; min-width: 24px; }
        QScrollBar::handle:horizontal:hover { background: #6d4aff; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
        QMenu { background: #ffffff; border: 1px solid #d8d8ea; border-radius: 4px; padding: 4px; }
        QMenu::item { padding: 5px 22px; border-radius: 3px; }
        QMenu::item:selected { background: #ede9ff; }
        QHeaderView::section {
            background: #f4f3fb; color: #555555; border: none;
            border-bottom: 1px solid #d8d8ea; padding: 4px;
        }
    )"));

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // 左栏：集合 / 历史 页签
    m_leftTabs = new QTabWidget(this);
    m_leftTabs->setMinimumWidth(230);
    m_collections = new CollectionTree(this);
    m_history = new HistoryList(this);
    m_leftTabs->addTab(m_collections, QStringLiteral("集合"));
    m_leftTabs->addTab(m_history, QStringLiteral("历史"));
    splitter->addWidget(m_leftTabs);

    m_request = new RequestPanel(this);
    splitter->addWidget(m_request);

    m_response = new ResponsePanel(this);
    splitter->addWidget(m_response);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 4);
    splitter->setStretchFactor(2, 3);
    setCentralWidget(splitter);

    // 集合请求 → 请求区回填
    connect(m_collections, &CollectionTree::requestActivated,
            m_request, &RequestPanel::loadRequest);
    // 历史点击 → 请求区回填
    connect(m_history, &HistoryList::requestActivated,
            m_request, &RequestPanel::loadRequest);
    // 发送完成后自动记录历史（存发送时的原始值 + 响应状态/耗时）
    connect(m_request, &RequestPanel::responseReceived, this,
            [this](int status, qint64 msec, const QByteArray &) {
        HistoryStore::instance()->addEntry(
            m_request->sentMethod(), m_request->sentUrl(), m_request->sentBody(),
            status, msec);
    });
    // 保存到集合
    connect(m_request, &RequestPanel::saveToCollectionRequested,
            this, &MainWindow::openSaveToCollection);

    statusBar()->addWidget(new QLabel(QStringLiteral("pulse-qt v%1").arg(APP_VERSION), this));
}

void MainWindow::openSaveToCollection() {
    m_saveDialog = new SaveToCollectionDialog(
        m_request->method(), m_request->url(), m_request->bodyText(), this);
    m_saveDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_saveDialog, &QDialog::accepted, this, [this]() {
        const auto target = m_saveDialog->target();
        CollectionStore::instance()->addRequest(
            target.first, target.second, m_saveDialog->requestName(),
            {m_request->method(), m_request->url(), m_request->bodyText()});
    });
    m_saveDialog->open();
}
