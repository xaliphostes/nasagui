#include "nasagui/StatusIndicator.h"
#include "nasagui/Theme.h"

#include <QPainter>

namespace nasagui {

StatusIndicator::StatusIndicator(const QString &name, QWidget *parent)
    : QWidget(parent)
    , m_name(name)
{
    m_blinkTimer.setInterval(450);
    connect(&m_blinkTimer, &QTimer::timeout, this, [this] {
        m_blinkOn = !m_blinkOn;
        update();
    });
}

void StatusIndicator::setStatus(Status status)
{
    if (m_status == status)
        return;
    m_status = status;
    m_blinkOn = true;
    if (m_status == Status::Alert)
        m_blinkTimer.start();
    else
        m_blinkTimer.stop();
    update();
}

void StatusIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor color;
    QString word;
    switch (m_status) {
    case Status::Off:     color = Theme::GridLine; word = "OFFLINE"; break;
    case Status::Nominal: color = Theme::Ok;       word = "NOMINAL"; break;
    case Status::Warning: color = Theme::Accent;   word = "CAUTION"; break;
    case Status::Alert:   color = Theme::Alert;    word = "ALERT";   break;
    }

    // Light with glow halo
    const QRectF light(4, height() / 2.0 - 4, 8, 8);
    if (m_status != Status::Off && m_blinkOn) {
        QColor halo = color;
        halo.setAlpha(60);
        p.setPen(Qt::NoPen);
        p.setBrush(halo);
        p.drawEllipse(light.center(), 8, 8);
        p.setBrush(color);
        p.drawEllipse(light);
    } else {
        p.setPen(QPen(color, 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(light);
    }

    p.setPen(Theme::TextPrimary);
    p.setFont(Theme::labelFont(9));
    p.drawText(QRectF(22, 0, width() - 90, height()),
               Qt::AlignVCenter | Qt::AlignLeft, m_name.toUpper());

    p.setPen(color);
    p.setFont(Theme::valueFont(8));
    p.drawText(QRectF(width() - 70, 0, 66, height()),
               Qt::AlignVCenter | Qt::AlignRight, word);
}

} // namespace nasagui
