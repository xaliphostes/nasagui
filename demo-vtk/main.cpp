// nasagui VTK demo: 3D viewport + scene tree + per-object properties panel.
// Loads VTK meshes (*.vtk *.vtp *.ply *.obj) and Gocad TSurf (*.ts) files.

#include <nasagui/CollapsibleDock.h>
#include <nasagui/Controls.h>
#include <nasagui/HudButton.h>
#include <nasagui/HudPanel.h>
#include <nasagui/Style.h>
#include <nasagui/Theme.h>

#include "../demo/BunnyMesh.h"
#include "TSurfReader.h"

#include <QApplication>
#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkDataArray.h>
#include <vtkFloatArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <vtkParametricFunctionSource.h>
#include <vtkParametricTorus.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataReader.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkXMLPolyDataReader.h>

using namespace nasagui;

// ---- Color tables (same presets as nasagui::ModelView) ---------------------

static const char *kColorMapNames[] = {"Viridis", "Cool-Warm", "Ice", "Thermal"};

static QGradientStops colorMapStops(int index)
{
    switch (index) {
    default:
    case 0:
        return {{0.00, QColor(68, 1, 84)},    {0.15, QColor(72, 36, 117)},
                {0.30, QColor(65, 68, 135)},  {0.45, QColor(52, 96, 141)},
                {0.60, QColor(41, 120, 142)}, {0.75, QColor(34, 144, 141)},
                {0.85, QColor(68, 176, 122)}, {0.95, QColor(160, 218, 57)},
                {1.00, QColor(253, 231, 37)}};
    case 1:
        return {{0.0, QColor(59, 76, 192)},   {0.5, QColor(221, 221, 221)},
                {1.0, QColor(180, 4, 38)}};
    case 2:
        return {{0.0, QColor(6, 11, 18)},     {0.4, QColor(26, 109, 133)},
                {0.75, QColor(53, 214, 237)}, {1.0, QColor(230, 250, 255)}};
    case 3:
        return {{0.00, QColor(4, 0, 10)},     {0.30, QColor(87, 16, 110)},
                {0.60, QColor(188, 55, 84)},  {0.80, QColor(243, 133, 25)},
                {1.00, QColor(252, 230, 140)}};
    }
}

static vtkSmartPointer<vtkLookupTable> makeLookupTable(int colorMapIndex)
{
    // Rasterize the gradient once, then copy it into the VTK table.
    QImage img(256, 1, QImage::Format_RGBA8888);
    {
        QPainter p(&img);
        QLinearGradient g(0, 0, img.width(), 0);
        g.setStops(colorMapStops(colorMapIndex));
        p.fillRect(img.rect(), g);
    }
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(256);
    for (int i = 0; i < 256; ++i) {
        const QColor c = img.pixelColor(i, 0);
        lut->SetTableValue(i, c.redF(), c.greenF(), c.blueF(), 1.0);
    }
    lut->Build();
    return lut;
}

// ---- Mesh helpers ----------------------------------------------------------

static vtkSmartPointer<vtkPolyData> withNormals(vtkPolyData *poly)
{
    vtkNew<vtkPolyDataNormals> normals;
    normals->SetInputData(poly);
    normals->SplittingOff();
    normals->ConsistencyOn();
    normals->Update();
    return normals->GetOutput();
}

static void addCoordinateArrays(vtkPolyData *poly)
{
    const char *names[] = {"X", "Y", "Z"};
    const vtkIdType n = poly->GetNumberOfPoints();
    for (int comp = 0; comp < 3; ++comp) {
        if (poly->GetPointData()->GetArray(names[comp]))
            continue;
        auto array = vtkSmartPointer<vtkFloatArray>::New();
        array->SetName(names[comp]);
        array->SetNumberOfTuples(n);
        for (vtkIdType i = 0; i < n; ++i)
            array->SetValue(i, float(poly->GetPoint(i)[comp]));
        poly->GetPointData()->AddArray(array);
    }
}

static vtkSmartPointer<vtkPolyData> bunnyPolyData()
{
    vtkNew<vtkPoints> points;
    for (std::size_t v = 0; v < bunny::vertexCount; ++v)
        points->InsertNextPoint(bunny::positions[3 * v],
                                bunny::positions[3 * v + 1],
                                bunny::positions[3 * v + 2]);
    vtkNew<vtkCellArray> tris;
    for (std::size_t f = 0; f + 2 < bunny::indexCount; f += 3) {
        tris->InsertNextCell(3);
        tris->InsertCellPoint(bunny::indices[f]);
        tris->InsertCellPoint(bunny::indices[f + 1]);
        tris->InsertCellPoint(bunny::indices[f + 2]);
    }
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    poly->SetPoints(points);
    poly->SetPolys(tris);
    return poly;
}

// ---- Scene object ----------------------------------------------------------

struct SceneObject {
    QString name;
    vtkSmartPointer<vtkPolyData> data;
    vtkSmartPointer<vtkPolyDataMapper> mapper;
    vtkSmartPointer<vtkActor> actor;
    QColor color = QColor(0x35, 0xd6, 0xed);
    bool visible = true;
    int representation = 0;   // 0 surface, 1 surface+edges, 2 wireframe, 3 points
    QString activeArray;      // empty = solid color
    int colorMap = 0;
    double opacity = 1.0;
};

// ---- Main window -----------------------------------------------------------

class VtkExplorerWindow : public QWidget
{
public:
    VtkExplorerWindow()
    {
        setWindowTitle("NASA-GUI // VTK Model Explorer");
        resize(1500, 880);
        buildUi();
        buildInitialScene();
    }

    void loadFile(const QString &path)
    {
        const QString suffix = QFileInfo(path).suffix().toLower();
        vtkSmartPointer<vtkPolyData> poly;
        QString error;

        if (suffix == "ts") {
            poly = tsurf::read(path, &error);
        } else if (suffix == "vtk") {
            vtkNew<vtkPolyDataReader> r;
            r->SetFileName(path.toUtf8().constData());
            r->Update();
            poly = r->GetOutput();
        } else if (suffix == "vtp") {
            vtkNew<vtkXMLPolyDataReader> r;
            r->SetFileName(path.toUtf8().constData());
            r->Update();
            poly = r->GetOutput();
        } else if (suffix == "ply") {
            vtkNew<vtkPLYReader> r;
            r->SetFileName(path.toUtf8().constData());
            r->Update();
            poly = r->GetOutput();
        } else if (suffix == "obj") {
            vtkNew<vtkOBJReader> r;
            r->SetFileName(path.toUtf8().constData());
            r->Update();
            poly = r->GetOutput();
        } else {
            error = QStringLiteral("unsupported format: .%1").arg(suffix);
        }

        if (!poly || poly->GetNumberOfPoints() == 0) {
            QMessageBox::warning(this, "Load failed",
                                 error.isEmpty()
                                     ? QStringLiteral("%1: empty mesh").arg(path)
                                     : error);
            return;
        }
        addObject(QFileInfo(path).completeBaseName(), poly);
        m_renderer->ResetCamera();
        render();
    }

private:
    void buildUi()
    {
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(18, 14, 18, 18);
        root->setSpacing(10);

        // Header
        auto *header = new QHBoxLayout;
        auto *title = new HudLabel("Model Explorer // VTK", HudLabel::Role::Title);
        title->setFont(Theme::titleFont(15));
        title->setAccent(Theme::Primary);
        header->addWidget(title);
        header->addStretch();
        auto *hint = new HudLabel("LMB ROTATE / RMB ZOOM / MMB PAN",
                                  HudLabel::Role::Unit);
        header->addWidget(hint);
        root->addLayout(header);

        auto *middle = new QHBoxLayout;
        middle->setSpacing(10);
        root->addLayout(middle, 1);

        // Left dock: scene tree + load button
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
        // Trackpad touch points must not be mapped to interactor button events,
        // or the camera rotates while merely moving the cursor (same fix as
        // ParaView; see ParaView issue #22901 / VTK issue #19073).
        m_vtkWidget->setEnableTouchEventProcessing(false);
        m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
        m_vtkWidget->setRenderWindow(m_renderWindow);
        m_renderer = vtkSmartPointer<vtkRenderer>::New();
        const QColor bg = Theme::Background;
        m_renderer->SetBackground(bg.redF(), bg.greenF(), bg.blueF());
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
    }

    QWidget *buildModelOpsPanel()
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

        auto *clear = new HudButton("Clear Model");
        clear->setAccent(Theme::Alert);
        connect(clear, &QPushButton::clicked, this, [this] { clearModel(); });
        lay->addWidget(clear);

        return panel;
    }

    void clearModel()
    {
        for (const SceneObject &obj : m_objects)
            m_renderer->RemoveActor(obj.actor);
        m_objects.clear();
        m_tree->clear();
        m_current = -1;
        syncPropertiesPanel();
        render();
    }

    QWidget *buildPropertiesPanel()
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
        for (const char *name : kColorMapNames)
            m_colorMap->addItem(name);
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

    void buildInitialScene()
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

    void addObject(const QString &name, vtkSmartPointer<vtkPolyData> poly)
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

        auto *item = new QTreeWidgetItem(m_tree,
                                         {name.toUpper()});
        item->setData(0, Qt::UserRole, m_objects.size() - 1);
        m_tree->setCurrentItem(item);
    }

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

    void applyProperties(SceneObject &obj)
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

    void applyAllProperties()
    {
        for (SceneObject &obj : m_objects)
            applyProperties(obj);
    }

    // Push the selected object's state into the panel widgets.
    void syncPropertiesPanel()
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
            if (!array || !array->GetName()
                || array->GetNumberOfComponents() != 1)
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

    void render() { m_renderWindow->Render(); }

    QTreeWidget *m_tree = nullptr;
    QVTKOpenGLNativeWidget *m_vtkWidget = nullptr;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    QList<SceneObject> m_objects;
    int m_current = -1;
    bool m_syncing = false;

    HudLabel *m_objName = nullptr;
    HudCheckBox *m_visible = nullptr;
    HudButton *m_colorButton = nullptr;
    HudComboBox *m_representation = nullptr;
    HudComboBox *m_attribute = nullptr;
    HudComboBox *m_colorMap = nullptr;
    HudSlider *m_opacity = nullptr;
};

int main(int argc, char *argv[])
{
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
    QApplication app(argc, argv);
    applyTheme(app);

    VtkExplorerWindow window;
    window.show();

    // Extra CLI arguments are loaded as models (also: --snapshot <file.png>)
    const QStringList args = app.arguments();
    const int snapIdx = args.indexOf("--snapshot");
    for (int i = 1; i < args.size(); ++i) {
        if (i == snapIdx || i == snapIdx + 1 || args[i].startsWith("--"))
            continue;
        window.loadFile(args[i]);
    }
    if (snapIdx >= 0 && snapIdx + 1 < args.size()) {
        const QString path = args.at(snapIdx + 1);
        QTimer::singleShot(2500, &app, [&window, path] {
            window.grab().save(path);
            QApplication::quit();
        });
    }
    return app.exec();
}
