#pragma once

#include <QTimer>
#include <QWidget>

namespace nasagui {

// System name + glowing status light + status word. Alert state blinks.
class StatusIndicator : public QWidget
{
    Q_OBJECT
public:
    enum class Status { Off, Nominal, Warning, Alert };

    explicit StatusIndicator(const QString &name, QWidget *parent = nullptr);

    void setStatus(Status status);
    Status status() const { return m_status; }

    QSize sizeHint() const override { return {170, 24}; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_name;
    Status m_status = Status::Nominal;
    QTimer m_blinkTimer;
    bool m_blinkOn = true;
};

} // namespace nasagui
