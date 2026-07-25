#pragma once

#include <QColor>
#include <QFont>

class QPainter;
class QPainterPath;

namespace nasagui::Theme {

// ---- Palette ---------------------------------------------------------------
const QColor Background {0x06, 0x0b, 0x12};        // deep space blue-black
const QColor PanelFill  {0x0a, 0x14, 0x1f, 215};   // translucent panel body
const QColor PanelBorder{0x1d, 0x3a, 0x4d};
const QColor GridLine   {0x14, 0x2a, 0x3a};
const QColor Primary    {0x35, 0xd6, 0xed};        // NASA cyan
const QColor PrimaryDim {0x1a, 0x6d, 0x85};
const QColor Accent     {0xff, 0x9f, 0x1c};        // Martian amber
const QColor Alert      {0xff, 0x4d, 0x4d};
const QColor Ok         {0x3d, 0xe0, 0x8a};
const QColor TextPrimary{0xd7, 0xe7, 0xef};
const QColor TextDim    {0x6f, 0x8a, 0x99};

// ---- Fonts -----------------------------------------------------------------
QFont titleFont(int pointSize = 11);   // condensed, letter-spaced headers
QFont labelFont(int pointSize = 10);   // small captions
QFont valueFont(int pointSize = 22);   // monospaced numeric readouts

// Stroke a path with a soft neon glow (layered translucent strokes).
void drawGlowPath(QPainter &p, const QPainterPath &path,
                  const QColor &color, qreal width = 1.5);

} // namespace nasagui::Theme
