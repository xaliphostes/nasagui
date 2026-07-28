#include <nasagui/BarGauge.h>
#include <nasagui/CircularGauge.h>
#include <nasagui/CollapsibleDock.h>
#include <nasagui/Controls.h>
#include <nasagui/HudButton.h>
#include <nasagui/HudPanel.h>
#include <nasagui/LayoutStore.h>
#include <nasagui/ModelView.h>
#include <nasagui/RadarScope.h>
#include <nasagui/StatusIndicator.h>
#include <nasagui/Style.h>
#include <nasagui/TelemetryPlot.h>
#include <nasagui/Theme.h>

#include "BunnyMesh.h"

#include <QActionGroup>
#include <QApplication>
#include <QDateTime>
#include <QMenu>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPlainTextEdit>
#include <QCloseEvent>
#include <QRandomGenerator>
#include <QSettings>
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
        resize(1500, 880);
        buildUi();

        connect(&m_dataTimer, &QTimer::timeout, this, [this] { tick(); });
        m_dataTimer.start(100);
        connect(&m_clockTimer, &QTimer::timeout, this, [this] { updateClock(); });
        m_clockTimer.start(1000);
        connect(&m_logTimer, &QTimer::timeout, this, [this] { appendLog(); });
        m_logTimer.start(2200);
        updateClock();
        appendLog();

        // Plain QLabels/stylesheets cache their colors, so refresh them
        // whenever the user picks another style.
        connect(Theme::Notifier::instance(), &Theme::Notifier::styleChanged,
                this, [this] { applyThemeColors(); });
        applyThemeColors();
    }

    const QVector<HudPanel *> &panels() const { return m_hudPanels; }

    void saveLayoutNow()
    {
        QSettings settings;
        saveLayout(settings, this);
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
    void closeEvent(QCloseEvent *event) override
    {
        saveLayoutNow();
        QWidget::closeEvent(event);
    }

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
    HudPanel *registerPanel(HudPanel *panel)
    {
        panel->setClosable(true);
        m_hudPanels.append(panel);
        return panel;
    }

    void populatePanelsMenu()
    {
        for (HudPanel *panel : m_hudPanels) {
            QAction *act = m_panelsMenu->addAction(panel->title());
            act->setCheckable(true);
            act->setChecked(true);
            connect(act, &QAction::toggled, panel, [panel](bool on) {
                if (on)
                    panel->openPanel();
                else
                    panel->closePanel();
            });
            connect(panel, &HudPanel::panelClosed, act, [act] {
                QSignalBlocker blocker(act);
                act->setChecked(false);
            });
            connect(panel, &HudPanel::panelOpened, act, [act] {
                QSignalBlocker blocker(act);
                act->setChecked(true);
            });
        }
    }

    void buildUi()
    {
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(18, 14, 18, 18);
        root->setSpacing(10);

        // ---- Header bar -----------------------------------------------------
        auto *header = new QHBoxLayout;
        m_title = new QLabel("MISSION OPERATIONS // ARES-V CONSOLE");
        m_title->setFont(Theme::titleFont(15));
        header->addWidget(m_title);
        header->addSpacing(24);
        header->addWidget(buildOpsMenuButton());
        header->addStretch();

        m_metLabel = new QLabel;
        m_metLabel->setFont(Theme::valueFont(13));
        header->addWidget(m_metLabel);
        header->addSpacing(24);

        m_utcLabel = new QLabel;
        m_utcLabel->setFont(Theme::valueFont(13));
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
        m_bottomDock->setContent(buildLogPanel());
        m_bottomDock->setContent(buildConfigPanel());
        root->addWidget(m_bottomDock);

        populatePanelsMenu();
    }

    QWidget *buildOpsMenuButton()
    {
        auto *ops = new HudButton("Ops");
        auto *menu = new QMenu(ops);
        menu->addAction("Snapshot Telemetry");
        menu->addAction("Export Event Log");
        menu->addSeparator();
        m_panelsMenu = new HudMenu("Panels", menu);   // stays open while toggling
        menu->addMenu(m_panelsMenu);
        menu->addSeparator();
        menu->addMenu(buildStyleMenu(menu));
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

    // Radio-style list of every style the theme offers; the choice is
    // remembered across runs.
    QMenu *buildStyleMenu(QWidget *parent)
    {
        auto *styleMenu = new QMenu("Style", parent);
        auto *group = new QActionGroup(styleMenu);
        for (Theme::Style style : Theme::styles()) {
            QAction *act = styleMenu->addAction(Theme::styleName(style));
            act->setCheckable(true);
            act->setChecked(style == Theme::style());
            group->addAction(act);
            connect(act, &QAction::triggered, this, [style] {
                setApplicationStyle(style);
                QSettings().setValue("style", Theme::styleName(style));
            });
        }
        return styleMenu;
    }

    // Re-apply every theme color this window caches outside of a paintEvent().
    void applyThemeColors()
    {
        const auto setTextColor = [](QWidget *w, const QColor &color) {
            QPalette pal = w->palette();
            pal.setColor(QPalette::WindowText, color);
            w->setPalette(pal);
        };

        setTextColor(m_title, Theme::Primary);
        setTextColor(m_metLabel, Theme::TextPrimary);
        setTextColor(m_utcLabel, Theme::TextDim);

        for (QLabel *sep : m_timelineSeparators)
            setTextColor(sep, Theme::TextDim);
        for (int i = 0; i < m_timelinePhases.size(); ++i)
            setTextColor(m_timelinePhases[i],
                         i == kActivePhase
                             ? Theme::Primary
                             : (i < kActivePhase ? Theme::TextPrimary
                                                 : Theme::TextDim));

        // Log text sits one notch above the dim text, whichever way "up" is.
        const QColor logColor = Theme::style() == Theme::Style::Daylight
            ? Theme::TextDim.darker(125)
            : Theme::TextDim.lighter(125);
        m_log->setStyleSheet(
            QStringLiteral("QPlainTextEdit { background: transparent; "
                           "border: none; color: %1; }")
                .arg(logColor.name()));
        m_abort->setAccent(Theme::Alert);
        update();
    }

    QWidget *buildConfigPanel()
    {
        auto *panel = registerPanel(new HudPanel("Flight Config"));
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
        auto *panel = registerPanel(new HudPanel("Mission Timeline"));
        auto *lay = new QHBoxLayout(panel);
        const char *phases[] = {"Launch", "Ascent", "Orbit", "Trans-Mars", "EDL"};
        lay->addStretch();
        for (int i = 0; i < 5; ++i) {
            if (i > 0) {
                auto *sep = new QLabel(QString::fromUtf8("▸"));
                m_timelineSeparators.append(sep);
                lay->addWidget(sep);
                lay->addSpacing(18);
            }
            auto *phase = new QLabel(QString(phases[i]).toUpper());
            phase->setFont(Theme::titleFont(i == kActivePhase ? 12 : 10));
            m_timelinePhases.append(phase);
            lay->addWidget(phase);
            lay->addSpacing(18);
        }
        lay->addStretch();
        return panel;   // colors come from applyThemeColors()
    }

    QWidget *buildPropulsionPanel()
    {
        auto *panel = registerPanel(new HudPanel("Propulsion"));
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
        auto *panel = registerPanel(new HudPanel("Reserves"));
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
        // Telemetry | Model are split horizontally; that row and Systems
        // vertically — all separations draggable.
        auto *topSplit = new HudSplitter(Qt::Horizontal);
        auto *centerSplit = new HudSplitter(Qt::Vertical);

        auto *telemetry = registerPanel(new HudPanel("Telemetry // Altitude"));
        auto *telLay = new QVBoxLayout(telemetry);
        m_plot = new TelemetryPlot;
        m_plot->setUnits("KM");
        telLay->addWidget(m_plot);
        topSplit->addWidget(telemetry);

        auto *model = registerPanel(new HudPanel("Model // Stanford Bunny"));
        auto *modelLay = new QVBoxLayout(model);
        auto *bunnyView = new ModelView;
        bunnyView->setMesh(bunny::positions, bunny::vertexCount,
                           bunny::indices, bunny::indexCount);

        // Per-vertex scalar = one coordinate component of the mesh
        auto componentScalars = [](int comp) {
            QVector<float> values;
            values.resize(int(bunny::vertexCount));
            for (int v = 0; v < int(bunny::vertexCount); ++v)
                values[v] = bunny::positions[3 * v + comp];
            return values;
        };
        bunnyView->setScalars(componentScalars(2), "Z");
        modelLay->addWidget(bunnyView, 1);

        auto *modelBar = new QHBoxLayout;
        modelBar->setSpacing(8);
        modelBar->addWidget(new HudLabel("Attribute"));
        auto *attr = new HudComboBox;
        attr->addItems({"X", "Y", "Z", "None"});
        attr->setCurrentIndex(2);
        attr->setFixedWidth(90);
        connect(attr, &QComboBox::currentIndexChanged, bunnyView,
                [bunnyView, componentScalars](int index) {
                    if (index >= 3)
                        bunnyView->clearScalars();
                    else
                        bunnyView->setScalars(componentScalars(index),
                                              QString(QChar('X' + index)));
                });
        modelBar->addWidget(attr);
        modelBar->addSpacing(12);
        modelBar->addWidget(new HudLabel("Color Table"));
        auto *cmap = new HudComboBox;
        cmap->addItems({"Viridis", "Cool-Warm", "Ice", "Thermal"});
        cmap->setFixedWidth(110);
        connect(cmap, &QComboBox::currentIndexChanged, bunnyView,
                [bunnyView](int index) {
                    bunnyView->setColorMap(
                        static_cast<ModelView::ColorMap>(index));
                });
        modelBar->addWidget(cmap);
        modelBar->addStretch();
        modelLay->addLayout(modelBar);
        topSplit->addWidget(model);
        centerSplit->addWidget(topSplit);

        auto *systems = registerPanel(new HudPanel("Systems"));
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
        m_abort = new HudButton("Abort");
        buttons->addWidget(arm);
        buttons->addWidget(new HudButton("Stage"));
        buttons->addWidget(new HudButton("Comms"));
        buttons->addStretch();
        buttons->addWidget(m_abort);
        sysLay->addLayout(buttons);
        centerSplit->addWidget(systems);
        centerSplit->setStretchFactor(0, 1);   // extra space goes to the top row
        centerSplit->setStretchFactor(1, 0);
        return centerSplit;
    }

    QWidget *buildRadarPanel()
    {
        auto *panel = registerPanel(new HudPanel("Proximity Scan"));
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
        auto *panel = registerPanel(new HudPanel("Cabin"));
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
        auto *panel = registerPanel(new HudPanel("Console // Event Log"));
        auto *lay = new QVBoxLayout(panel);
        m_log = new QPlainTextEdit;
        m_log->setReadOnly(true);
        m_log->setFrameStyle(QFrame::NoFrame);
        m_log->setFont(Theme::valueFont(9));
        lay->addWidget(m_log);   // colors come from applyThemeColors()
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

    static constexpr int kActivePhase = 2;   // "Orbit"

    QTimer m_dataTimer, m_clockTimer, m_logTimer;
    double m_t = 0.0;
    int m_elapsed = 0;
    int m_logIndex = 0;

    QVector<HudPanel *> m_hudPanels;
    QMenu *m_panelsMenu = nullptr;

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
    QLabel *m_title = nullptr;
    QLabel *m_metLabel = nullptr;
    QLabel *m_utcLabel = nullptr;
    QVector<QLabel *> m_timelineSeparators;
    QVector<QLabel *> m_timelinePhases;
    HudButton *m_abort = nullptr;
};

int main(int argc, char *argv[])
{
    ModelView::setDefaultSurfaceFormat();   // before QApplication
    QApplication app(argc, argv);
    QApplication::setOrganizationName("nasagui");
    QApplication::setApplicationName("mission-demo");

    // Style: --theme <name> wins over the one remembered from the last run.
    // (Qt itself swallows "--style", hence the different spelling.)
    {
        const QStringList args = app.arguments();
        const int styleIdx = args.indexOf("--theme");
        const QString name = styleIdx >= 0 && styleIdx + 1 < args.size()
            ? args.at(styleIdx + 1)
            : QSettings().value("style").toString();
        applyTheme(app, Theme::styleFromName(name));
    }

    MissionWindow window;
    {
        QSettings settings;
        if (app.arguments().contains("--reset-layout"))
            settings.remove("layout");
        else
            restoreLayout(settings, &window);
    }
    window.show();

    // Headless verification: --snapshot <file.png> renders and exits;
    // add --collapse to snapshot with left/bottom docks collapsed, or
    // --style-test to snapshot after a live switch to the Daylight style.
    const QStringList args = app.arguments();
    const int idx = args.indexOf("--snapshot");
    if (idx >= 0 && idx + 1 < args.size()) {
        const QString path = args.at(idx + 1);
        if (args.contains("--close-test")) {
            QTimer::singleShot(700, &window, [&window] {
                for (auto *panel : window.panels())
                    if (panel->title().startsWith("Telemetry")
                        || panel->title() == "Reserves"
                        || panel->title().startsWith("Console")
                        || panel->title() == "Flight Config")
                        panel->closePanel();
            });
        }
        if (args.contains("--collapse")) {
            QTimer::singleShot(800, &window, [&window] {
                window.dock(CollapsibleDock::Edge::Left)->setExpanded(false);
                window.dock(CollapsibleDock::Edge::Bottom)->setExpanded(false);
            });
        }
        if (args.contains("--style-test")) {
            QTimer::singleShot(1200, &app, [] {
                setApplicationStyle(Theme::Style::Daylight);
            });
        }
        QTimer::singleShot(2000, &app, [&window, path] {
            window.grab().save(path);
            window.saveLayoutNow();   // quit() skips closeEvent
            QApplication::quit();
        });
    }

    return app.exec();
}
