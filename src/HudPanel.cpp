#include "nasagui/HudPanel.h"
#include "nasagui/Theme.h"

#include <QPainter>

namespace nasagui {

HudPanel::HudPanel(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_accent(Theme::Primary)
{
    setContentsMargins(14, m_title.isEmpty() ? 14 : 40, 14, 14);
}

void HudPanel::setTitle(const QString &title)
{
    m_title = title;
    setContentsMargins(14, m_title.isEmpty() ? 14 : 40, 14, 14);
    update();
}

void HudPanel::setAccent(const QColor &accent)
{
    m_accent = accent;
    update();
}

void HudPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);

    p.fillRect(r, Theme::PanelFill);
    p.setPen(QPen(Theme::PanelBorder, 1.0));
    p.drawRect(r);

    // Corner brackets
    const qreal len = 14.0;
    p.setPen(QPen(m_accent, 2.0));
    const QPointF tl = r.topLeft(), tr = r.topRight();
    const QPointF bl = r.bottomLeft(), br = r.bottomRight();
    p.drawPolyline(QPolygonF{{tl.x(), tl.y() + len}, tl, {tl.x() + len, tl.y()}});
    p.drawPolyline(QPolygonF{{tr.x() - len, tr.y()}, tr, {tr.x(), tr.y() + len}});
    p.drawPolyline(QPolygonF{{bl.x(), bl.y() - len}, bl, {bl.x() + len, bl.y()}});
    p.drawPolyline(QPolygonF{{br.x() - len, br.y()}, br, {br.x(), br.y() - len}});

    if (m_title.isEmpty())
        return;

    // Header: accent tick, title, separator line
    p.fillRect(QRectF(r.left() + 12, r.top() + 12, 3, 12), m_accent);
    p.setPen(Theme::TextPrimary);
    p.setFont(Theme::titleFont(10));
    p.drawText(QRectF(r.left() + 22, r.top() + 8, r.width() - 34, 20),
               Qt::AlignVCenter | Qt::AlignLeft, m_title.toUpper());
    p.setPen(QPen(Theme::PanelBorder, 1.0));
    p.drawLine(QPointF(r.left() + 10, r.top() + 30),
               QPointF(r.right() - 10, r.top() + 30));
}

} // namespace nasagui
