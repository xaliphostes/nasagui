#pragma once

#include <QWidget>

namespace nasagui {

// Vertical segmented level bar (fuel / O2 / reserves style readout).
class BarGauge : public QWidget
{
    Q_OBJECT
public:
    explicit BarGauge(QWidget *parent = nullptr);

    void setRange(double min, double max);
    void setLabel(const QString &label);
    void setSuffix(const QString &suffix);   // appended to the numeric readout
    void setSegmentCount(int count);

    double value() const { return m_value; }

    QSize sizeHint() const override { return {64, 180}; }
    QSize minimumSizeHint() const override { return {48, 120}; }

public slots:
    void setValue(double value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_min = 0.0;
    double m_max = 100.0;
    double m_value = 0.0;
    int m_segments = 22;
    QString m_label;
    QString m_suffix;
};

} // namespace nasagui
