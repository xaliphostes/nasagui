#pragma once

#include <QColor>
#include <QWidget>

namespace nasagui {

// Container panel with translucent fill, thin border, corner brackets and a
// letter-spaced header. Put content in it with any QLayout.
class HudPanel : public QWidget
{
    Q_OBJECT
public:
    explicit HudPanel(const QString &title = {}, QWidget *parent = nullptr);

    void setTitle(const QString &title);
    QString title() const { return m_title; }

    void setAccent(const QColor &accent);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QColor m_accent;
};

} // namespace nasagui
