#include "nasagui/HudButton.h"
#include "nasagui/Theme.h"

#include <QPainter>

namespace nasagui {

HudButton::HudButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
    , m_accent(Theme::Primary)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
}

void HudButton::setAccent(const QColor &accent)
{
    m_accent = accent;
    update();
}

void HudButton::enterEvent(QEnterEvent *) { update(); }
void HudButton::leaveEvent(QEvent *) { update(); }

void HudButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
    const bool active = isDown() || isChecked();
    const bool hovered = underMouse() && isEnabled();

    if (active) {
        QColor fill = m_accent;
        fill.setAlpha(45);
        p.fillRect(r, fill);
    } else if (hovered) {
        QColor fill = m_accent;
        fill.setAlpha(20);
        p.fillRect(r, fill);
    }

    p.setPen(QPen(active || hovered ? m_accent : Theme::PanelBorder, 1.0));
    p.drawRect(r);

    // Corner ticks
    const qreal len = 6.0;
    p.setPen(QPen(m_accent, 2.0));
    const QPointF tl = r.topLeft(), tr = r.topRight();
    const QPointF bl = r.bottomLeft(), br = r.bottomRight();
    p.drawPolyline(QPolygonF{{tl.x(), tl.y() + len}, tl, {tl.x() + len, tl.y()}});
    p.drawPolyline(QPolygonF{{tr.x() - len, tr.y()}, tr, {tr.x(), tr.y() + len}});
    p.drawPolyline(QPolygonF{{bl.x(), bl.y() - len}, bl, {bl.x() + len, bl.y()}});
    p.drawPolyline(QPolygonF{{br.x() - len, br.y()}, br, {br.x(), br.y() - len}});

    p.setPen(isEnabled() ? (active || hovered ? m_accent : Theme::TextPrimary)
                         : Theme::TextDim);
    p.setFont(Theme::titleFont(9));
    p.drawText(r, Qt::AlignCenter, text().toUpper());
}

} // namespace nasagui
