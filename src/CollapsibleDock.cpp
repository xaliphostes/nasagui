#include "nasagui/CollapsibleDock.h"
#include "nasagui/Controls.h"
#include "nasagui/HudPanel.h"
#include "nasagui/Theme.h"

#include <QApplication>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QPainter>

namespace nasagui {

namespace {
constexpr int kStripThickness = 20;
constexpr int kAnimationMs = 240;
} // namespace

// ---- CollapsibleDock -------------------------------------------------------

CollapsibleDock::CollapsibleDock(Edge edge, const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_edge(edge)
    , m_title(title)
{
    m_expandedSize = collapsesHorizontally() ? 340 : 150;

    m_contentArea = new QWidget(this);
    auto *inner = new QVBoxLayout(m_contentArea);
    inner->setContentsMargins(0, 0, 0, 0);
    inner->setSpacing(0);
    // Left/Right docks stack their panels vertically, Top/Bottom docks place
    // them side by side; the splitter makes the separations draggable.
    m_splitter = new HudSplitter(
        collapsesHorizontally() ? Qt::Vertical : Qt::Horizontal, m_contentArea);
    inner->addWidget(m_splitter);

    m_strip = new DockStrip(this);

    auto *lay = collapsesHorizontally()
        ? static_cast<QBoxLayout *>(new QHBoxLayout(this))
        : static_cast<QBoxLayout *>(new QVBoxLayout(this));
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);
    // The strip faces the window center so it never leaves the screen edge.
    if (m_edge == Edge::Left || m_edge == Edge::Top) {
        lay->addWidget(m_contentArea);
        lay->addWidget(m_strip);
    } else {
        lay->addWidget(m_strip);
        lay->addWidget(m_contentArea);
    }

    m_anim = new QVariantAnimation(this);
    m_anim->setDuration(kAnimationMs);
    m_anim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_anim, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &v) { applySize(v.toInt()); });

    applySize(m_expandedSize);
}

void CollapsibleDock::setContent(QWidget *content)
{
    m_splitter->addWidget(content);
    watchPanels(content);
}

void CollapsibleDock::watchPanels(QWidget *content)
{
    QList<HudPanel *> found = content->findChildren<HudPanel *>();
    if (auto *panel = qobject_cast<HudPanel *>(content))
        found.prepend(panel);
    for (HudPanel *panel : found) {
        if (m_watchedPanels.contains(panel))
            continue;
        m_watchedPanels.append(panel);
        connect(panel, &HudPanel::panelClosed,
                this, &CollapsibleDock::updatePanelsState);
        connect(panel, &HudPanel::panelOpened,
                this, &CollapsibleDock::updatePanelsState);
    }
}

void CollapsibleDock::updatePanelsState()
{
    if (m_watchedPanels.isEmpty())
        return;
    bool anyOpen = false;
    for (const auto &panel : m_watchedPanels)
        if (panel && !panel->isHidden())
            anyOpen = true;
    setExpanded(anyOpen);
}

void CollapsibleDock::setExpandedSize(int px)
{
    m_expandedSize = qMax(0, px);
    if (m_expanded && m_anim->state() != QAbstractAnimation::Running)
        applySize(m_expandedSize);
}

void CollapsibleDock::setExpanded(bool expanded)
{
    if (m_expanded == expanded)
        return;
    m_expanded = expanded;

    m_anim->stop();
    const int current = collapsesHorizontally() ? m_contentArea->width()
                                                : m_contentArea->height();
    m_anim->setStartValue(current);
    m_anim->setEndValue(m_expanded ? m_expandedSize : 0);
    m_anim->start();

    m_strip->update();
    emit expandedChanged(m_expanded);
}

void CollapsibleDock::setExpandedImmediate(bool expanded)
{
    m_anim->stop();
    const bool changed = (m_expanded != expanded);
    m_expanded = expanded;
    applySize(expanded ? m_expandedSize : 0);
    m_strip->update();
    if (changed)
        emit expandedChanged(expanded);
}

void CollapsibleDock::applySize(int px)
{
    if (collapsesHorizontally())
        m_contentArea->setFixedWidth(px);
    else
        m_contentArea->setFixedHeight(px);
}

// ---- DockStrip -------------------------------------------------------------

DockStrip::DockStrip(CollapsibleDock *dock)
    : QWidget(dock)
    , m_dock(dock)
{
    const bool vertical = dock->edge() == CollapsibleDock::Edge::Left
                       || dock->edge() == CollapsibleDock::Edge::Right;
    if (vertical)
        setFixedWidth(kStripThickness);
    else
        setFixedHeight(kStripThickness);
    updateCursor();
    connect(dock, &CollapsibleDock::expandedChanged, this, [this] {
        updateCursor();
        update();
    });
    setToolTip(dock->title());
}

void DockStrip::updateCursor()
{
    const bool vertical = m_dock->edge() == CollapsibleDock::Edge::Left
                       || m_dock->edge() == CollapsibleDock::Edge::Right;
    // Expanded: the strip doubles as a resize divider. Collapsed: click-only.
    if (m_dock->isExpanded())
        setCursor(vertical ? Qt::SplitHCursor : Qt::SplitVCursor);
    else
        setCursor(Qt::PointingHandCursor);
}

void DockStrip::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    // Don't toggle yet: wait for the release to see if this was a drag.
    m_pressed = true;
    m_dragging = false;
    m_pressGlobal = event->globalPosition().toPoint();
    m_pressSize = m_dock->expandedSize();
    event->accept();
}

void DockStrip::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_pressed || !m_dock->isExpanded())
        return;
    const QPoint delta = event->globalPosition().toPoint() - m_pressGlobal;
    if (!m_dragging
        && delta.manhattanLength() < QApplication::startDragDistance())
        return;   // still within the click tolerance
    m_dragging = true;

    using Edge = CollapsibleDock::Edge;
    int grow = 0;
    switch (m_dock->edge()) {
    case Edge::Left:   grow = delta.x();  break;
    case Edge::Right:  grow = -delta.x(); break;
    case Edge::Top:    grow = delta.y();  break;
    case Edge::Bottom: grow = -delta.y(); break;
    }
    const bool vertical = m_dock->edge() == Edge::Left
                       || m_dock->edge() == Edge::Right;
    const int maxSize = (vertical ? m_dock->window()->width()
                                  : m_dock->window()->height()) - 160;
    m_dock->setExpandedSize(qBound(80, m_pressSize + grow, qMax(120, maxSize)));
    event->accept();
}

void DockStrip::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    if (m_pressed && !m_dragging)
        m_dock->toggle();   // plain click
    m_pressed = false;
    m_dragging = false;
    event->accept();
}

void DockStrip::enterEvent(QEnterEvent *) { update(); }
void DockStrip::leaveEvent(QEvent *) { update(); }

void DockStrip::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const bool hovered = underMouse();

    p.fillRect(r, Theme::PanelFill);
    if (hovered) {
        QColor h = Theme::Primary;
        h.setAlpha(22);
        p.fillRect(r, h);
    }
    p.setPen(QPen(Theme::PanelBorder, 1.0));
    p.drawRect(r);

    using Edge = CollapsibleDock::Edge;
    const Edge edge = m_dock->edge();
    const bool vertical = edge == Edge::Left || edge == Edge::Right;
    const bool expanded = m_dock->isExpanded();
    const QColor fg = hovered ? Theme::Primary : Theme::TextDim;

    // Chevrons point toward where the content will move on click.
    QPointF d;
    switch (edge) {
    case Edge::Left:   d = expanded ? QPointF(-1, 0) : QPointF(1, 0); break;
    case Edge::Right:  d = expanded ? QPointF(1, 0) : QPointF(-1, 0); break;
    case Edge::Top:    d = expanded ? QPointF(0, -1) : QPointF(0, 1); break;
    case Edge::Bottom: d = expanded ? QPointF(0, 1) : QPointF(0, -1); break;
    }
    const QPointF perp(-d.y(), d.x());
    auto chevron = [&](const QPointF &c) {
        p.drawPolyline(QPolygonF{c - d * 2.5 + perp * 3.5,
                                 c + d * 2.5,
                                 c - d * 2.5 - perp * 3.5});
    };
    auto doubleChevron = [&](const QPointF &c) {
        chevron(c - d * 3.0);
        chevron(c + d * 3.0);
    };

    p.setPen(QPen(fg, 1.5));
    p.setBrush(Qt::NoBrush);
    const QPointF center = r.center();
    if (vertical) {
        doubleChevron(QPointF(center.x(), r.top() + 16));
        doubleChevron(QPointF(center.x(), r.bottom() - 16));
    } else {
        doubleChevron(QPointF(r.left() + 16, center.y()));
        doubleChevron(QPointF(center.x(), center.y()));
        doubleChevron(QPointF(r.right() - 16, center.y()));
    }

    p.setPen(fg);
    p.setFont(Theme::titleFont(8));
    if (vertical) {
        p.save();
        p.translate(center);
        p.rotate(-90);
        p.drawText(QRectF(-r.height() / 2 + 30, -kStripThickness / 2.0,
                          r.height() - 60, kStripThickness),
                   Qt::AlignCenter, m_dock->title().toUpper());
        p.restore();
    } else {
        p.drawText(QRectF(r.left() + 34, r.top(), r.width() / 2 - 40, r.height()),
                   Qt::AlignVCenter | Qt::AlignLeft, m_dock->title().toUpper());
    }
}

} // namespace nasagui
