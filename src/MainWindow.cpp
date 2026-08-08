#include "MainWindow.h"
#include "RequestPanel.h"
#include "ResponsePanel.h"
#include <QSplitter>
#include <QStatusBar>
#include <QLabel>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("pulse-qt"));
    resize(1200, 800);

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
    )"));

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
