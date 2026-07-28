#pragma once

#include <QString>

class QSettings;
class QWidget;

namespace nasagui {

// Persists the UI layout under `group` in `settings`:
//   - QSplitter / HudSplitter positions
//   - CollapsibleDock expanded state + size
//   - HudPanel open/closed state
// Widgets are keyed by objectName (if set), else by title, plus their
// traversal index — so keep construction order stable, and give duplicated
// titles an objectName. Call restoreLayout() after the UI is fully built
// (menus wired), typically right before show(); call saveLayout() from
// closeEvent().
void saveLayout(QSettings &settings, const QWidget *root,
                const QString &group = QStringLiteral("layout"));
void restoreLayout(QSettings &settings, QWidget *root,
                   const QString &group = QStringLiteral("layout"));

} // namespace nasagui
