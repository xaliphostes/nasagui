#include "nasagui/Theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStringList>

namespace nasagui::Theme {

namespace {

struct PaletteDef
{
    QColor background, panelFill, fieldFill, panelBorder, gridLine;
    QColor primary, primaryDim, accent, alert, ok;
    QColor textPrimary, textDim;
    qreal glow;
};

// Original mission-control look: NASA cyan on deep space blue-black.
const PaletteDef kMissionControl {
    {0x06, 0x0b, 0x12},        // background
    {0x0a, 0x14, 0x1f, 215},   // panel fill (translucent)
    {0x0a, 0x14, 0x1f},        // field fill
    {0x1d, 0x3a, 0x4d},        // panel border
    {0x14, 0x2a, 0x3a},        // grid line
    {0x35, 0xd6, 0xed},        // primary
    {0x1a, 0x6d, 0x85},        // primary dim
    {0xff, 0x9f, 0x1c},        // accent
    {0xff, 0x4d, 0x4d},        // alert
    {0x3d, 0xe0, 0x8a},        // ok
    {0xd7, 0xe7, 0xef},        // text primary
    {0x6f, 0x8a, 0x99},        // text dim
    1.0                        // glow
};

// Same language, daylight readable: pale deck, teal accent, ink text.
const PaletteDef kDaylight {
    {0xe9, 0xef, 0xf4},        // background
    {0xff, 0xff, 0xff, 225},   // panel fill (translucent)
    {0xff, 0xff, 0xff},        // field fill
    {0xb2, 0xc4, 0xd2},        // panel border
    {0xcd, 0xdb, 0xe5},        // grid line
    {0x0e, 0x7c, 0x93},        // primary
    {0x7c, 0xb8, 0xc7},        // primary dim
    {0xbc, 0x62, 0x0a},        // accent
    {0xc6, 0x2f, 0x33},        // alert
    {0x1a, 0x84, 0x55},        // ok
    {0x16, 0x25, 0x30},        // text primary
    {0x5e, 0x75, 0x84},        // text dim
    0.5                        // glow
};

const PaletteDef &definition(Style style)
{
    return style == Style::Daylight ? kDaylight : kMissionControl;
}

Style s_style = Style::MissionControl;

} // namespace

// Defined after the tables above so the static initialisation order holds.
QColor Background  = kMissionControl.background;
QColor PanelFill   = kMissionControl.panelFill;
QColor FieldFill   = kMissionControl.fieldFill;
QColor PanelBorder = kMissionControl.panelBorder;
QColor GridLine    = kMissionControl.gridLine;
QColor Primary     = kMissionControl.primary;
QColor PrimaryDim  = kMissionControl.primaryDim;
QColor Accent      = kMissionControl.accent;
QColor Alert       = kMissionControl.alert;
QColor Ok          = kMissionControl.ok;
QColor TextPrimary = kMissionControl.textPrimary;
QColor TextDim     = kMissionControl.textDim;
qreal GlowStrength = kMissionControl.glow;

Style style()
{
    return s_style;
}

void setStyle(Style style)
{
    const PaletteDef &def = definition(style);
    s_style     = style;
    Background  = def.background;
    PanelFill   = def.panelFill;
    FieldFill   = def.fieldFill;
    PanelBorder = def.panelBorder;
    GridLine    = def.gridLine;
    Primary     = def.primary;
    PrimaryDim  = def.primaryDim;
    Accent      = def.accent;
    Alert       = def.alert;
    Ok          = def.ok;
    TextPrimary = def.textPrimary;
    TextDim     = def.textDim;
    GlowStrength = def.glow;

    emit Notifier::instance()->styleChanged();
}

QVector<Style> styles()
{
    return {Style::MissionControl, Style::Daylight};
}

QString styleName(Style style)
{
    switch (style) {
    case Style::MissionControl: return QStringLiteral("Mission Control");
    case Style::Daylight:       return QStringLiteral("Daylight");
    }
    return QStringLiteral("Mission Control");
}

Style styleFromName(const QString &name, Style fallback)
{
    for (Style style : styles())
        if (styleName(style).compare(name, Qt::CaseInsensitive) == 0)
            return style;
    return fallback;
}

Notifier *Notifier::instance()
{
    static Notifier notifier;
    return &notifier;
}

QFont titleFont(int pointSize)
{
    QFont f(QStringList{"Avenir Next Condensed", "Helvetica Neue", "Arial"});
    f.setPointSize(qMax(6, pointSize));
    f.setWeight(QFont::DemiBold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    return f;
}

QFont labelFont(int pointSize)
{
    QFont f(QStringList{"Avenir Next Condensed", "Helvetica Neue", "Arial"});
    f.setPointSize(qMax(6, pointSize));
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    return f;
}

QFont valueFont(int pointSize)
{
    QFont f(QStringList{"Menlo", "Monaco", "Courier New"});
    f.setPointSize(qMax(6, pointSize));
    return f;
}

void drawGlowPath(QPainter &p, const QPainterPath &path,
                  const QColor &color, qreal width)
{
    p.save();
    p.setBrush(Qt::NoBrush);
    for (int i = 3; i >= 1; --i) {
        QColor halo = color;
        halo.setAlphaF(0.09 * i * GlowStrength);
        p.setPen(QPen(halo, width + i * 3.0, Qt::SolidLine, Qt::RoundCap));
        p.drawPath(path);
    }
    p.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(path);
    p.restore();
}

} // namespace nasagui::Theme
