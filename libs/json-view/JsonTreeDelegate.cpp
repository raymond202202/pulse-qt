#include "JsonTreeDelegate.h"
#include "JsonTreeModel.h"

#include <QFontMetrics>
#include <QPainter>
#include <QStyleOptionViewItem>

JsonTreeDelegate::JsonTreeDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
    setThemeColors(false);
}

void JsonTreeDelegate::setThemeColors(bool dark)
{
    m_dark = dark;
    if (dark) {
        m_keyColor = QColor("#89b4fa");
        m_stringColor = QColor("#a6e3a1");
        m_numberColor = QColor("#fab387");
        m_boolColor = QColor("#cba6f7");
        m_nullColor = QColor("#f38ba8");
        m_bracketColor = QColor("#f9e2af");
        m_placeholderColor = QColor("#a6adc8");
        m_textColor = QColor("#cdd6f4");
        m_hoverBg = QColor("#313155");
        m_matchBg = QColor(137, 180, 250, 38);
        m_activeBg = QColor(137, 180, 250, 60);
        m_matchBorder = QColor(137, 180, 250, 102);
    } else {
        m_keyColor = QColor("#0550ae");
        m_stringColor = QColor("#0a6e2e");
        m_numberColor = QColor("#b35900");
        m_boolColor = QColor("#6e3fa0");
        m_nullColor = QColor("#c93c3c");
        m_bracketColor = QColor("#555555");
        m_placeholderColor = QColor("#888888");
        m_textColor = QColor("#2c2c2c");
        m_hoverBg = QColor("#e8e8f0");
        m_matchBg = QColor(9, 105, 218, 30);
        m_activeBg = QColor(9, 105, 218, 60);
        m_matchBorder = QColor(9, 105, 218, 128);
    }
}

void JsonTreeDelegate::drawTextSegment(QPainter *p, const QFontMetrics &fm,
                                       int &x, int y, const QString &text,
                                       const QColor &color) const
{
    if (text.isEmpty()) return;
    p->setPen(color);
    p->drawText(x, y, text);
    x += fm.horizontalAdvance(text);
}

void JsonTreeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (!index.isValid()) return;

    const QStyleOptionViewItem opt = option;
    painter->save();
    painter->setClipRect(opt.rect);

    const bool selected = opt.state & QStyle::State_Selected;
    const bool hovered = opt.state & QStyle::State_MouseOver;
    const QString path = index.data(JsonTreeModel::PathRole).toString();
    const bool isMatch = m_searchActive && !m_matchedPath.isEmpty() && path == m_matchedPath;
    const bool isActive = !m_activePath.isEmpty() && path == m_activePath;

    // 背景
    QRectF bgRect = QRectF(opt.rect).adjusted(1, 1, -1, -1);
    if (selected || isMatch || isActive) {
        painter->fillRect(bgRect, m_matchBg);
        painter->setPen(QPen(m_matchBorder, 1));
        painter->drawRoundedRect(bgRect, 3, 3);
    } else if (hovered) {
        painter->fillRect(bgRect, m_hoverBg);
    }

    const int type = index.data(JsonTreeModel::TypeRole).toInt();
    const QString key = index.data(JsonTreeModel::KeyRole).toString();
    const QString valueText = index.data(JsonTreeModel::ValueTextRole).toString();
    // 真实子项数（含未加载/截断部分），与 Object/Array 显示一致
    const int childCount = index.data(JsonTreeModel::ChildCountRole).toInt();
    const bool isContainer = (type == JsonTreeModel::Object || type == JsonTreeModel::Array);

    // 文本颜色（Truncated 整行 placeholder 色）
    QColor textColor = m_textColor;
    if (type == JsonTreeModel::Truncated) textColor = m_placeholderColor;

    painter->setFont(opt.font);
    const QFontMetrics fm(opt.font);
    int x = opt.rect.left() + 6;
    const int y = opt.rect.top() + (opt.rect.height() - fm.ascent() - fm.descent()) / 2 + fm.ascent();

    const bool isRoot = (type == JsonTreeModel::Root);
    if (isRoot) {
        // 根行：{ N 项 } 或 [ N 项 ]
        drawTextSegment(painter, fm, x, y, type == JsonTreeModel::Object ? "{" : "[",
                        m_bracketColor);
        drawTextSegment(painter, fm, x, y, QString(" %1 项 ").arg(childCount),
                        m_placeholderColor);
        drawTextSegment(painter, fm, x, y, type == JsonTreeModel::Object ? "}" : "]",
                        m_bracketColor);
        painter->restore();
        return;
    }

    // key 段
    if (!key.isEmpty() && type != JsonTreeModel::Truncated) {
        drawTextSegment(painter, fm, x, y, "\"" + key + "\": ", m_keyColor);
    }

    if (type == JsonTreeModel::Truncated) {
        drawTextSegment(painter, fm, x, y, key, m_placeholderColor);
    } else if (isContainer) {
        drawTextSegment(painter, fm, x, y,
                        type == JsonTreeModel::Object ? "{" : "[", m_bracketColor);
        drawTextSegment(painter, fm, x, y, QString(" %1 项 ").arg(childCount),
                        m_placeholderColor);
        drawTextSegment(painter, fm, x, y,
                        type == JsonTreeModel::Object ? "}" : "]", m_bracketColor);
    } else {
        // 叶子
        QColor vc = m_textColor;
        QFont f = opt.font;
        switch (type) {
        case JsonTreeModel::String:   vc = m_stringColor; break;
        case JsonTreeModel::Number:   vc = m_numberColor; break;
        case JsonTreeModel::Boolean:  vc = m_boolColor;   break;
        case JsonTreeModel::Null:     vc = m_nullColor; f.setItalic(true); break;
        default: break;
        }
        painter->setFont(f);
        drawTextSegment(painter, fm, x, y, valueText, vc);
    }

    painter->restore();
}

QSize JsonTreeDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    Q_UNUSED(index);
    const QFontMetrics fm(option.font);
    return QSize(200, fm.height() + 8);
}
