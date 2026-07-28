#pragma once

#include "SceneObject.h"

#include <QList>
#include <QWidget>

class QTreeWidget;
class QVTKOpenGLNativeWidget;
class vtkGenericOpenGLRenderWindow;
class vtkRenderer;

namespace nasagui {
class HudButton;
class HudCheckBox;
class HudComboBox;
class HudLabel;
class HudSlider;
} // namespace nasagui

// Main window: scene tree (left dock) + VTK viewport + properties (right dock).
class VtkExplorerWindow : public QWidget
{
    Q_OBJECT
public:
    VtkExplorerWindow();
    ~VtkExplorerWindow() override;

    void loadFile(const QString &path);
    void saveLayoutNow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    QWidget *buildStyleSelector();
    QWidget *buildModelOpsPanel();
    QWidget *buildPropertiesPanel();
    void applyThemeColors();
    void buildInitialScene();
    void addObject(const QString &name, vtkSmartPointer<vtkPolyData> poly);
    void clearModel();
    void applyProperties(SceneObject &obj);
    void applyAllProperties();
    void syncPropertiesPanel();
    void render();

    // Run an edit on the selected object, re-apply and render.
    template <typename F>
    void withCurrent(F &&edit)
    {
        if (m_syncing || m_current < 0 || m_current >= m_objects.size())
            return;
        edit(m_objects[m_current]);
        applyProperties(m_objects[m_current]);
        render();
    }

    QTreeWidget *m_tree = nullptr;
    QVTKOpenGLNativeWidget *m_vtkWidget = nullptr;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    QList<SceneObject> m_objects;
    int m_current = -1;
    bool m_syncing = false;

    nasagui::HudLabel *m_title = nullptr;
    nasagui::HudButton *m_clearButton = nullptr;
    nasagui::HudLabel *m_objName = nullptr;
    nasagui::HudCheckBox *m_visible = nullptr;
    nasagui::HudButton *m_colorButton = nullptr;
    nasagui::HudComboBox *m_representation = nullptr;
    nasagui::HudComboBox *m_attribute = nullptr;
    nasagui::HudComboBox *m_colorMap = nullptr;
    nasagui::HudSlider *m_opacity = nullptr;
};
