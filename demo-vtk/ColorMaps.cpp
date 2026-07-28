#include "ColorMaps.h"

#include <QImage>
#include <QLinearGradient>
#include <QPainter>

QStringList colorMapNames()
{
    return {"Viridis", "Cool-Warm", "Ice", "Thermal"};
}

QGradientStops colorMapStops(int index)
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

vtkSmartPointer<vtkLookupTable> makeLookupTable(int colorMapIndex)
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
