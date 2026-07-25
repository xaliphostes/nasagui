#include "nasagui/CircularGauge.h"
#include "nasagui/Theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace nasagui {

namespace {
constexpr double kStartAngle = 225.0;  // degrees, CCW from 3 o'clock
constexpr double kSpan = 270.0;        // swept clockwise
} // namespace

CircularGauge::CircularGauge(QWidget *parent)
    : QWidget(parent)
{
}

void CircularGauge::setRange(double min, double max)
{
    m_min = min;
    m_max = qMax(max, min + 1e-9);
    update();
}

void CircularGauge::setLabel(const QString &label) { m_label = label; update(); }
void CircularGauge::setUnits(const QString &units) { m_units = units; update(); }
void CircularGauge::setDecimals(int decimals) { m_decimals = decimals; update(); }
void CircularGauge::setWarnFrom(double value) { m_warnFrom = value; update(); }

void CircularGauge::setValue(double value)
{
    m_value = qBound(m_min, value, m_max);
    update();
}

void CircularGauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double side = qMin(width(), height());
    const double margin = side * 0.10;
    const QRectF arcRect((width() - side) / 2 + margin,
                         (height() - side) / 2 + margin,
                         side - 2 * margin, side - 2 * margin);
    const QPointF c = arcRect.center();
    const double R = arcRect.width() / 2;

    // Track
    QPainterPath track;
    track.arcMoveTo(arcRect, kStartAngle);
    track.arcTo(arcRect, kStartAngle, -kSpan);
    p.setPen(QPen(Theme::GridLine, 4.0, Qt::SolidLine, Qt::FlatCap));
    p.drawPath(track);

    // Ticks (major every 5th)
    for (int i = 0; i <= 40; ++i) {
        const bool major = (i % 5 == 0);
        const double a = qDegreesToRadians(kStartAngle - kSpan * i / 40.0);
        const QPointF dir(qCos(a), -qSin(a));
        const double inner = major ? 0.80 : 0.85;
        p.setPen(QPen(major ? Theme::TextDim : Theme::GridLine, major ? 1.5 : 1.0));
        p.drawLine(c + dir * (R * inner), c + dir * (R * 0.90));
    }

    // Value arc
    const double frac = qBound(0.0, (m_value - m_min) / (m_max - m_min), 1.0);
    if (frac > 0.001) {
        QPainterPath arc;
        arc.arcMoveTo(arcRect, kStartAngle);
        arc.arcTo(arcRect, kStartAngle, -kSpan * frac);
        const QColor color = m_value >= m_warnFrom ? Theme::Accent : Theme::Primary;
        Theme::drawGlowPath(p, arc, color, 3.5);

        // Leading dot
        const double ea = qDegreesToRadians(kStartAngle - kSpan * frac);
        const QPointF end = c + QPointF(qCos(ea), -qSin(ea)) * R;
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(end, 3.5, 3.5);
        p.setBrush(Qt::NoBrush);
    }

    // Readout
    p.setPen(Theme::TextPrimary);
    p.setFont(Theme::valueFont(int(side * 0.15)));
    p.drawText(arcRect, Qt::AlignCenter, QString::number(m_value, 'f', m_decimals));

    p.setPen(Theme::TextDim);
    p.setFont(Theme::labelFont(int(side * 0.065)));
    QRectF unitsRect = arcRect;
    unitsRect.translate(0, side * 0.16);
    p.drawText(unitsRect, Qt::AlignCenter, m_units);

    // Label in the bottom gap of the arc
    p.setPen(Theme::TextDim);
    p.setFont(Theme::titleFont(int(side * 0.06)));
    p.drawText(QRectF(0, height() - side * 0.14, width(), side * 0.12),
               Qt::AlignHCenter | Qt::AlignVCenter, m_label.toUpper());
}

} // namespace nasagui
