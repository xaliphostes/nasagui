#include "nasagui/LayoutStore.h"
#include "nasagui/CollapsibleDock.h"
#include "nasagui/HudPanel.h"

#include <QSettings>
#include <QSplitter>

namespace nasagui {

namespace {

QString widgetKey(const QWidget *w, const QString &fallback, int index)
{
    QString id = w->objectName().isEmpty() ? fallback : w->objectName();
    id.replace('/', '-');
    id.replace(' ', '-');
    return QStringLiteral("%1@%2").arg(id.toLower()).arg(index);
}

} // namespace

void saveLayout(QSettings &settings, const QWidget *root, const QString &group)
{
    settings.beginGroup(group);

    int i = 0;
    for (const HudPanel *panel : root->findChildren<HudPanel *>())
        settings.setValue("panel/" + widgetKey(panel, panel->title(), i++),
                          !panel->isHidden());

    i = 0;
    for (const CollapsibleDock *dock : root->findChildren<CollapsibleDock *>()) {
        const QString key = widgetKey(dock, dock->title(), i++);
        settings.setValue("dock/" + key + "/expanded", dock->isExpanded());
        settings.setValue("dock/" + key + "/size", dock->expandedSize());
    }

    i = 0;
    for (const QSplitter *splitter : root->findChildren<QSplitter *>())
        settings.setValue("splitter/" + widgetKey(splitter, "s", i++),
                          splitter->saveState());

    settings.endGroup();
}

void restoreLayout(QSettings &settings, QWidget *root, const QString &group)
{
    settings.beginGroup(group);

    // Panels first (their signals let docks auto-adjust), then dock state
    // overrides, then splitter geometry.
    int i = 0;
    for (HudPanel *panel : root->findChildren<HudPanel *>()) {
        const QVariant open =
            settings.value("panel/" + widgetKey(panel, panel->title(), i++));
        if (open.isValid())
            panel->setPanelOpen(open.toBool(), /*animate=*/false);
    }

    i = 0;
    for (CollapsibleDock *dock : root->findChildren<CollapsibleDock *>()) {
        const QString key = widgetKey(dock, dock->title(), i++);
        const QVariant size = settings.value("dock/" + key + "/size");
        if (size.isValid())
            dock->setExpandedSize(size.toInt());
        const QVariant expanded = settings.value("dock/" + key + "/expanded");
        if (expanded.isValid())
            dock->setExpandedImmediate(expanded.toBool());
    }

    i = 0;
    for (QSplitter *splitter : root->findChildren<QSplitter *>()) {
        const QVariant state =
            settings.value("splitter/" + widgetKey(splitter, "s", i++));
        if (state.isValid())
            splitter->restoreState(state.toByteArray());
    }

    settings.endGroup();
}

} // namespace nasagui
