#include "nasagui/TelemetryPlot.h"
#include "nasagui/Theme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

namespace nasagui {

TelemetryPlot::TelemetryPlot(QWidget *parent)
    : QWidget(parent)
{
    m_data.reserve(m_capacity);
}

void TelemetryPlot::setCapacity(int samples)
{
    m_capacity = qMax(2, samples);
    while (m_data.size() > m_capacity)
        m_data.removeFirst();
    update();
}

void TelemetryPlot::setYRange(double min, double max)
{
    m_yMin = min;
    m_yMax = qMax(max, min + 1e-9);
    m_autoRange = false;
    update();
}

void TelemetryPlot::setAutoRange(bool on) { m_autoRange = on; update(); }
void TelemetryPlot::setUnits(const QString &units) { m_units = units; update(); }

void TelemetryPlot::append(double sample)
{
    m_data.append(sample);
    while (m_data.size() > m_capacity)
        m_data.removeFirst();
    update();
}

void TelemetryPlot::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = QRectF(rect()).adjusted(44, 8, -8, -18);

    // Grid
    p.setPen(QPen(Theme::GridLine, 1.0));
    const int vLines = 8, hLines = 4;
    for (int i = 0; i <= vLines; ++i) {
        const double x = r.left() + r.width() * i / vLines;
        p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
    }
    for (int i = 0; i <= hLines; ++i) {
        const double y = r.top() + r.height() * i / hLines;
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
    }

    double yMin = m_yMin, yMax = m_yMax;
    if (m_autoRange && m_data.size() >= 2) {
        const auto [lo, hi] = std::minmax_element(m_data.cbegin(), m_data.cend());
        const double pad = qMax(1e-9, (*hi - *lo) * 0.15);
        yMin = *lo - pad;
        yMax = *hi + pad;
    }

    // Axis labels
    p.setPen(Theme::TextDim);
    p.setFont(Theme::valueFont(8));
    for (int i = 0; i <= hLines; ++i) {
        const double v = yMax - (yMax - yMin) * i / hLines;
        const double y = r.top() + r.height() * i / hLines;
        p.drawText(QRectF(0, y - 8, 40, 16), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(v, 'f', 0));
    }

    if (m_data.size() < 2)
        return;

    // Polyline, newest sample pinned to the right edge
    const double step = r.width() / double(m_capacity - 1);
    QPainterPath line;
    for (int i = 0; i < m_data.size(); ++i) {
        const double x = r.right() - (m_data.size() - 1 - i) * step;
        const double t = (m_data[i] - yMin) / (yMax - yMin);
        const double y = r.bottom() - qBound(0.0, t, 1.0) * r.height();
        if (i == 0)
            line.moveTo(x, y);
        else
            line.lineTo(x, y);
    }

    // Gradient fill under the curve
    QPainterPath fill = line;
    fill.lineTo(r.right(), r.bottom());
    fill.lineTo(r.right() - (m_data.size() - 1) * step, r.bottom());
    fill.closeSubpath();
    QLinearGradient grad(r.topLeft(), r.bottomLeft());
    QColor top = Theme::Primary;
    top.setAlpha(70);
    grad.setColorAt(0.0, top);
    grad.setColorAt(1.0, Qt::transparent);
    p.fillPath(fill, grad);

    Theme::drawGlowPath(p, line, Theme::Primary, 1.6);

    // Live readout
    p.setPen(Theme::Primary);
    p.setFont(Theme::valueFont(11));
    p.drawText(QRectF(r.left(), r.top() + 2, r.width() - 8, 18),
               Qt::AlignRight | Qt::AlignTop,
               QString::number(m_data.last(), 'f', 1) + " " + m_units);
}

} // namespace nasagui
