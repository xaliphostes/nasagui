#pragma once

#include "nasagui/Theme.h"

class QApplication;

namespace nasagui {

// Apply the current style's palette, fonts and stylesheet app-wide.
// Call once, right after constructing the QApplication.
void applyTheme(QApplication &app);

// Same, starting from an explicit style.
void applyTheme(QApplication &app, Theme::Style style);

// Switch style at runtime: rewrites the Theme palette, re-applies the app
// palette/stylesheet, emits Theme::Notifier::styleChanged() and repaints every
// widget. Safe to call any time after applyTheme().
void setApplicationStyle(Theme::Style style);

} // namespace nasagui
