#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDial>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QRadioButton>
#include <QSlider>

// Interactive form controls in the nasagui HUD style. All of them are drop-in
// replacements for their Qt base class (same signals, slots and API).

namespace nasagui {

// Square indicator with a glowing check mark.
class HudCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    explicit HudCheckBox(const QString &text, QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

// Circular indicator with a glowing dot.
class HudRadioButton : public QRadioButton
{
    Q_OBJECT
public:
    explicit HudRadioButton(const QString &text, QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

// Dark field with chevron; the popup list is styled by applyTheme().
class HudComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit HudComboBox(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

// Standard line edit plus corner ticks that light up on focus.
class HudLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit HudLineEdit(const QString &text = {}, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

// Horizontal slider with glowing fill and a bar handle.
class HudSlider : public QSlider
{
    Q_OBJECT
public:
    explicit HudSlider(QWidget *parent = nullptr);
    QSize sizeHint() const override { return {160, 26}; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int valueForPos(const QPoint &pos) const;
};

// Spin box with +/- chevron zones on the right (double; setDecimals(0) for int).
class HudSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit HudSpinBox(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

// Rotary dial painted as a mini gauge (track, glow arc, needle).
class HudDial : public QDial
{
    Q_OBJECT
public:
    explicit HudDial(QWidget *parent = nullptr);
    QSize sizeHint() const override { return {84, 84}; }

protected:
    void paintEvent(QPaintEvent *event) override;
};

// QMenu that stays open when a checkable action is clicked, so several
// items can be toggled in one visit (show/hide lists, option sets, ...).
// Non-checkable actions close the menu as usual.
class HudMenu : public QMenu
{
    Q_OBJECT
public:
    using QMenu::QMenu;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

// QLabel preconfigured for a typographic role of the theme.
class HudLabel : public QLabel
{
    Q_OBJECT
public:
    enum class Role { Title, Caption, Value, Unit };

    explicit HudLabel(const QString &text, Role role = Role::Caption,
                      QWidget *parent = nullptr);

    void setRole(Role role);
    void setAccent(const QColor &color);   // overrides the role color

private:
    void apply();

    Role m_role;
    QColor m_accent;      // invalid = use role color
};

} // namespace nasagui
