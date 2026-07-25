#pragma once

#include <QPointer>
#include <QVariantAnimation>
#include <QVector>
#include <QWidget>

namespace nasagui {

class DockStrip;
class HudPanel;

// Edge-anchored collapsible container. A slim clickable strip (with rotated
// title and chevrons) stays visible when collapsed; expanding/collapsing is
// animated. Place it around your central widget in a QBoxLayout.
class CollapsibleDock : public QWidget
{
    Q_OBJECT
public:
    enum class Edge { Left, Right, Top, Bottom };

    explicit CollapsibleDock(Edge edge, const QString &title,
                             QWidget *parent = nullptr);

    // Reparented into the dock. Any HudPanel inside the content is watched:
    // when the last one is closed the dock auto-collapses, and reopening a
    // panel auto-expands it again.
    void setContent(QWidget *content);
    void setExpandedSize(int px);        // width (Left/Right) or height (Top/Bottom)

    Edge edge() const { return m_edge; }
    QString title() const { return m_title; }
    bool isExpanded() const { return m_expanded; }

public slots:
    void setExpanded(bool expanded);
    void toggle() { setExpanded(!m_expanded); }

signals:
    void expandedChanged(bool expanded);

private:
    bool collapsesHorizontally() const
    {
        return m_edge == Edge::Left || m_edge == Edge::Right;
    }
    void applySize(int px);
    void watchPanels(QWidget *content);
    void updatePanelsState();

    Edge m_edge;
    QString m_title;
    bool m_expanded = true;
    int m_expandedSize;
    QWidget *m_contentArea = nullptr;
    DockStrip *m_strip = nullptr;
    QVariantAnimation *m_anim = nullptr;
    QVector<QPointer<HudPanel>> m_watchedPanels;
};

// Internal: the always-visible toggle strip.
class DockStrip : public QWidget
{
    Q_OBJECT
public:
    explicit DockStrip(CollapsibleDock *dock);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    CollapsibleDock *m_dock;
};

} // namespace nasagui
