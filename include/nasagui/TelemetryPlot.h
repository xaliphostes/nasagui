#pragma once

#include <QVector>
#include <QWidget>

namespace nasagui {

// Scrolling time-series plot with grid, glow stroke and gradient fill.
class TelemetryPlot : public QWidget
{
    Q_OBJECT
public:
    explicit TelemetryPlot(QWidget *parent = nullptr);

    void setCapacity(int samples);
    void setYRange(double min, double max);  // disables auto-scaling
    void setAutoRange(bool on);
    void setUnits(const QString &units);

    QSize sizeHint() const override { return {400, 180}; }

public slots:
    void append(double sample);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> m_data;
    int m_capacity = 300;
    bool m_autoRange = true;
    double m_yMin = 0.0;
    double m_yMax = 1.0;
    QString m_units;
};

} // namespace nasagui
