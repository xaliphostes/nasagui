#pragma once

#include <QBrush>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>
#include <QVector>

class QOpenGLShaderProgram;
class QOpenGLTexture;

namespace nasagui {

// 3D mesh viewport in the HUD style: dark background, rim-lit fill, cyan
// wireframe, floor grid and an orbit camera (left-drag orbits, wheel zooms).
class ModelView : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit ModelView(QWidget *parent = nullptr);
    ~ModelView() override;

    // Installs the OpenGL 3.3 core surface format this widget needs.
    // MUST be called before the QApplication is constructed — a per-widget
    // format cannot be shared with the window's default context on macOS.
    static void setDefaultSurfaceFormat();

    // Triangle mesh: xyz positions and 3 indices per face. Normals are
    // computed internally; the camera auto-fits the bounding sphere.
    void setMesh(const float *positions, std::size_t vertexCount,
                 const unsigned int *indices, std::size_t indexCount);

    void setAutoRotate(bool on);

    // ---- Scalar field ("plot an attribute with a color table") ----------
    enum class ColorMap { Viridis, CoolWarm, Ice, Thermal };

    // One value per vertex; the mesh is colored through the current color
    // table and a colorbar legend appears. The range is min/max of the data
    // unless setScalarRange() was called.
    void setScalars(const QVector<float> &values, const QString &name = {});
    void clearScalars();                       // back to plain shaded fill
    void setScalarRange(float min, float max); // fixed mapping range
    void resetScalarRange();                   // auto range from the data
    void setColorMap(ColorMap map);
    void setColorMap(const QGradientStops &stops);   // custom table

    QSize sizeHint() const override { return {320, 240}; }
    QSize minimumSizeHint() const override { return {160, 120}; }

protected:
    void initializeGL() override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void uploadMesh();

    // CPU-side mesh (interleaved position + normal)
    QVector<float> m_interleaved;
    QVector<unsigned int> m_indices;
    bool m_meshDirty = false;

    // Scalar field
    QVector<float> m_scalars;
    QString m_scalarName;
    float m_scalarMin = 0.0f;
    float m_scalarMax = 1.0f;
    bool m_autoScalarRange = true;
    QGradientStops m_lutStops;
    QOpenGLTexture *m_lut = nullptr;
    bool m_lutDirty = true;
    QOpenGLBuffer m_scalarVbo{QOpenGLBuffer::VertexBuffer};
    QVector3D m_center;
    float m_radius = 1.0f;
    float m_floorY = 0.0f;

    // GL resources
    QOpenGLShaderProgram *m_litProgram = nullptr;
    QOpenGLShaderProgram *m_flatProgram = nullptr;
    QOpenGLVertexArrayObject m_meshVao;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_ibo{QOpenGLBuffer::IndexBuffer};
    QOpenGLVertexArrayObject m_gridVao;
    QOpenGLBuffer m_gridVbo{QOpenGLBuffer::VertexBuffer};
    int m_gridVertexCount = 0;

    // Orbit camera
    double m_azimuth = 35.0;     // degrees
    double m_elevation = 18.0;   // degrees
    double m_distance = 0.0;     // world units; 0 = not fitted yet
    QPoint m_lastPos;
    bool m_dragging = false;
    QTimer m_spinTimer;
    bool m_autoRotate = true;
};

} // namespace nasagui
