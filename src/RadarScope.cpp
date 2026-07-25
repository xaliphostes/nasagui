#include "nasagui/RadarScope.h"
#include "nasagui/Theme.h"

#include <QPainter>
#include <QtMath>

namespace nasagui {

RadarScope::RadarScope(QWidget *parent)
    : QWidget(parent)
{
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        m_sweep = std::fmod(m_sweep + 2.2, 360.0);
        update();
    });
    m_timer.start();
}

void RadarScope::addContact(double angleDeg, double radiusFrac, bool hostile)
{
    m_contacts.append({std::fmod(angleDeg + 360.0, 360.0),
                       qBound(0.0, radiusFrac, 1.0), hostile});
    update();
}

void RadarScope::clearContacts()
{
    m_contacts.clear();
    update();
}

void RadarScope::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double side = qMin(width(), height());
    const QPointF c(width() / 2.0, height() / 2.0);
    const double R = side / 2.0 - 24.0;

    auto polar = [&](double angleDeg, double radius) {
        const double a = qDegreesToRadians(angleDeg);
        return c + QPointF(qCos(a) * radius, -qSin(a) * radius);
    };

    // Rings and crosshair
    p.setPen(QPen(Theme::GridLine, 1.0));
    p.setBrush(Qt::NoBrush);
    for (double f : {0.33, 0.66, 1.0})
        p.drawEllipse(c, R * f, R * f);
    p.drawLine(polar(0, R), polar(180, R));
    p.drawLine(polar(90, R), polar(270, R));

    // Bearing labels
    p.setPen(Theme::TextDim);
    p.setFont(Theme::valueFont(7));
    const struct { double a; const char *t; } bearings[] =
        {{90, "000"}, {0, "090"}, {270, "180"}, {180, "270"}};
    for (const auto &b : bearings) {
        const QPointF pos = polar(b.a, R + 9);
        p.drawText(QRectF(pos.x() - 12, pos.y() - 7, 24, 14),
                   Qt::AlignCenter, b.t);
    }

    // Sweep with trailing fade
    for (int k = 45; k >= 0; --k) {
        QColor trail = Theme::Primary;
        trail.setAlphaF(0.45 * (1.0 - k / 46.0));
        p.setPen(QPen(trail, k == 0 ? 2.0 : 1.5));
        p.drawLine(c, polar(m_sweep - k * 1.4, R));
    }

    // Contacts fade after the sweep passes them
    for (const auto &ct : m_contacts) {
        const double behind = std::fmod(m_sweep - ct.angle + 720.0, 360.0);
        const double alpha = qMax(0.0, 1.0 - behind / 220.0);
        if (alpha <= 0.02)
            continue;
        QColor color = ct.hostile ? Theme::Accent : Theme::Ok;
        const QPointF pos = polar(ct.angle, ct.radius * R);
        QColor halo = color;
        halo.setAlphaF(alpha * 0.30);
        p.setPen(Qt::NoPen);
        p.setBrush(halo);
        p.drawEllipse(pos, 7, 7);
        color.setAlphaF(alpha);
        p.setBrush(color);
        p.drawEllipse(pos, 3, 3);
    }

    // Center marker
    p.setPen(QPen(Theme::Primary, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, 3, 3);
}

} // namespace nasagui
