#pragma once

#include <QWidget>
#include <limits>

namespace nasagui {

// 270-degree arc gauge with tick marks, glowing value arc and a central
// monospaced readout. Arc turns amber past the warn threshold.
class CircularGauge : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue)
public:
    explicit CircularGauge(QWidget *parent = nullptr);

    void setRange(double min, double max);
    void setLabel(const QString &label);
    void setUnits(const QString &units);
    void setDecimals(int decimals);
    void setWarnFrom(double value);   // absolute value; pass +inf to disable

    double value() const { return m_value; }

    QSize sizeHint() const override { return {180, 180}; }
    QSize minimumSizeHint() const override { return {120, 120}; }

public slots:
    void setValue(double value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_min = 0.0;
    double m_max = 100.0;
    double m_value = 0.0;
    double m_warnFrom = std::numeric_limits<double>::max();
    int m_decimals = 0;
    QString m_label;
    QString m_units;
};

} // namespace nasagui
