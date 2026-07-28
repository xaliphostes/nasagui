// nasagui VTK demo: 3D viewport + scene tree + per-object properties panel.
// Loads VTK meshes (*.vtk *.vtp *.ply *.obj) and Gocad TSurf (*.ts) files.

#include "VtkExplorerWindow.h"

#include <nasagui/LayoutStore.h>
#include <nasagui/Style.h>

#include <QApplication>
#include <QSettings>
#include <QSurfaceFormat>
#include <QTimer>

#include <QVTKOpenGLNativeWidget.h>

int main(int argc, char *argv[])
{
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
    QApplication app(argc, argv);
    QApplication::setOrganizationName("nasagui");
    QApplication::setApplicationName("vtk-explorer");

    // Style: --theme <name> wins over the one remembered from the last run.
    // (Qt itself swallows "--style", hence the different spelling.)
    {
        const QStringList args = app.arguments();
        const int styleIdx = args.indexOf("--theme");
        const QString name = styleIdx >= 0 && styleIdx + 1 < args.size()
            ? args.at(styleIdx + 1)
            : QSettings().value("style").toString();
        nasagui::applyTheme(app, nasagui::Theme::styleFromName(name));
    }

    VtkExplorerWindow window;
    {
        QSettings settings;
        if (app.arguments().contains("--reset-layout"))
            settings.remove("layout");
        else
            nasagui::restoreLayout(settings, &window);
    }
    window.show();

    // Extra CLI arguments are loaded as models (also: --snapshot <file.png>)
    const QStringList args = app.arguments();
    const int snapIdx = args.indexOf("--snapshot");
    for (int i = 1; i < args.size(); ++i) {
        // Skip options and their values ("--snapshot out.png", "--theme X").
        if (args[i].startsWith("--")) {
            if (args[i] == "--snapshot" || args[i] == "--theme")
                ++i;
            continue;
        }
        window.loadFile(args[i]);
    }
    if (snapIdx >= 0 && snapIdx + 1 < args.size()) {
        const QString path = args.at(snapIdx + 1);
        QTimer::singleShot(2500, &app, [&window, path] {
            window.grab().save(path);
            window.saveLayoutNow();   // quit() skips closeEvent
            QApplication::quit();
        });
    }
    return app.exec();
}
