#include "nasagui/Style.h"
#include "nasagui/Theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace nasagui {

void applyTheme(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette pal;
    pal.setColor(QPalette::Window, Theme::Background);
    pal.setColor(QPalette::WindowText, Theme::TextPrimary);
    pal.setColor(QPalette::Base, QColor(0x0a, 0x14, 0x1f));
    pal.setColor(QPalette::AlternateBase, QColor(0x0d, 0x1a, 0x28));
    pal.setColor(QPalette::Text, Theme::TextPrimary);
    pal.setColor(QPalette::Button, QColor(0x0a, 0x14, 0x1f));
    pal.setColor(QPalette::ButtonText, Theme::TextPrimary);
    pal.setColor(QPalette::Highlight, Theme::Primary);
    pal.setColor(QPalette::HighlightedText, Theme::Background);
    pal.setColor(QPalette::ToolTipBase, QColor(0x0a, 0x14, 0x1f));
    pal.setColor(QPalette::ToolTipText, Theme::TextPrimary);
    pal.setColor(QPalette::PlaceholderText, Theme::TextDim);
    pal.setColor(QPalette::Disabled, QPalette::Text, Theme::TextDim);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, Theme::TextDim);
    app.setPalette(pal);

    app.setFont(Theme::labelFont(10));

    app.setStyleSheet(R"(
        QLabel { color: #d7e7ef; background: transparent; }
        QToolTip {
            background-color: #0a141f; color: #d7e7ef;
            border: 1px solid #1d3a4d; padding: 4px;
        }
        QScrollBar:vertical {
            background: #060b12; width: 8px; margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #1d3a4d; min-height: 24px;
        }
        QScrollBar::handle:vertical:hover { background: #35d6ed; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
        QScrollBar:horizontal {
            background: #060b12; height: 8px; margin: 0;
        }
        QScrollBar::handle:horizontal {
            background: #1d3a4d; min-width: 24px;
        }
        QScrollBar::handle:horizontal:hover { background: #35d6ed; }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            background: #0a141f; color: #d7e7ef;
            border: 1px solid #1d3a4d; padding: 4px 6px;
            selection-background-color: #35d6ed;
            selection-color: #060b12;
        }
        QLineEdit:focus, QComboBox:focus { border-color: #35d6ed; }
        QComboBox QAbstractItemView {
            background-color: #0a141f; border: 1px solid #35d6ed;
            color: #d7e7ef; padding: 2px; outline: 0;
            selection-background-color: rgba(53, 214, 237, 40);
            selection-color: #35d6ed;
        }
        QMenuBar {
            background-color: #060b12; color: #d7e7ef;
            border-bottom: 1px solid #1d3a4d;
        }
        QMenuBar::item { padding: 6px 12px; background: transparent; }
        QMenuBar::item:selected {
            color: #35d6ed; background: rgba(53, 214, 237, 20);
        }
        QMenu {
            background-color: #0a141f; border: 1px solid #35d6ed;
            color: #d7e7ef; padding: 4px;
        }
        QMenu::item {
            padding: 6px 28px 6px 16px; background: transparent;
        }
        QMenu::item:selected {
            background: rgba(53, 214, 237, 40); color: #35d6ed;
        }
        QMenu::item:disabled { color: #6f8a99; }
        QMenu::separator { height: 1px; background: #1d3a4d; margin: 4px 8px; }
        QMenu::indicator { width: 12px; height: 12px; left: 4px; }
        QMenu::indicator:checked {
            background: #35d6ed; border: 1px solid #35d6ed;
        }
        QMenu::indicator:unchecked {
            background: transparent; border: 1px solid #1d3a4d;
        }
    )");
}

} // namespace nasagui
