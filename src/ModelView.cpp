#include "nasagui/ModelView.h"
#include "nasagui/Theme.h"

#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QPainter>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>

namespace nasagui {

namespace {

const char *kLitVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in float aScalar;
uniform mat4 uMvp;
uniform mat4 uMv;
uniform mat3 uNormalMatrix;
out vec3 vNormal;
out vec3 vViewPos;
out float vScalar;
void main() {
    vNormal = normalize(uNormalMatrix * aNormal);
    vViewPos = (uMv * vec4(aPos, 1.0)).xyz;
    vScalar = aScalar;
    gl_Position = uMvp * vec4(aPos, 1.0);
})";

const char *kLitFragmentShader = R"(#version 330 core
in vec3 vNormal;
in vec3 vViewPos;
in float vScalar;
uniform vec3 uBase;
uniform vec3 uGlow;
uniform bool uUseScalars;
uniform float uScalarMin;
uniform float uScalarMax;
uniform sampler2D uLut;
out vec4 fragColor;
void main() {
    vec3 n = normalize(vNormal);
    vec3 v = normalize(-vViewPos);
    float diff = max(dot(n, normalize(vec3(0.4, 0.8, 0.6))), 0.0);
    float rim = pow(1.0 - max(dot(n, v), 0.0), 2.2);
    vec3 base = uBase;
    float rimGain = 0.9;
    if (uUseScalars) {
        float t = clamp((vScalar - uScalarMin) / max(uScalarMax - uScalarMin, 1e-9),
                        0.0, 1.0);
        base = texture(uLut, vec2(t, 0.5)).rgb;
        rimGain = 0.25;                  // keep colors readable
    }
    vec3 c = base * (0.45 + 0.55 * diff) + uGlow * rim * rimGain;
    fragColor = vec4(c, 1.0);
})";

const char *kFlatVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMvp;
void main() { gl_Position = uMvp * vec4(aPos, 1.0); })";

const char *kFlatFragmentShader = R"(#version 330 core
uniform vec4 uColor;
out vec4 fragColor;
void main() { fragColor = uColor; })";

QGradientStops presetStops(ModelView::ColorMap map)
{
    using M = ModelView::ColorMap;
    switch (map) {
    case M::Viridis:
        return {{0.00, QColor(68, 1, 84)},    {0.15, QColor(72, 36, 117)},
                {0.30, QColor(65, 68, 135)},  {0.45, QColor(52, 96, 141)},
                {0.60, QColor(41, 120, 142)}, {0.75, QColor(34, 144, 141)},
                {0.85, QColor(68, 176, 122)}, {0.95, QColor(160, 218, 57)},
                {1.00, QColor(253, 231, 37)}};
    case M::CoolWarm:
        return {{0.0, QColor(59, 76, 192)},   {0.5, QColor(221, 221, 221)},
                {1.0, QColor(180, 4, 38)}};
    case M::Ice:
        return {{0.0, QColor(6, 11, 18)},     {0.4, QColor(26, 109, 133)},
                {0.75, QColor(53, 214, 237)}, {1.0, QColor(230, 250, 255)}};
    case M::Thermal:
        return {{0.00, QColor(4, 0, 10)},     {0.30, QColor(87, 16, 110)},
                {0.60, QColor(188, 55, 84)},  {0.80, QColor(243, 133, 25)},
                {1.00, QColor(252, 230, 140)}};
    }
    return {};
}

QImage lutImage(const QGradientStops &stops)
{
    QImage img(256, 1, QImage::Format_RGBA8888);
    QPainter p(&img);
    QLinearGradient g(0, 0, img.width(), 0);
    g.setStops(stops);
    p.fillRect(img.rect(), g);
    return img;
}

} // namespace

void ModelView::setDefaultSurfaceFormat()
{
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);   // the QPainter overlay clips with stencil
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);
}

ModelView::ModelView(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setCursor(Qt::OpenHandCursor);
    m_lutStops = presetStops(ColorMap::Viridis);

    m_spinTimer.setInterval(33);
    connect(&m_spinTimer, &QTimer::timeout, this, [this] {
        if (!m_dragging) {
            m_azimuth = std::fmod(m_azimuth + 0.12, 360.0);
            update();
        }
    });
    m_spinTimer.start();
}

ModelView::~ModelView()
{
    makeCurrent();
    delete m_lut;
    m_vbo.destroy();
    m_ibo.destroy();
    m_scalarVbo.destroy();
    m_gridVbo.destroy();
    m_meshVao.destroy();
    m_gridVao.destroy();
    delete m_litProgram;
    delete m_flatProgram;
    doneCurrent();
}

void ModelView::setAutoRotate(bool on)
{
    m_autoRotate = on;
    if (on)
        m_spinTimer.start();
    else
        m_spinTimer.stop();
}

void ModelView::setScalars(const QVector<float> &values, const QString &name)
{
    m_scalars = values;
    m_scalarName = name;
    if (m_autoScalarRange && !m_scalars.isEmpty()) {
        const auto [lo, hi] = std::minmax_element(m_scalars.cbegin(),
                                                  m_scalars.cend());
        m_scalarMin = *lo;
        m_scalarMax = *hi;
    }
    m_meshDirty = true;
    update();
}

void ModelView::clearScalars()
{
    m_scalars.clear();
    m_meshDirty = true;
    update();
}

void ModelView::setScalarRange(float min, float max)
{
    m_scalarMin = min;
    m_scalarMax = qMax(max, min + 1e-9f);
    m_autoScalarRange = false;
    update();
}

void ModelView::resetScalarRange()
{
    m_autoScalarRange = true;
    if (!m_scalars.isEmpty()) {
        const auto [lo, hi] = std::minmax_element(m_scalars.cbegin(),
                                                  m_scalars.cend());
        m_scalarMin = *lo;
        m_scalarMax = *hi;
    }
    update();
}

void ModelView::setColorMap(ColorMap map)
{
    setColorMap(presetStops(map));
}

void ModelView::setColorMap(const QGradientStops &stops)
{
    m_lutStops = stops;
    m_lutDirty = true;
    update();
}

void ModelView::setMesh(const float *positions, std::size_t vertexCount,
                        const unsigned int *indices, std::size_t indexCount)
{
    // Averaged per-vertex normals from face normals
    QVector<QVector3D> normals;
    normals.resize(int(vertexCount));
    for (std::size_t f = 0; f + 2 < indexCount; f += 3) {
        const unsigned int i0 = indices[f], i1 = indices[f + 1], i2 = indices[f + 2];
        const QVector3D p0(positions[3 * i0], positions[3 * i0 + 1], positions[3 * i0 + 2]);
        const QVector3D p1(positions[3 * i1], positions[3 * i1 + 1], positions[3 * i1 + 2]);
        const QVector3D p2(positions[3 * i2], positions[3 * i2 + 1], positions[3 * i2 + 2]);
        const QVector3D fn = QVector3D::crossProduct(p1 - p0, p2 - p0);
        normals[int(i0)] += fn;
        normals[int(i1)] += fn;
        normals[int(i2)] += fn;
    }

    m_interleaved.resize(int(vertexCount) * 6);
    QVector3D lo(1e9f, 1e9f, 1e9f), hi(-1e9f, -1e9f, -1e9f);
    for (std::size_t v = 0; v < vertexCount; ++v) {
        const QVector3D p(positions[3 * v], positions[3 * v + 1], positions[3 * v + 2]);
        const QVector3D n = normals[int(v)].normalized();
        float *dst = m_interleaved.data() + v * 6;
        dst[0] = p.x(); dst[1] = p.y(); dst[2] = p.z();
        dst[3] = n.x(); dst[4] = n.y(); dst[5] = n.z();
        lo.setX(qMin(lo.x(), p.x())); lo.setY(qMin(lo.y(), p.y())); lo.setZ(qMin(lo.z(), p.z()));
        hi.setX(qMax(hi.x(), p.x())); hi.setY(qMax(hi.y(), p.y())); hi.setZ(qMax(hi.z(), p.z()));
    }

    m_indices.resize(int(indexCount));
    std::copy(indices, indices + indexCount, m_indices.begin());

    m_center = (lo + hi) * 0.5f;
    m_radius = qMax(0.001f, (hi - lo).length() * 0.5f);
    m_floorY = lo.y();
    if (m_distance <= 0.0)
        m_distance = m_radius * 2.8;

    m_meshDirty = true;
    update();
}

void ModelView::initializeGL()
{
    initializeOpenGLFunctions();

    m_litProgram = new QOpenGLShaderProgram;
    m_litProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, kLitVertexShader);
    m_litProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, kLitFragmentShader);
    if (!m_litProgram->link())
        qWarning("nasagui::ModelView: lit shader failed: %s",
                 qPrintable(m_litProgram->log()));

    m_flatProgram = new QOpenGLShaderProgram;
    m_flatProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, kFlatVertexShader);
    m_flatProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, kFlatFragmentShader);
    if (!m_flatProgram->link())
        qWarning("nasagui::ModelView: flat shader failed: %s",
                 qPrintable(m_flatProgram->log()));
}

void ModelView::uploadMesh()
{
    if (!m_meshVao.isCreated())
        m_meshVao.create();
    m_meshVao.bind();

    if (!m_vbo.isCreated())
        m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(m_interleaved.constData(),
                   int(m_interleaved.size() * sizeof(float)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));

    // Optional per-vertex scalar attribute
    const bool hasScalars =
        m_scalars.size() * 6 == m_interleaved.size() && !m_scalars.isEmpty();
    if (hasScalars) {
        if (!m_scalarVbo.isCreated())
            m_scalarVbo.create();
        m_scalarVbo.bind();
        m_scalarVbo.allocate(m_scalars.constData(),
                             int(m_scalars.size() * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);
    } else {
        glDisableVertexAttribArray(2);
    }

    if (!m_ibo.isCreated())
        m_ibo.create();
    m_ibo.bind();
    m_ibo.allocate(m_indices.constData(),
                   int(m_indices.size() * sizeof(unsigned int)));
    m_meshVao.release();

    // Floor grid centred under the model
    QVector<float> grid;
    const int half = 5;
    const float ext = m_radius * 1.6f;
    const float step = ext / half;
    for (int i = -half; i <= half; ++i) {
        const float o = i * step;
        grid << m_center.x() - ext << m_floorY << m_center.z() + o
             << m_center.x() + ext << m_floorY << m_center.z() + o;
        grid << m_center.x() + o << m_floorY << m_center.z() - ext
             << m_center.x() + o << m_floorY << m_center.z() + ext;
    }
    m_gridVertexCount = grid.size() / 3;

    if (!m_gridVao.isCreated())
        m_gridVao.create();
    m_gridVao.bind();
    if (!m_gridVbo.isCreated())
        m_gridVbo.create();
    m_gridVbo.bind();
    m_gridVbo.allocate(grid.constData(), int(grid.size() * sizeof(float)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    m_gridVao.release();

    m_meshDirty = false;
}

void ModelView::paintGL()
{
    if (m_meshDirty)
        uploadMesh();

    const QColor bg = Theme::Background;
    glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Never draw with broken programs — bail after the clear so the
    // framebuffer at least holds the background, not stale VRAM.
    if (m_indices.isEmpty() || !m_litProgram || !m_litProgram->isLinked()
        || !m_flatProgram->isLinked())
        return;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const double az = qDegreesToRadians(m_azimuth);
    const double el = qDegreesToRadians(m_elevation);
    const QVector3D eye = m_center
        + QVector3D(float(qCos(el) * qCos(az)), float(qSin(el)),
                    float(qCos(el) * qSin(az))) * float(m_distance);

    QMatrix4x4 proj;
    proj.perspective(45.0f, float(width()) / qMax(1, height()),
                     m_radius * 0.05f, m_radius * 50.0f);
    QMatrix4x4 view;
    view.lookAt(eye, m_center, QVector3D(0, 1, 0));
    const QMatrix4x4 mvp = proj * view;

    // Floor grid
    m_flatProgram->bind();
    m_flatProgram->setUniformValue("uMvp", mvp);
    QColor gridColor = Theme::GridLine;
    m_flatProgram->setUniformValue(
        "uColor", QVector4D(gridColor.redF(), gridColor.greenF(),
                            gridColor.blueF(), 0.9f));
    m_gridVao.bind();
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    m_gridVao.release();

    // Color table texture
    const bool useScalars =
        m_scalars.size() * 6 == m_interleaved.size() && !m_scalars.isEmpty();
    if (m_lutDirty || !m_lut) {
        delete m_lut;
        m_lut = new QOpenGLTexture(lutImage(m_lutStops));
        m_lut->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        m_lut->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_lutDirty = false;
    }

    // Solid fill, offset back so the wireframe stays visible on top
    m_litProgram->bind();
    m_litProgram->setUniformValue("uMvp", mvp);
    m_litProgram->setUniformValue("uMv", view);
    m_litProgram->setUniformValue("uNormalMatrix", view.normalMatrix());
    m_litProgram->setUniformValue("uBase", QVector3D(0.055f, 0.12f, 0.18f));
    const QColor glow = Theme::Primary;
    m_litProgram->setUniformValue(
        "uGlow", QVector3D(glow.redF(), glow.greenF(), glow.blueF()));
    m_litProgram->setUniformValue("uUseScalars", useScalars);
    m_litProgram->setUniformValue("uScalarMin", m_scalarMin);
    m_litProgram->setUniformValue("uScalarMax", m_scalarMax);
    m_lut->bind(0);
    m_litProgram->setUniformValue("uLut", 0);
    m_meshVao.bind();
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, nullptr);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Wireframe pass
    m_flatProgram->bind();
    m_flatProgram->setUniformValue("uMvp", mvp);
    m_flatProgram->setUniformValue(
        "uColor", QVector4D(glow.redF(), glow.greenF(), glow.blueF(), 0.28f));
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, nullptr);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_meshVao.release();
    m_flatProgram->release();

    // HUD overlay
    glDisable(GL_DEPTH_TEST);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(Theme::valueFont(8));
    p.setPen(Theme::TextDim);
    p.drawText(QRectF(8, height() - 22, width() - 16, 16),
               Qt::AlignLeft | Qt::AlignVCenter,
               QString::asprintf("AZ %05.1f  EL %+05.1f  RNG %.2f",
                                 std::fmod(m_azimuth + 360.0, 360.0),
                                 m_elevation, m_distance));
    p.drawText(QRectF(8, height() - 22, width() - 16, 16),
               Qt::AlignRight | Qt::AlignVCenter, "DRAG ORBIT / WHEEL ZOOM");

    // Colorbar legend
    if (useScalars) {
        const QRectF bar(width() - 26.0, height() * 0.24, 10.0, height() * 0.5);
        QLinearGradient g(bar.bottomLeft(), bar.topLeft());
        g.setStops(m_lutStops);
        p.fillRect(bar, g);
        p.setPen(QPen(Theme::PanelBorder, 1.0));
        p.drawRect(bar);

        p.setPen(Theme::TextDim);
        p.setFont(Theme::valueFont(7));
        const QRectF maxRect(bar.left() - 60, bar.top() - 16, bar.width() + 60, 14);
        const QRectF minRect(bar.left() - 60, bar.bottom() + 2, bar.width() + 60, 14);
        p.drawText(maxRect, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(m_scalarMax, 'g', 3));
        p.drawText(minRect, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(m_scalarMin, 'g', 3));
        if (!m_scalarName.isEmpty()) {
            p.setPen(Theme::TextPrimary);
            p.setFont(Theme::titleFont(7));
            p.drawText(QRectF(bar.left() - 60, maxRect.top() - 16,
                              bar.width() + 60, 14),
                       Qt::AlignRight | Qt::AlignVCenter, m_scalarName.toUpper());
        }
    }
}

void ModelView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void ModelView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging)
        return;
    const QPoint delta = event->pos() - m_lastPos;
    m_lastPos = event->pos();
    m_azimuth = std::fmod(m_azimuth + delta.x() * 0.4, 360.0);
    m_elevation = qBound(-85.0, m_elevation - delta.y() * 0.4, 85.0);
    update();
}

void ModelView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
    }
}

void ModelView::wheelEvent(QWheelEvent *event)
{
    const double factor = std::pow(1.0015, -event->angleDelta().y());
    m_distance = qBound(double(m_radius) * 1.2, m_distance * factor,
                        double(m_radius) * 10.0);
    update();
}

} // namespace nasagui
