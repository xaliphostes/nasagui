#include "nasagui/Controls.h"
#include "nasagui/Theme.h"

#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace nasagui {

namespace {

void drawDoubleChevron(QPainter &p, const QPointF &c, const QPointF &d,
                       const QColor &color, qreal size = 3.0)
{
    const QPointF perp(-d.y(), d.x());
    p.setPen(QPen(color, 1.4));
    p.setBrush(Qt::NoBrush);
    for (double off : {-2.5, 2.5}) {
        const QPointF cc = c + d * off;
        p.drawPolyline(QPolygonF{cc - d * (size * 0.6) + perp * size,
                                 cc + d * (size * 0.6),
                                 cc - d * (size * 0.6) - perp * size});
    }
}

} // namespace

// ---- HudCheckBox -----------------------------------------------------------

HudCheckBox::HudCheckBox(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
{
    setCursor(Qt::PointingHandCursor);
}

QSize HudCheckBox::sizeHint() const
{
    const QFontMetrics fm(Theme::labelFont(9));
    return {26 + fm.horizontalAdvance(text().toUpper()), 22};
}

void HudCheckBox::enterEvent(QEnterEvent *) { update(); }
void HudCheckBox::leaveEvent(QEvent *) { update(); }

void HudCheckBox::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const bool on = isChecked();
    const bool hovered = underMouse() && isEnabled();
    const QRectF box(1.5, height() / 2.0 - 7, 14, 14);

    p.fillRect(box, QColor(0x0a, 0x14, 0x1f));
    p.setPen(QPen(on || hovered ? Theme::Primary : Theme::PanelBorder, 1.2));
    p.drawRect(box);

    if (on) {
        QPainterPath check;
        check.moveTo(box.left() + 3.0, box.center().y() + 1.0);
        check.lineTo(box.center().x() - 0.5, box.bottom() - 3.5);
        check.lineTo(box.right() - 2.0, box.top() + 2.5);
        Theme::drawGlowPath(p, check, Theme::Primary, 1.7);
    }

    p.setPen(isEnabled() ? Theme::TextPrimary : Theme::TextDim);
    p.setFont(Theme::labelFont(9));
    p.drawText(QRectF(24, 0, width() - 24, height()),
               Qt::AlignVCenter | Qt::AlignLeft, text().toUpper());
}

// ---- HudRadioButton --------------------------------------------------------

HudRadioButton::HudRadioButton(const QString &text, QWidget *parent)
    : QRadioButton(text, parent)
{
    setCursor(Qt::PointingHandCursor);
}

QSize HudRadioButton::sizeHint() const
{
    const QFontMetrics fm(Theme::labelFont(9));
    return {26 + fm.horizontalAdvance(text().toUpper()), 22};
}

void HudRadioButton::enterEvent(QEnterEvent *) { update(); }
void HudRadioButton::leaveEvent(QEvent *) { update(); }

void HudRadioButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const bool on = isChecked();
    const bool hovered = underMouse() && isEnabled();
    const QPointF c(9.0, height() / 2.0);

    p.setBrush(QColor(0x0a, 0x14, 0x1f));
    p.setPen(QPen(on || hovered ? Theme::Primary : Theme::PanelBorder, 1.2));
    p.drawEllipse(c, 7.0, 7.0);

    if (on) {
        QColor halo = Theme::Primary;
        halo.setAlpha(70);
        p.setPen(Qt::NoPen);
        p.setBrush(halo);
        p.drawEllipse(c, 5.5, 5.5);
        p.setBrush(Theme::Primary);
        p.drawEllipse(c, 3.0, 3.0);
    }

    p.setPen(isEnabled() ? Theme::TextPrimary : Theme::TextDim);
    p.setFont(Theme::labelFont(9));
    p.drawText(QRectF(24, 0, width() - 24, height()),
               Qt::AlignVCenter | Qt::AlignLeft, text().toUpper());
}

// ---- HudComboBox -----------------------------------------------------------

HudComboBox::HudComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(28);
    setView(new QListView(this));   // so the applyTheme() popup QSS applies
}

void HudComboBox::enterEvent(QEnterEvent *) { update(); }
void HudComboBox::leaveEvent(QEvent *) { update(); }

void HudComboBox::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const bool active = (underMouse() || hasFocus()) && isEnabled();

    p.fillRect(r, QColor(0x0a, 0x14, 0x1f));
    p.setPen(QPen(active ? Theme::Primary : Theme::PanelBorder, 1.0));
    p.drawRect(r);

    p.setPen(isEnabled() ? Theme::TextPrimary : Theme::TextDim);
    p.setFont(Theme::labelFont(9));
    p.drawText(r.adjusted(10, 0, -28, 0), Qt::AlignVCenter | Qt::AlignLeft,
               currentText().toUpper());

    drawDoubleChevron(p, QPointF(r.right() - 14, r.center().y()), QPointF(0, 1),
                      active ? Theme::Primary : Theme::TextDim);
}

// ---- HudLineEdit -----------------------------------------------------------

HudLineEdit::HudLineEdit(const QString &text, QWidget *parent)
    : QLineEdit(text, parent)
{
}

void HudLineEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);

    QPainter p(this);
    const QRectF r = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
    const qreal len = 5.0;
    p.setPen(QPen(hasFocus() ? Theme::Primary : Theme::PrimaryDim, 1.5));
    const QPointF tl = r.topLeft(), tr = r.topRight();
    const QPointF bl = r.bottomLeft(), br = r.bottomRight();
    p.drawPolyline(QPolygonF{{tl.x(), tl.y() + len}, tl, {tl.x() + len, tl.y()}});
    p.drawPolyline(QPolygonF{{tr.x() - len, tr.y()}, tr, {tr.x(), tr.y() + len}});
    p.drawPolyline(QPolygonF{{bl.x(), bl.y() - len}, bl, {bl.x() + len, bl.y()}});
    p.drawPolyline(QPolygonF{{br.x() - len, br.y()}, br, {br.x(), br.y() - len}});
}

// ---- HudSlider -------------------------------------------------------------

namespace {
constexpr int kSliderMargin = 10;
}

HudSlider::HudSlider(QWidget *parent)
    : QSlider(Qt::Horizontal, parent)
{
    setCursor(Qt::PointingHandCursor);
}

int HudSlider::valueForPos(const QPoint &pos) const
{
    const double span = qMax(1, width() - 2 * kSliderMargin);
    const double frac = qBound(0.0, (pos.x() - kSliderMargin) / span, 1.0);
    return minimum() + qRound(frac * (maximum() - minimum()));
}

void HudSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setSliderDown(true);
        setValue(valueForPos(event->pos()));
        event->accept();
        return;
    }
    QSlider::mousePressEvent(event);
}

void HudSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (isSliderDown()) {
        setValue(valueForPos(event->pos()));
        event->accept();
        return;
    }
    QSlider::mouseMoveEvent(event);
}

void HudSlider::mouseReleaseEvent(QMouseEvent *event)
{
    setSliderDown(false);
    QSlider::mouseReleaseEvent(event);
}

void HudSlider::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double y = height() / 2.0;
    const double left = kSliderMargin, right = width() - kSliderMargin;

    // Track and ticks
    p.setPen(QPen(Theme::GridLine, 2.0));
    p.drawLine(QPointF(left, y), QPointF(right, y));
    p.setPen(QPen(Theme::GridLine, 1.0));
    for (int i = 0; i <= 10; ++i) {
        const double x = left + (right - left) * i / 10.0;
        p.drawLine(QPointF(x, y + 5), QPointF(x, y + 8));
    }

    const double range = qMax(1, maximum() - minimum());
    const double frac = (value() - minimum()) / range;
    const double hx = left + frac * (right - left);

    // Glowing fill
    if (frac > 0.001) {
        QPainterPath fill;
        fill.moveTo(left, y);
        fill.lineTo(hx, y);
        Theme::drawGlowPath(p, fill, Theme::Primary, 2.0);
    }

    // Handle
    QColor halo = Theme::Primary;
    halo.setAlpha(55);
    p.setPen(Qt::NoPen);
    p.setBrush(halo);
    p.drawEllipse(QPointF(hx, y), 8.0, 8.0);
    p.setBrush(Theme::Primary);
    p.drawRect(QRectF(hx - 2.5, y - 8, 5, 16));
}

// ---- HudSpinBox ------------------------------------------------------------

namespace {
constexpr int kSpinZone = 24;   // width of the +/- chevron column
}

HudSpinBox::HudSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent)
{
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setDecimals(0);
    setMinimumHeight(28);
    setStyleSheet(QStringLiteral("QDoubleSpinBox { padding-right: %1px; }")
                      .arg(kSpinZone + 2));
}

void HudSpinBox::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->pos().x() > width() - kSpinZone) {
        if (event->pos().y() < height() / 2)
            stepUp();
        else
            stepDown();
        event->accept();
        return;
    }
    QDoubleSpinBox::mousePressEvent(event);
}

void HudSpinBox::paintEvent(QPaintEvent *event)
{
    QDoubleSpinBox::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const double x = width() - kSpinZone;
    p.setPen(QPen(Theme::PanelBorder, 1.0));
    p.drawLine(QPointF(x, 4), QPointF(x, height() - 4));

    const QColor fg = isEnabled() ? Theme::TextDim : Theme::GridLine;
    drawDoubleChevron(p, QPointF(x + kSpinZone / 2.0, height() * 0.30),
                      QPointF(0, -1), fg, 2.5);
    drawDoubleChevron(p, QPointF(x + kSpinZone / 2.0, height() * 0.70),
                      QPointF(0, 1), fg, 2.5);
}

// ---- HudDial ---------------------------------------------------------------

HudDial::HudDial(QWidget *parent)
    : QDial(parent)
{
    setCursor(Qt::PointingHandCursor);
}

void HudDial::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double side = qMin(width(), height());
    const double margin = side * 0.12;
    const QRectF arcRect((width() - side) / 2 + margin,
                         (height() - side) / 2 + margin,
                         side - 2 * margin, side - 2 * margin);
    const QPointF c = arcRect.center();
    const double R = arcRect.width() / 2;

    // QDial maps mouse angles to values over a 300-degree span starting at
    // 240 (8 o'clock) and ending at -60 (4 o'clock); the painting matches it.
    constexpr double kStart = 240.0;
    constexpr double kSpan = 300.0;

    QPainterPath track;
    track.arcMoveTo(arcRect, kStart);
    track.arcTo(arcRect, kStart, -kSpan);
    p.setPen(QPen(Theme::GridLine, 3.0, Qt::SolidLine, Qt::FlatCap));
    p.setBrush(Qt::NoBrush);
    p.drawPath(track);

    for (int i = 0; i <= 10; ++i) {
        const double a = qDegreesToRadians(kStart - kSpan * i / 10.0);
        const QPointF dir(qCos(a), -qSin(a));
        p.setPen(QPen(Theme::GridLine, 1.0));
        p.drawLine(c + dir * (R * 0.76), c + dir * (R * 0.88));
    }

    const double range = qMax(1, maximum() - minimum());
    const double frac = (value() - minimum()) / range;
    const QColor color = isEnabled() ? Theme::Primary : Theme::TextDim;

    if (frac > 0.001) {
        QPainterPath arc;
        arc.arcMoveTo(arcRect, kStart);
        arc.arcTo(arcRect, kStart, -kSpan * frac);
        Theme::drawGlowPath(p, arc, color, 2.5);
    }

    // Needle
    const double a = qDegreesToRadians(kStart - kSpan * frac);
    const QPointF dir(qCos(a), -qSin(a));
    QColor halo = color;
    halo.setAlpha(70);
    p.setPen(QPen(halo, 4.0, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(c + dir * (R * 0.18), c + dir * (R * 0.70));
    p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(c + dir * (R * 0.18), c + dir * (R * 0.70));

    p.setPen(QPen(color, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, 3.0, 3.0);
}

// ---- HudMenu ---------------------------------------------------------------

void HudMenu::mouseReleaseEvent(QMouseEvent *event)
{
    QAction *action = actionAt(event->pos());
    if (action && action->isCheckable() && action->isEnabled()) {
        action->trigger();   // toggle without closing the menu
        event->accept();
        return;
    }
    QMenu::mouseReleaseEvent(event);
}

// ---- HudLabel --------------------------------------------------------------

HudLabel::HudLabel(const QString &text, Role role, QWidget *parent)
    : QLabel(text, parent)
    , m_role(role)
{
    apply();
}

void HudLabel::setRole(Role role)
{
    m_role = role;
    apply();
}

void HudLabel::setAccent(const QColor &color)
{
    m_accent = color;
    apply();
}

void HudLabel::apply()
{
    QColor color;
    switch (m_role) {
    case Role::Title:
        setFont(Theme::titleFont(11));
        color = Theme::TextPrimary;
        setText(text().toUpper());
        break;
    case Role::Caption:
        setFont(Theme::titleFont(8));
        color = Theme::TextDim;
        setText(text().toUpper());
        break;
    case Role::Value:
        setFont(Theme::valueFont(13));
        color = Theme::TextPrimary;
        break;
    case Role::Unit:
        setFont(Theme::labelFont(8));
        color = Theme::TextDim;
        break;
    }
    if (m_accent.isValid())
        color = m_accent;
    setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                      .arg(color.name()));
}

} // namespace nasagui
