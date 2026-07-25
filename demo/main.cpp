#include <nasagui/BarGauge.h>
#include <nasagui/CircularGauge.h>
#include <nasagui/CollapsibleDock.h>
#include <nasagui/Controls.h>
#include <nasagui/HudButton.h>
#include <nasagui/HudPanel.h>
#include <nasagui/RadarScope.h>
#include <nasagui/StatusIndicator.h>
#include <nasagui/Style.h>
#include <nasagui/TelemetryPlot.h>
#include <nasagui/Theme.h>

#include <QActionGroup>
#include <QApplication>
#include <QDateTime>
#include <QMenu>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPlainTextEdit>
#include <QRandomGenerator>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

using namespace nasagui;

class MissionWindow : public QWidget
{
public:
    MissionWindow()
    {
        setWindowTitle("NASA-GUI // Mission Operations");
        resize(1380, 860);
        buildUi();

        connect(&m_dataTimer, &QTimer::timeout, this, [this] { tick(); });
        m_dataTimer.start(100);
        connect(&m_clockTimer, &QTimer::timeout, this, [this] { updateClock(); });
        m_clockTimer.start(1000);
        connect(&m_logTimer, &QTimer::timeout, this, [this] { appendLog(); });
        m_logTimer.start(2200);
        updateClock();
        appendLog();
    }

    CollapsibleDock *dock(CollapsibleDock::Edge edge) const
    {
        switch (edge) {
        case CollapsibleDock::Edge::Left:   return m_leftDock;
        case CollapsibleDock::Edge::Right:  return m_rightDock;
        case CollapsibleDock::Edge::Top:    return m_topDock;
        case CollapsibleDock::Edge::Bottom: return m_bottomDock;
        }
        return nullptr;
    }

protected:
    // Faint dot grid over the whole background
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.fillRect(rect(), Theme::Background);
        QColor dot = Theme::GridLine;
        dot.setAlpha(90);
        p.setPen(QPen(dot, 1.0));
        for (int x = 12; x < width(); x += 26)
            for (int y = 12; y < height(); y += 26)
                p.drawPoint(x, y);
    }

private:
    void buildUi()
    {
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(18, 14, 18, 18);
        root->setSpacing(10);

        // ---- Header bar -----------------------------------------------------
        auto *header = new QHBoxLayout;
        auto *title = new QLabel("MISSION OPERATIONS // ARES-V CONSOLE");
        title->setFont(Theme::titleFont(15));
        QPalette tp = title->palette();
        tp.setColor(QPalette::WindowText, Theme::Primary);
        title->setPalette(tp);
        header->addWidget(title);
        header->addSpacing(24);
        header->addWidget(buildOpsMenuButton());
        header->addStretch();

        m_metLabel = new QLabel;
        m_metLabel->setFont(Theme::valueFont(13));
        header->addWidget(m_metLabel);
        header->addSpacing(24);

        m_utcLabel = new QLabel;
        m_utcLabel->setFont(Theme::valueFont(13));
        QPalette up = m_utcLabel->palette();
        up.setColor(QPalette::WindowText, Theme::TextDim);
        m_utcLabel->setPalette(up);
        header->addWidget(m_utcLabel);
        root->addLayout(header);

        // ---- Top dock: mission timeline -------------------------------------
        m_topDock = new CollapsibleDock(CollapsibleDock::Edge::Top, "Timeline");
        m_topDock->setExpandedSize(104);
        m_topDock->setContent(buildTimelinePanel());
        root->addWidget(m_topDock);

        // ---- Middle row: left dock | center | right dock --------------------
        auto *middle = new QHBoxLayout;
        middle->setSpacing(10);
        root->addLayout(middle, 1);

        m_leftDock = new CollapsibleDock(CollapsibleDock::Edge::Left, "Propulsion");
        m_leftDock->setExpandedSize(420);
        m_leftDock->setContent(buildPropulsionPanel());
        m_leftDock->setContent(buildReservesPanel());
        middle->addWidget(m_leftDock);

        middle->addWidget(buildCenterColumn(), 1);

        m_rightDock = new CollapsibleDock(CollapsibleDock::Edge::Right, "Scan");
        m_rightDock->setExpandedSize(300);
        m_rightDock->setContent(buildRadarPanel());
        m_rightDock->setContent(buildCabinPanel());
        middle->addWidget(m_rightDock);

        // ---- Bottom dock: event log -----------------------------------------
        m_bottomDock = new CollapsibleDock(CollapsibleDock::Edge::Bottom, "Event Log / Config");
        m_bottomDock->setExpandedSize(200);
        auto *bottom = new QWidget;
        auto *bottomLay = new QHBoxLayout(bottom);
        bottomLay->setContentsMargins(0, 0, 0, 0);
        bottomLay->setSpacing(10);
        bottomLay->addWidget(buildLogPanel(), 1);
        bottomLay->addWidget(buildConfigPanel());
        m_bottomDock->setContent(bottom);
        root->addWidget(m_bottomDock);
    }

    QWidget *buildOpsMenuButton()
    {
        auto *ops = new HudButton("Ops");
        auto *menu = new QMenu(ops);
        menu->addAction("Snapshot Telemetry");
        menu->addAction("Export Event Log");
        menu->addSeparator();
        auto *night = menu->addAction("Night Mode");
        night->setCheckable(true);
        night->setChecked(true);
        auto *rate = menu->addMenu("Uplink Rate");
        auto *group = new QActionGroup(menu);
        for (const char *r : {"32 kbps", "128 kbps", "1 Mbps"}) {
            auto *act = rate->addAction(r);
            act->setCheckable(true);
            group->addAction(act);
        }
        group->actions().first()->setChecked(true);
        menu->addSeparator();
        connect(menu->addAction("Shutdown Console"), &QAction::triggered,
                this, &QWidget::close);
        connect(ops, &QPushButton::clicked, ops, [ops, menu] {
            menu->popup(ops->mapToGlobal(QPoint(0, ops->height() + 4)));
        });
        return ops;
    }

    QWidget *buildConfigPanel()
    {
        auto *panel = new HudPanel("Flight Config");
        auto *lay = new QHBoxLayout(panel);
        lay->setSpacing(28);

        // Text/choice fields
        auto *fields = new QGridLayout;
        fields->setVerticalSpacing(4);
        fields->addWidget(new HudLabel("Callsign"), 0, 0);
        auto *callsign = new HudLineEdit("ARES-V");
        callsign->setFixedWidth(130);
        fields->addWidget(callsign, 1, 0);
        fields->addWidget(new HudLabel("Comms Band"), 2, 0);
        auto *band = new HudComboBox;
        band->addItems({"X-Band", "S-Band", "Ka-Band", "UHF"});
        fields->addWidget(band, 3, 0);
        fields->setRowStretch(4, 1);
        lay->addLayout(fields);

        // Numeric fields
        auto *numeric = new QGridLayout;
        numeric->setVerticalSpacing(4);
        numeric->addWidget(new HudLabel("Burn Duration [s]"), 0, 0);
        auto *burn = new HudSpinBox;
        burn->setRange(0, 600);
        burn->setSingleStep(5);
        burn->setValue(120);
        numeric->addWidget(burn, 1, 0);
        numeric->addWidget(new HudLabel("Throttle Trim"), 2, 0);
        auto *trim = new HudSlider;
        trim->setRange(0, 100);
        trim->setValue(62);
        numeric->addWidget(trim, 3, 0);
        numeric->setRowStretch(4, 1);
        lay->addLayout(numeric);

        // Toggles
        auto *toggles = new QVBoxLayout;
        toggles->setSpacing(2);
        auto *align = new HudCheckBox("Auto-Align");
        align->setChecked(true);
        toggles->addWidget(align);
        toggles->addWidget(new HudCheckBox("Downlink"));
        toggles->addSpacing(6);
        auto *gAuto = new HudRadioButton("GNC Auto");
        gAuto->setChecked(true);
        toggles->addWidget(gAuto);
        toggles->addWidget(new HudRadioButton("GNC Manual"));
        toggles->addStretch();
        lay->addLayout(toggles);

        // Antenna azimuth dial
        auto *azimuth = new QVBoxLayout;
        azimuth->setSpacing(2);
        azimuth->addWidget(new HudLabel("Ant Azimuth"), 0, Qt::AlignHCenter);
        auto *dial = new HudDial;
        dial->setRange(0, 359);
        dial->setValue(220);
        azimuth->addWidget(dial, 0, Qt::AlignHCenter);
        auto *azValue = new HudLabel("220°", HudLabel::Role::Value);
        azimuth->addWidget(azValue, 0, Qt::AlignHCenter);
        connect(dial, &QDial::valueChanged, azValue, [azValue](int v) {
            azValue->setText(QString::number(v) + QChar(0x00B0));
        });
        lay->addLayout(azimuth);

        return panel;
    }

    QWidget *buildTimelinePanel()
    {
        auto *panel = new HudPanel("Mission Timeline");
        auto *lay = new QHBoxLayout(panel);
        const char *phases[] = {"Launch", "Ascent", "Orbit", "Trans-Mars", "EDL"};
        const int active = 2;
        lay->addStretch();
        for (int i = 0; i < 5; ++i) {
            if (i > 0) {
                auto *sep = new QLabel(QString::fromUtf8("▸"));
                QPalette sp = sep->palette();
                sp.setColor(QPalette::WindowText, Theme::TextDim);
                sep->setPalette(sp);
                lay->addWidget(sep);
                lay->addSpacing(18);
            }
            auto *phase = new QLabel(QString(phases[i]).toUpper());
            phase->setFont(Theme::titleFont(i == active ? 12 : 10));
            QPalette pp = phase->palette();
            pp.setColor(QPalette::WindowText,
                        i == active ? Theme::Primary
                                    : (i < active ? Theme::TextPrimary : Theme::TextDim));
            phase->setPalette(pp);
            lay->addWidget(phase);
            lay->addSpacing(18);
        }
        lay->addStretch();
        return panel;
    }

    QWidget *buildPropulsionPanel()
    {
        auto *panel = new HudPanel("Propulsion");
        auto *lay = new QHBoxLayout(panel);
        m_thrust = new CircularGauge;
        m_thrust->setRange(0, 100);
        m_thrust->setLabel("Thrust");
        m_thrust->setUnits("%");
        m_thrust->setWarnFrom(90);
        m_power = new CircularGauge;
        m_power->setRange(0, 40);
        m_power->setLabel("Power");
        m_power->setUnits("kW");
        m_power->setDecimals(1);
        m_power->setWarnFrom(34);
        lay->addWidget(m_thrust);
        lay->addWidget(m_power);
        return panel;
    }

    QWidget *buildReservesPanel()
    {
        auto *panel = new HudPanel("Reserves");
        auto *lay = new QHBoxLayout(panel);
        const char *names[] = {"O2", "H2O", "Fuel"};
        for (int i = 0; i < 3; ++i) {
            auto *bar = new BarGauge;
            bar->setRange(0, 100);
            bar->setLabel(names[i]);
            bar->setSuffix("%");
            lay->addWidget(bar);
            m_reserves[i] = bar;
        }
        return panel;
    }

    QWidget *buildCenterColumn()
    {
        auto *center = new QWidget;
        auto *lay = new QVBoxLayout(center);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(10);

        auto *telemetry = new HudPanel("Telemetry // Altitude");
        auto *telLay = new QVBoxLayout(telemetry);
        m_plot = new TelemetryPlot;
        m_plot->setUnits("KM");
        telLay->addWidget(m_plot);
        lay->addWidget(telemetry, 1);

        auto *systems = new HudPanel("Systems");
        auto *sysLay = new QVBoxLayout(systems);
        auto *sysGrid = new QGridLayout;
        sysGrid->setHorizontalSpacing(24);
        const struct { const char *name; StatusIndicator::Status st; } sys[] = {
            {"Guidance",     StatusIndicator::Status::Nominal},
            {"Life Support", StatusIndicator::Status::Nominal},
            {"Comms Array",  StatusIndicator::Status::Warning},
            {"Reactor",      StatusIndicator::Status::Nominal},
            {"Thermal",      StatusIndicator::Status::Alert},
            {"Nav Beacon",   StatusIndicator::Status::Off},
        };
        for (int i = 0; i < 6; ++i) {
            auto *ind = new StatusIndicator(sys[i].name);
            ind->setStatus(sys[i].st);
            sysGrid->addWidget(ind, i / 2, i % 2);
        }
        sysLay->addLayout(sysGrid);
        sysLay->addSpacing(8);

        auto *buttons = new QHBoxLayout;
        auto *arm = new HudButton("Arm");
        arm->setCheckable(true);
        auto *abort = new HudButton("Abort");
        abort->setAccent(Theme::Alert);
        buttons->addWidget(arm);
        buttons->addWidget(new HudButton("Stage"));
        buttons->addWidget(new HudButton("Comms"));
        buttons->addStretch();
        buttons->addWidget(abort);
        sysLay->addLayout(buttons);
        lay->addWidget(systems);
        return center;
    }

    QWidget *buildRadarPanel()
    {
        auto *panel = new HudPanel("Proximity Scan");
        auto *lay = new QVBoxLayout(panel);
        auto *scope = new RadarScope;
        auto *rng = QRandomGenerator::global();
        for (int i = 0; i < 5; ++i)
            scope->addContact(rng->bounded(360.0), 0.25 + rng->bounded(0.7), i == 0);
        lay->addWidget(scope);
        return panel;
    }

    QWidget *buildCabinPanel()
    {
        auto *panel = new HudPanel("Cabin");
        auto *lay = new QVBoxLayout(panel);
        m_pressure = new CircularGauge;
        m_pressure->setRange(0, 120);
        m_pressure->setLabel("Pressure");
        m_pressure->setUnits("kPa");
        m_pressure->setWarnFrom(110);
        lay->addWidget(m_pressure);
        return panel;
    }

    QWidget *buildLogPanel()
    {
        auto *panel = new HudPanel("Console // Event Log");
        auto *lay = new QVBoxLayout(panel);
        m_log = new QPlainTextEdit;
        m_log->setReadOnly(true);
        m_log->setFrameStyle(QFrame::NoFrame);
        m_log->setFont(Theme::valueFont(9));
        m_log->setStyleSheet(
            "QPlainTextEdit { background: transparent; border: none; color: #8fb3c4; }");
        lay->addWidget(m_log);
        return panel;
    }

    void tick()
    {
        m_t += 0.1;
        auto *rng = QRandomGenerator::global();
        auto noise = [rng](double amp) {
            return (rng->bounded(2.0) - 1.0) * amp;
        };

        m_thrust->setValue(78 + 14 * qSin(m_t * 0.6) + noise(1.5));
        m_power->setValue(24 + 6 * qSin(m_t * 0.4 + 1.3) + noise(0.4));
        m_pressure->setValue(101 + 2 * qSin(m_t * 0.2) + noise(0.3));
        m_reserves[0]->setValue(87 - m_t * 0.05);
        m_reserves[1]->setValue(64 - m_t * 0.03);
        m_reserves[2]->setValue(52 + 40 * qSin(m_t * 0.05));
        m_plot->append(412 + 38 * qSin(m_t * 0.3) + noise(4.0));
    }

    void updateClock()
    {
        const int h = m_elapsed / 3600, m = (m_elapsed / 60) % 60,
                  s = m_elapsed % 60;
        m_elapsed++;
        m_metLabel->setText(QString::asprintf("MET +%02d:%02d:%02d", h, m, s));
        m_utcLabel->setText(
            QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss 'UTC'"));
    }

    void appendLog()
    {
        static const char *messages[] = {
            "GNC: attitude hold nominal, drift 0.02 deg/s",
            "EPS: bus voltage 28.1 V, load 24.3 kW",
            "COMMS: DSN handover Canberra -> Goldstone",
            "THERMAL: radiator loop B temp above limit",
            "ECLSS: CO2 scrubber cycle complete",
            "NAV: state vector updated from star tracker",
        };
        const auto *msg = messages[m_logIndex++ % 6];
        m_log->appendPlainText(
            QDateTime::currentDateTimeUtc().toString("hh:mm:ss") + "  " + msg);
    }

    QTimer m_dataTimer, m_clockTimer, m_logTimer;
    double m_t = 0.0;
    int m_elapsed = 0;
    int m_logIndex = 0;

    CollapsibleDock *m_topDock = nullptr;
    CollapsibleDock *m_bottomDock = nullptr;
    CollapsibleDock *m_leftDock = nullptr;
    CollapsibleDock *m_rightDock = nullptr;

    CircularGauge *m_thrust = nullptr;
    CircularGauge *m_power = nullptr;
    CircularGauge *m_pressure = nullptr;
    BarGauge *m_reserves[3] = {};
    TelemetryPlot *m_plot = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QLabel *m_metLabel = nullptr;
    QLabel *m_utcLabel = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    applyTheme(app);

    MissionWindow window;
    window.show();

    // Headless verification: --snapshot <file.png> renders and exits;
    // add --collapse to snapshot with left/bottom docks collapsed.
    const QStringList args = app.arguments();
    const int idx = args.indexOf("--snapshot");
    if (idx >= 0 && idx + 1 < args.size()) {
        const QString path = args.at(idx + 1);
        if (args.contains("--collapse")) {
            QTimer::singleShot(800, &window, [&window] {
                window.dock(CollapsibleDock::Edge::Left)->setExpanded(false);
                window.dock(CollapsibleDock::Edge::Bottom)->setExpanded(false);
            });
        }
        QTimer::singleShot(2000, &app, [&window, path] {
            window.grab().save(path);
            QApplication::quit();
        });
    }

    return app.exec();
}
