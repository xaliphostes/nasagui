#pragma once

#include <QColor>
#include <QWidget>

class QVariantAnimation;

namespace nasagui {

// Container panel with translucent fill, thin border, corner brackets and a
// letter-spaced header. Put content in it with any QLayout.
// With setClosable(true) the header gains a ✕ button that collapses the
// panel shut (animated); openPanel() expands it back.
class HudPanel : public QWidget
{
    Q_OBJECT
public:
    explicit HudPanel(const QString &title = {}, QWidget *parent = nullptr);

    void setTitle(const QString &title);
    QString title() const { return m_title; }

    void setAccent(const QColor &accent);

    void setClosable(bool closable);
    bool isClosable() const { return m_closable; }

public slots:
    void closePanel();   // animated collapse, then hide() + panelClosed()
    void openPanel();    // show() + animated expand, then panelOpened()

signals:
    void panelClosed();
    void panelOpened();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRectF closeButtonRect() const;

    QString m_title;
    QColor m_accent;
    bool m_closable = false;
    bool m_closeHovered = false;
    bool m_closing = false;
    QVariantAnimation *m_sizeAnim = nullptr;
};

} // namespace nasagui
