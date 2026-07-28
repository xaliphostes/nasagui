#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>
#include <QVector>

class QPainter;
class QPainterPath;

namespace nasagui::Theme {

// ---- Styles ----------------------------------------------------------------
// The palette below is mutable: picking a style rewrites it in place, so every
// widget that reads Theme::Xxx at paint time follows along automatically.
enum class Style {
    MissionControl,   // deep-space console (default)
    Daylight,         // same design language, lighter: pale deck, ink text
};

Style style();
void setStyle(Style style);

QVector<Style> styles();                  // all styles, in menu order
QString styleName(Style style);           // human readable, e.g. "Mission Control"
Style styleFromName(const QString &name,  // inverse of styleName()
                    Style fallback = Style::MissionControl);

// Emits styleChanged() after every setStyle(). Widgets that cache a theme
// colour (in a QPalette, a stylesheet, a renderer…) should reapply it here.
class Notifier : public QObject
{
    Q_OBJECT
public:
    static Notifier *instance();

signals:
    void styleChanged();
};

// ---- Palette ---------------------------------------------------------------
extern QColor Background;    // window / viewport backdrop
extern QColor PanelFill;     // translucent panel body
extern QColor FieldFill;     // input, check box and combo box background
extern QColor PanelBorder;
extern QColor GridLine;
extern QColor Primary;       // accent used by gauges, sliders, highlights
extern QColor PrimaryDim;
extern QColor Accent;        // secondary accent (caution)
extern QColor Alert;
extern QColor Ok;
extern QColor TextPrimary;
extern QColor TextDim;

// Halo intensity of drawGlowPath(); light styles need a fainter glow.
extern qreal GlowStrength;

// ---- Fonts -----------------------------------------------------------------
QFont titleFont(int pointSize = 11);   // condensed, letter-spaced headers
QFont labelFont(int pointSize = 10);   // small captions
QFont valueFont(int pointSize = 22);   // monospaced numeric readouts

// Stroke a path with a soft neon glow (layered translucent strokes).
void drawGlowPath(QPainter &p, const QPainterPath &path,
                  const QColor &color, qreal width = 1.5);

} // namespace nasagui::Theme
