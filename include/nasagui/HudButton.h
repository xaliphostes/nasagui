#pragma once

#include <QColor>
#include <QPushButton>

namespace nasagui {

// Flat bordered button with corner ticks; glows on hover, fills when checked.
class HudButton : public QPushButton
{
    Q_OBJECT
public:
    explicit HudButton(const QString &text, QWidget *parent = nullptr);

    void setAccent(const QColor &accent);

    QSize sizeHint() const override { return {112, 34}; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QColor m_accent;
};

} // namespace nasagui
