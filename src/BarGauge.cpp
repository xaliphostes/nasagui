#include "nasagui/BarGauge.h"
#include "nasagui/Theme.h"

#include <QPainter>

namespace nasagui {

BarGauge::BarGauge(QWidget *parent)
    : QWidget(parent)
{
}

void BarGauge::setRange(double min, double max)
{
    m_min = min;
    m_max = qMax(max, min + 1e-9);
    update();
}

void BarGauge::setLabel(const QString &label) { m_label = label; update(); }
void BarGauge::setSuffix(const QString &suffix) { m_suffix = suffix; update(); }
void BarGauge::setSegmentCount(int count) { m_segments = qMax(2, count); update(); }

void BarGauge::setValue(double value)
{
    m_value = qBound(m_min, value, m_max);
    update();
}

void BarGauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int topText = 20;
    const int bottomText = 20;
    const QRectF bar(width() * 0.30, topText,
                     width() * 0.40, height() - topText - bottomText);

    const double frac = qBound(0.0, (m_value - m_min) / (m_max - m_min), 1.0);
    const int lit = qRound(frac * m_segments);
    const double gap = 2.0;
    const double segH = (bar.height() - gap * (m_segments - 1)) / m_segments;

    for (int i = 0; i < m_segments; ++i) {
        // i = 0 is the bottom segment
        const double y = bar.bottom() - segH - i * (segH + gap);
        QColor color = Theme::GridLine;
        if (i < lit) {
            const double level = double(i + 1) / m_segments;
            color = level > 0.85 ? Theme::Accent : Theme::Primary;
        }
        p.fillRect(QRectF(bar.left(), y, bar.width(), segH), color);
    }

    p.setPen(QPen(Theme::PanelBorder, 1.0));
    p.drawRect(bar.adjusted(-2, -2, 2, 2));

    // Numeric readout above, label below
    p.setPen(Theme::TextPrimary);
    p.setFont(Theme::valueFont(9));
    p.drawText(QRectF(0, 0, width(), topText - 2), Qt::AlignCenter,
               QString::number(m_value, 'f', 0) + m_suffix);

    p.setPen(Theme::TextDim);
    p.setFont(Theme::titleFont(8));
    p.drawText(QRectF(0, height() - bottomText + 2, width(), bottomText - 2),
               Qt::AlignCenter, m_label.toUpper());
}

} // namespace nasagui
