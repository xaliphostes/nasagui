#pragma once

#include <QTimer>
#include <QVector>
#include <QWidget>

namespace nasagui {

// Circular radar / proximity scope with a rotating sweep and fading contacts.
class RadarScope : public QWidget
{
    Q_OBJECT
public:
    explicit RadarScope(QWidget *parent = nullptr);

    // Contacts: angle in degrees (CCW from east), radius fraction [0..1].
    void addContact(double angleDeg, double radiusFrac, bool hostile = false);
    void clearContacts();

    QSize sizeHint() const override { return {220, 220}; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Contact {
        double angle;
        double radius;
        bool hostile;
    };
    QVector<Contact> m_contacts;
    QTimer m_timer;
    double m_sweep = 0.0;
};

} // namespace nasagui
