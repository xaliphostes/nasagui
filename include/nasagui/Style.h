#pragma once

class QApplication;

namespace nasagui {

// Apply the dark mission-control palette, fonts and stylesheet app-wide.
// Call once, right after constructing the QApplication.
void applyTheme(QApplication &app);

} // namespace nasagui
