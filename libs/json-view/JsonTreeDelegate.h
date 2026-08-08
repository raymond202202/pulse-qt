#pragma once

#include <QAbstractItemDelegate>
#include <QColor>

// JSON 树彩色渲染：key/value 分色（还原原版 CSS 配色）+ 搜索命中/激活高亮
class JsonTreeDelegate : public QAbstractItemDelegate {
    Q_OBJECT
public:
    explicit JsonTreeDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    // 主题配色（亮/暗两套，由主窗口注入）
    void setThemeColors(bool dark);
    // 搜索/激活状态
    void setMatchedPath(const QString &path) { m_matchedPath = path; }
    void setActivePath(const QString &path) { m_activePath = path; }
    void setSearchActive(bool active) { m_searchActive = active; }

private:
    void drawTextSegment(QPainter *p, const QFontMetrics &fm, int &x, int y,
                         const QString &text, const QColor &color) const;

    bool m_dark = false;
    QString m_matchedPath;
    QString m_activePath;
    bool m_searchActive = false;

    QColor m_keyColor, m_stringColor, m_numberColor, m_boolColor, m_nullColor,
           m_bracketColor, m_placeholderColor, m_textColor, m_hoverBg,
           m_matchBg, m_activeBg, m_matchBorder;
};
