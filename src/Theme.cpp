#include "nasagui/Theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStringList>

namespace nasagui::Theme {

QFont titleFont(int pointSize)
{
    QFont f(QStringList{"Avenir Next Condensed", "Helvetica Neue", "Arial"});
    f.setPointSize(qMax(6, pointSize));
    f.setWeight(QFont::DemiBold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    return f;
}

QFont labelFont(int pointSize)
{
    QFont f(QStringList{"Avenir Next Condensed", "Helvetica Neue", "Arial"});
    f.setPointSize(qMax(6, pointSize));
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    return f;
}

QFont valueFont(int pointSize)
{
    QFont f(QStringList{"Menlo", "Monaco", "Courier New"});
    f.setPointSize(qMax(6, pointSize));
    return f;
}

void drawGlowPath(QPainter &p, const QPainterPath &path,
                  const QColor &color, qreal width)
{
    p.save();
    p.setBrush(Qt::NoBrush);
    for (int i = 3; i >= 1; --i) {
        QColor halo = color;
        halo.setAlphaF(0.09 * i);
        p.setPen(QPen(halo, width + i * 3.0, Qt::SolidLine, Qt::RoundCap));
        p.drawPath(path);
    }
    p.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(path);
    p.restore();
}

} // namespace nasagui::Theme
