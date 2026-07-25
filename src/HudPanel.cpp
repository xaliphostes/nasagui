#include "nasagui/HudPanel.h"
#include "nasagui/Theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QVariantAnimation>

namespace nasagui {

HudPanel::HudPanel(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_accent(Theme::Primary)
{
    setContentsMargins(14, m_title.isEmpty() ? 14 : 40, 14, 14);

    m_sizeAnim = new QVariantAnimation(this);
    m_sizeAnim->setDuration(220);
    m_sizeAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_sizeAnim, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &v) { setMaximumHeight(v.toInt()); });
    connect(m_sizeAnim, &QVariantAnimation::finished, this, [this] {
        if (m_closing) {
            m_closing = false;
            hide();
            setMaximumHeight(QWIDGETSIZE_MAX);
            emit panelClosed();
        } else {
            setMaximumHeight(QWIDGETSIZE_MAX);
            emit panelOpened();
        }
    });
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

void HudPanel::setClosable(bool closable)
{
    m_closable = closable;
    setMouseTracking(closable);
    update();
}

QRectF HudPanel::closeButtonRect() const
{
    return QRectF(width() - 28.0, 9.0, 18.0, 18.0);
}

void HudPanel::closePanel()
{
    if (m_closing || (!isVisible() && m_sizeAnim->state() != QAbstractAnimation::Running))
        return;
    m_closing = true;
    m_sizeAnim->stop();
    m_sizeAnim->setStartValue(height());
    m_sizeAnim->setEndValue(0);
    m_sizeAnim->start();
}

void HudPanel::openPanel()
{
    if (isVisible() && !m_closing
        && m_sizeAnim->state() != QAbstractAnimation::Running)
        return;
    m_closing = false;
    m_sizeAnim->stop();
    setMaximumHeight(0);
    show();
    m_sizeAnim->setStartValue(0);
    m_sizeAnim->setEndValue(sizeHint().height());
    m_sizeAnim->start();
}

void HudPanel::mousePressEvent(QMouseEvent *event)
{
    if (m_closable && event->button() == Qt::LeftButton
        && closeButtonRect().contains(event->position())) {
        closePanel();
        return;
    }
    QWidget::mousePressEvent(event);
}

void HudPanel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_closable) {
        const bool hovered = closeButtonRect().contains(event->position());
        if (hovered != m_closeHovered) {
            m_closeHovered = hovered;
            setCursor(hovered ? Qt::PointingHandCursor : Qt::ArrowCursor);
            update();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void HudPanel::leaveEvent(QEvent *event)
{
    if (m_closeHovered) {
        m_closeHovered = false;
        setCursor(Qt::ArrowCursor);
        update();
    }
    QWidget::leaveEvent(event);
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

    if (!m_title.isEmpty()) {
        // Header: accent tick, title, separator line
        p.fillRect(QRectF(r.left() + 12, r.top() + 12, 3, 12), m_accent);
        p.setPen(Theme::TextPrimary);
        p.setFont(Theme::titleFont(10));
        p.drawText(QRectF(r.left() + 22, r.top() + 8, r.width() - 56, 20),
                   Qt::AlignVCenter | Qt::AlignLeft, m_title.toUpper());
        p.setPen(QPen(Theme::PanelBorder, 1.0));
        p.drawLine(QPointF(r.left() + 10, r.top() + 30),
                   QPointF(r.right() - 10, r.top() + 30));
    }

    if (m_closable) {
        const QRectF cr = closeButtonRect();
        if (m_closeHovered) {
            QColor fill = Theme::Alert;
            fill.setAlpha(30);
            p.fillRect(cr, fill);
            p.setPen(QPen(Theme::Alert, 1.0));
            p.drawRect(cr);
        }
        p.setPen(QPen(m_closeHovered ? Theme::Alert : Theme::TextDim, 1.4));
        const qreal in = 5.5;
        p.drawLine(cr.topLeft() + QPointF(in, in),
                   cr.bottomRight() - QPointF(in, in));
        p.drawLine(cr.topRight() + QPointF(-in, in),
                   cr.bottomLeft() + QPointF(in, -in));
    }
}

} // namespace nasagui
