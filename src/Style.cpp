#include "nasagui/Style.h"
#include "nasagui/Theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QWidget>

namespace nasagui {

namespace {

QString hex(const QColor &c)
{
    return c.name(QColor::HexRgb);
}

QString rgba(const QColor &c, int alpha)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha);
}

// The field fill is a hair lighter/darker than the panel body so lists and
// alternating rows keep their separation in both styles.
QColor alternateFill()
{
    const QColor base = Theme::FieldFill;
    return Theme::style() == Theme::Style::Daylight ? base.darker(104)
                                                    : base.lighter(112);
}

void applyPalette(QApplication &app)
{
    QPalette pal;
    pal.setColor(QPalette::Window, Theme::Background);
    pal.setColor(QPalette::WindowText, Theme::TextPrimary);
    pal.setColor(QPalette::Base, Theme::FieldFill);
    pal.setColor(QPalette::AlternateBase, alternateFill());
    pal.setColor(QPalette::Text, Theme::TextPrimary);
    pal.setColor(QPalette::Button, Theme::FieldFill);
    pal.setColor(QPalette::ButtonText, Theme::TextPrimary);
    pal.setColor(QPalette::Highlight, Theme::Primary);
    pal.setColor(QPalette::HighlightedText, Theme::Background);
    pal.setColor(QPalette::ToolTipBase, Theme::FieldFill);
    pal.setColor(QPalette::ToolTipText, Theme::TextPrimary);
    pal.setColor(QPalette::PlaceholderText, Theme::TextDim);
    pal.setColor(QPalette::Disabled, QPalette::Text, Theme::TextDim);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, Theme::TextDim);
    app.setPalette(pal);
}

void applyStyleSheet(QApplication &app)
{
    QString qss = QStringLiteral(R"(
        QLabel { color: @text; background: transparent; }
        QToolTip {
            background-color: @field; color: @text;
            border: 1px solid @border; padding: 4px;
        }
        QScrollBar:vertical {
            background: @bg; width: 8px; margin: 0;
        }
        QScrollBar::handle:vertical {
            background: @border; min-height: 24px;
        }
        QScrollBar::handle:vertical:hover { background: @primary; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
        QScrollBar:horizontal {
            background: @bg; height: 8px; margin: 0;
        }
        QScrollBar::handle:horizontal {
            background: @border; min-width: 24px;
        }
        QScrollBar::handle:horizontal:hover { background: @primary; }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            background: @field; color: @text;
            border: 1px solid @border; padding: 4px 6px;
            selection-background-color: @primary;
            selection-color: @bg;
        }
        QLineEdit:focus, QComboBox:focus { border-color: @primary; }
        QComboBox QAbstractItemView {
            background-color: @field; border: 1px solid @primary;
            color: @text; padding: 2px; outline: 0;
            selection-background-color: @selected;
            selection-color: @primary;
        }
        QTreeView, QTreeWidget, QListView {
            background: transparent; border: none;
            color: @text; outline: 0;
        }
        QTreeView::item, QListView::item { padding: 4px 2px; }
        QTreeView::item:hover, QListView::item:hover {
            background: @hovered;
        }
        QTreeView::item:selected, QListView::item:selected {
            background: @selected; color: @primary;
        }
        QTreeView::branch { background: transparent; }
        QHeaderView::section {
            background: @field; color: @textdim;
            border: 1px solid @border; padding: 4px 6px;
        }
        QMenuBar {
            background-color: @bg; color: @text;
            border-bottom: 1px solid @border;
        }
        QMenuBar::item { padding: 6px 12px; background: transparent; }
        QMenuBar::item:selected {
            color: @primary; background: @tinted;
        }
        QMenu {
            background-color: @field; border: 1px solid @primary;
            color: @text; padding: 4px;
        }
        QMenu::item {
            padding: 6px 28px 6px 16px; background: transparent;
        }
        QMenu::item:selected {
            background: @selected; color: @primary;
        }
        QMenu::item:disabled { color: @textdim; }
        QMenu::separator { height: 1px; background: @border; margin: 4px 8px; }
        QMenu::indicator { width: 12px; height: 12px; left: 4px; }
        QMenu::indicator:checked {
            background: @primary; border: 1px solid @primary;
        }
        QMenu::indicator:unchecked {
            background: transparent; border: 1px solid @border;
        }
    )");

    // Longest tokens first: "@textdim" must be replaced before "@text".
    qss.replace(QLatin1String("@textdim"), hex(Theme::TextDim));
    qss.replace(QLatin1String("@text"), hex(Theme::TextPrimary));
    qss.replace(QLatin1String("@bg"), hex(Theme::Background));
    qss.replace(QLatin1String("@field"), hex(Theme::FieldFill));
    qss.replace(QLatin1String("@border"), hex(Theme::PanelBorder));
    qss.replace(QLatin1String("@primary"), hex(Theme::Primary));
    qss.replace(QLatin1String("@selected"), rgba(Theme::Primary, 40));
    qss.replace(QLatin1String("@hovered"), rgba(Theme::Primary, 15));
    qss.replace(QLatin1String("@tinted"), rgba(Theme::Primary, 20));

    app.setStyleSheet(qss);
}

} // namespace

void applyTheme(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));
    applyPalette(app);
    app.setFont(Theme::labelFont(10));
    applyStyleSheet(app);
}

void applyTheme(QApplication &app, Theme::Style style)
{
    Theme::setStyle(style);
    applyTheme(app);
}

void setApplicationStyle(Theme::Style style)
{
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app || style == Theme::style())
        return;

    Theme::setStyle(style);   // emits styleChanged()
    applyPalette(*app);
    applyStyleSheet(*app);

    for (QWidget *w : QApplication::allWidgets())
        w->update();
}

} // namespace nasagui
