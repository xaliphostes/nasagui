#include "VtkExplorerWindow.h"
#include "ColorMaps.h"
#include "MeshUtils.h"

#include <nasagui/CollapsibleDock.h>
#include <nasagui/Controls.h>
#include <nasagui/HudButton.h>
#include <nasagui/HudPanel.h>
#include <nasagui/LayoutStore.h>
#include <nasagui/Style.h>
#include <nasagui/Theme.h>

#include <QCloseEvent>
#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkDataArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkParametricFunctionSource.h>
#include <vtkParametricTorus.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

using namespace nasagui;

VtkExplorerWindow::VtkExplorerWindow()
{
    setWindowTitle("NASA-GUI // VTK Model Explorer");
    resize(1500, 880);
    buildUi();
    buildInitialScene();
}

VtkExplorerWindow::~VtkExplorerWindow() = default;

void VtkExplorerWindow::loadFile(const QString &path)
{
    QString error;
    vtkSmartPointer<vtkPolyData> poly = loadMeshFile(path, &error);
    if (!poly) {
        QMessageBox::warning(this, "Load failed",
                             error.isEmpty()
                                 ? QStringLiteral("%1: cannot load").arg(path)
                                 : error);
        return;
    }
    addObject(QFileInfo(path).completeBaseName(), poly);
    m_renderer->ResetCamera();
    render();
}

void VtkExplorerWindow::saveLayoutNow()
{
    QSettings settings;
    saveLayout(settings, this);
}

void VtkExplorerWindow::closeEvent(QCloseEvent *event)
{
    saveLayoutNow();
    QWidget::closeEvent(event);
}

void VtkExplorerWindow::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 18);
    root->setSpacing(10);

    // Header
    auto *header = new QHBoxLayout;
    m_title = new HudLabel("Model Explorer // VTK", HudLabel::Role::Title);
    m_title->setPointSize(15);
    header->addWidget(m_title);
    header->addStretch();
    auto *hint = new HudLabel("LMB ROTATE / RMB ZOOM / MMB PAN",
                              HudLabel::Role::Unit);
    header->addWidget(hint);
    header->addSpacing(18);
    header->addWidget(new HudLabel("Style"));
    header->addWidget(buildStyleSelector());
    root->addLayout(header);

    auto *middle = new QHBoxLayout;
    middle->setSpacing(10);
    root->addLayout(middle, 1);

    // Left dock: scene tree + model operations
    auto *leftDock = new CollapsibleDock(CollapsibleDock::Edge::Left, "Scene");
    leftDock->setExpandedSize(260);
    auto *scenePanel = new HudPanel("Scene Objects");
    auto *sceneLay = new QVBoxLayout(scenePanel);
    m_tree = new QTreeWidget;
    m_tree->setHeaderHidden(true);
    m_tree->setRootIsDecorated(false);
    m_tree->setFont(Theme::labelFont(10));
    connect(m_tree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
                m_current = current ? current->data(0, Qt::UserRole).toInt() : -1;
                syncPropertiesPanel();
            });
    sceneLay->addWidget(m_tree, 1);
    leftDock->setContent(scenePanel);
    leftDock->setContent(buildModelOpsPanel());
    middle->addWidget(leftDock);

    // Center: VTK viewport
    auto *viewPanel = new HudPanel("Viewport");
    auto *viewLay = new QVBoxLayout(viewPanel);
    m_vtkWidget = new QVTKOpenGLNativeWidget;

    // =========================================================================
    // Trackpad touch points must not be mapped to interactor button events,
    // or the camera rotates while merely moving the cursor (same fix as
    // ParaView; see ParaView issue #22901 / VTK issue #19073).
    // =========================================================================
    m_vtkWidget->setEnableTouchEventProcessing(false);

    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_vtkWidget->setRenderWindow(m_renderWindow);
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderWindow->AddRenderer(m_renderer);
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    m_renderWindow->GetInteractor()->SetInteractorStyle(style);
    viewLay->addWidget(m_vtkWidget);
    middle->addWidget(viewPanel, 1);

    // Right dock: properties
    auto *rightDock = new CollapsibleDock(CollapsibleDock::Edge::Right,
                                          "Properties");
    rightDock->setExpandedSize(280);
    rightDock->setContent(buildPropertiesPanel());
    middle->addWidget(rightDock);

    // The 3D scene and the cached accents follow the selected style.
    connect(Theme::Notifier::instance(), &Theme::Notifier::styleChanged,
            this, [this] { applyThemeColors(); });
    applyThemeColors();
}

// Lets the user switch style; the choice is remembered across runs.
QWidget *VtkExplorerWindow::buildStyleSelector()
{
    auto *combo = new HudComboBox;
    combo->setFixedWidth(150);
    for (Theme::Style style : Theme::styles())
        combo->addItem(Theme::styleName(style));
    combo->setCurrentText(Theme::styleName(Theme::style()));
    connect(combo, &QComboBox::currentIndexChanged, this, [](int index) {
        const QVector<Theme::Style> all = Theme::styles();
        if (index < 0 || index >= all.size())
            return;
        setApplicationStyle(all.at(index));
        QSettings().setValue("style", Theme::styleName(all.at(index)));
    });
    return combo;
}

// Re-apply every theme color kept outside of a paintEvent() (VTK scene, accents).
void VtkExplorerWindow::applyThemeColors()
{
    m_title->setAccent(Theme::Primary);
    m_clearButton->setAccent(Theme::Alert);

    const QColor bg = Theme::Background;
    m_renderer->SetBackground(bg.redF(), bg.greenF(), bg.blueF());
    applyAllProperties();   // edge color comes from the theme
    syncPropertiesPanel();
    render();
}

QWidget *VtkExplorerWindow::buildModelOpsPanel()
{
    auto *panel = new HudPanel("Model");
    auto *lay = new QVBoxLayout(panel);
    lay->setSpacing(8);

    auto *load = new HudButton("Load Objects…");
    connect(load, &QPushButton::clicked, this, [this] {
        const QStringList paths = QFileDialog::getOpenFileNames(
            this, "Load objects", QString(),
            "Meshes (*.vtk *.vtp *.ply *.obj *.ts);;"
            "VTK (*.vtk *.vtp);;Gocad TSurf (*.ts);;All files (*)");
        for (const QString &path : paths)
            loadFile(path);
    });
    lay->addWidget(load);

    auto *center = new HudButton("Center Model");
    connect(center, &QPushButton::clicked, this, [this] {
        m_renderer->ResetCamera();
        render();
    });
    lay->addWidget(center);

    m_clearButton = new HudButton("Clear Model");
    connect(m_clearButton, &QPushButton::clicked, this, [this] { clearModel(); });
    lay->addWidget(m_clearButton);

    return panel;
}

void VtkExplorerWindow::clearModel()
{
    for (const SceneObject &obj : m_objects)
        m_renderer->RemoveActor(obj.actor);
    m_objects.clear();
    m_tree->clear();
    m_current = -1;
    syncPropertiesPanel();
    render();
}

QWidget *VtkExplorerWindow::buildPropertiesPanel()
{
    auto *panel = new HudPanel("Properties");
    auto *lay = new QVBoxLayout(panel);
    lay->setSpacing(6);

    m_objName = new HudLabel("No Selection", HudLabel::Role::Value);
    lay->addWidget(m_objName);
    lay->addSpacing(6);

    m_visible = new HudCheckBox("Visible");
    connect(m_visible, &QCheckBox::toggled, this, [this](bool on) {
        withCurrent([on](SceneObject &o) { o.visible = on; });
    });
    lay->addWidget(m_visible);
    lay->addSpacing(6);

    lay->addWidget(new HudLabel("Color"));
    m_colorButton = new HudButton("Change…");
    connect(m_colorButton, &QPushButton::clicked, this, [this] {
        if (m_current < 0)
            return;
        const QColor c = QColorDialog::getColor(
            m_objects[m_current].color, this, "Object color");
        if (c.isValid())
            withCurrent([&c](SceneObject &o) { o.color = c; });
        syncPropertiesPanel();
    });
    lay->addWidget(m_colorButton);
    lay->addSpacing(6);

    lay->addWidget(new HudLabel("Representation"));
    m_representation = new HudComboBox;
    m_representation->addItems(
        {"Surface", "Surface + Edges", "Wireframe", "Points"});
    connect(m_representation, &QComboBox::currentIndexChanged, this,
            [this](int index) {
                withCurrent([index](SceneObject &o) { o.representation = index; });
            });
    lay->addWidget(m_representation);
    lay->addSpacing(6);

    lay->addWidget(new HudLabel("Attribute"));
    m_attribute = new HudComboBox;
    connect(m_attribute, &QComboBox::currentIndexChanged, this,
            [this](int index) {
                const QString name =
                    index <= 0 ? QString() : m_attribute->itemText(index);
                withCurrent([&name](SceneObject &o) { o.activeArray = name; });
            });
    lay->addWidget(m_attribute);
    lay->addSpacing(6);

    lay->addWidget(new HudLabel("Color Table"));
    m_colorMap = new HudComboBox;
    m_colorMap->addItems(colorMapNames());
    connect(m_colorMap, &QComboBox::currentIndexChanged, this,
            [this](int index) {
                withCurrent([index](SceneObject &o) { o.colorMap = index; });
            });
    lay->addWidget(m_colorMap);
    lay->addSpacing(6);

    lay->addWidget(new HudLabel("Opacity"));
    m_opacity = new HudSlider;
    m_opacity->setRange(0, 100);
    m_opacity->setValue(100);
    connect(m_opacity, &QSlider::valueChanged, this, [this](int value) {
        withCurrent([value](SceneObject &o) { o.opacity = value / 100.0; });
    });
    lay->addWidget(m_opacity);

    lay->addStretch();
    return panel;
}

void VtkExplorerWindow::buildInitialScene()
{
    addObject("Stanford Bunny", bunnyPolyData());
    m_objects.last().activeArray = "Z";           // colored on startup

    vtkNew<vtkSphereSource> sphere;
    sphere->SetRadius(0.06);
    sphere->SetCenter(0.22, 0.09, 0.0);
    sphere->SetThetaResolution(48);
    sphere->SetPhiResolution(48);
    sphere->Update();
    addObject("Sphere", sphere->GetOutput());

    vtkNew<vtkParametricTorus> torusFn;
    vtkNew<vtkParametricFunctionSource> torus;
    torus->SetParametricFunction(torusFn);
    torus->SetUResolution(64);
    torus->SetVResolution(32);
    torus->Update();
    vtkNew<vtkPolyData> torusPoly;
    torusPoly->DeepCopy(torus->GetOutput());
    addObject("Torus", torusPoly);
    m_objects.last().actor->SetScale(0.05);
    m_objects.last().actor->SetPosition(-0.22, 0.08, 0.0);
    m_objects.last().color = Theme::Accent;

    applyAllProperties();
    m_renderer->ResetCamera();
    m_tree->setCurrentItem(m_tree->topLevelItem(0));
    render();
}

void VtkExplorerWindow::addObject(const QString &name,
                                  vtkSmartPointer<vtkPolyData> poly)
{
    SceneObject obj;
    obj.name = name;
    obj.data = withNormals(poly);
    addCoordinateArrays(obj.data);
    obj.mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    obj.mapper->SetInputData(obj.data);
    obj.actor = vtkSmartPointer<vtkActor>::New();
    obj.actor->SetMapper(obj.mapper);
    m_renderer->AddActor(obj.actor);
    m_objects.append(obj);
    applyProperties(m_objects.last());

    auto *item = new QTreeWidgetItem(m_tree, {name.toUpper()});
    item->setData(0, Qt::UserRole, m_objects.size() - 1);
    m_tree->setCurrentItem(item);
}

void VtkExplorerWindow::applyProperties(SceneObject &obj)
{
    obj.actor->SetVisibility(obj.visible);
    vtkProperty *prop = obj.actor->GetProperty();
    prop->SetColor(obj.color.redF(), obj.color.greenF(), obj.color.blueF());
    prop->SetOpacity(obj.opacity);
    prop->SetPointSize(3.0);
    prop->SetLineWidth(1.0);
    const QColor edge = Theme::PrimaryDim;
    prop->SetEdgeColor(edge.redF(), edge.greenF(), edge.blueF());

    switch (obj.representation) {
    case 0: prop->SetRepresentationToSurface(); prop->EdgeVisibilityOff(); break;
    case 1: prop->SetRepresentationToSurface(); prop->EdgeVisibilityOn(); break;
    case 2: prop->SetRepresentationToWireframe(); prop->EdgeVisibilityOff(); break;
    case 3: prop->SetRepresentationToPoints(); prop->EdgeVisibilityOff(); break;
    }

    vtkDataArray *array = obj.activeArray.isEmpty()
        ? nullptr
        : obj.data->GetPointData()->GetArray(
              obj.activeArray.toUtf8().constData());
    if (array) {
        double range[2];
        array->GetRange(range);
        obj.mapper->SetScalarModeToUsePointFieldData();
        obj.mapper->SelectColorArray(obj.activeArray.toUtf8().constData());
        obj.mapper->SetScalarRange(range);
        obj.mapper->SetLookupTable(makeLookupTable(obj.colorMap));
        obj.mapper->ScalarVisibilityOn();
    } else {
        obj.mapper->ScalarVisibilityOff();
    }
}

void VtkExplorerWindow::applyAllProperties()
{
    for (SceneObject &obj : m_objects)
        applyProperties(obj);
}

// Push the selected object's state into the panel widgets.
void VtkExplorerWindow::syncPropertiesPanel()
{
    m_syncing = true;
    const bool valid = m_current >= 0 && m_current < m_objects.size();
    for (QWidget *w : std::initializer_list<QWidget *>{
             m_visible, m_colorButton, m_representation, m_attribute,
             m_colorMap, m_opacity})
        w->setEnabled(valid);

    if (!valid) {
        m_objName->setText("No Selection");
        m_syncing = false;
        return;
    }
    const SceneObject &obj = m_objects[m_current];
    m_objName->setText(obj.name.toUpper());
    m_objName->setAccent(Theme::Primary);
    m_visible->setChecked(obj.visible);
    m_colorButton->setAccent(obj.color);
    m_colorButton->setText(obj.color.name().toUpper());
    m_representation->setCurrentIndex(obj.representation);

    m_attribute->clear();
    m_attribute->addItem("Solid Color");
    int activeIndex = 0;
    vtkPointData *pd = obj.data->GetPointData();
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
        vtkDataArray *array = pd->GetArray(i);
        if (!array || !array->GetName() || array->GetNumberOfComponents() != 1)
            continue;
        m_attribute->addItem(QString::fromUtf8(array->GetName()));
        if (obj.activeArray == QString::fromUtf8(array->GetName()))
            activeIndex = m_attribute->count() - 1;
    }
    m_attribute->setCurrentIndex(activeIndex);

    m_colorMap->setCurrentIndex(obj.colorMap);
    m_opacity->setValue(int(obj.opacity * 100));
    m_syncing = false;
}

void VtkExplorerWindow::render()
{
    m_renderWindow->Render();
}
