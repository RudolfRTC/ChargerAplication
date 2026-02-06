#include "ui/main_window.h"
#include "ui/theme.h"

#include <QCloseEvent>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString("OBC Charger Controller \u2014 %1").arg(Theme::COMPANY));
    resize(1280, 860);

    // Central widget
    auto* central = new QWidget;
    central->setObjectName("central");
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Header
    auto* header = new QFrame;
    header->setObjectName("header");
    header->setFixedHeight(64);
    auto* hLay = new QHBoxLayout(header);
    hLay->setContentsMargins(16, 4, 16, 4);

    auto* titleCol = new QVBoxLayout;
    titleCol->setSpacing(0);
    auto* titleLbl = new QLabel("OBC CHARGER CONTROLLER");
    titleLbl->setObjectName("header_title");
    auto* subtitleLbl = new QLabel(QString("%1  \u00b7  %2").arg(Theme::COMPANY, Theme::ADDRESS));
    subtitleLbl->setObjectName("header_subtitle");
    titleCol->addWidget(titleLbl);
    titleCol->addWidget(subtitleLbl);
    hLay->addLayout(titleCol, 1);

    auto* aboutLbl = new QLabel(QString("<a style='color:%1;' href='#'>About</a>").arg(Theme::CYAN));
    aboutLbl->setCursor(Qt::PointingHandCursor);
    connect(aboutLbl, &QLabel::linkActivated, this, &MainWindow::showAbout);
    hLay->addWidget(aboutLbl);
    root->addWidget(header);

    // 3-column body
    auto* bodySplitter = new QSplitter(Qt::Horizontal);

    // Left sidebar - Connection
    auto* leftWidget = new QWidget;
    leftWidget->setObjectName("left_sidebar");
    leftWidget->setMinimumWidth(240);
    leftWidget->setMaximumWidth(340);
    auto* leftInner = new QVBoxLayout(leftWidget);
    leftInner->setContentsMargins(6, 6, 6, 6);
    m_connPanel = new ConnectionPanel;
    leftInner->addWidget(m_connPanel);
    leftInner->addStretch();

    auto* leftScroll = new QScrollArea;
    leftScroll->setWidgetResizable(true);
    leftScroll->setWidget(leftWidget);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bodySplitter->addWidget(leftScroll);

    // Center - Tabs + Telemetry
    auto* centerWidget = new QWidget;
    auto* centerLay = new QVBoxLayout(centerWidget);
    centerLay->setContentsMargins(4, 4, 4, 4);

    auto* tabs = new QTabWidget;
    m_graphPanel = new GraphPanel;
    m_logPanel = new LogPanel;
    tabs->addTab(m_graphPanel, "Graphs");
    tabs->addTab(m_logPanel, "Log");
    centerLay->addWidget(tabs, 3);

    m_telePanel = new TelemetryPanel;
    centerLay->addWidget(m_telePanel, 0);
    bodySplitter->addWidget(centerWidget);

    // Right sidebar - Control
    auto* rightWidget = new QWidget;
    rightWidget->setObjectName("right_sidebar");
    rightWidget->setMinimumWidth(260);
    rightWidget->setMaximumWidth(380);
    auto* rightInner = new QVBoxLayout(rightWidget);
    rightInner->setContentsMargins(6, 6, 6, 6);
    m_ctrlPanel = new ControlPanel;
    rightInner->addWidget(m_ctrlPanel);
    rightInner->addStretch();

    auto* rightScroll = new QScrollArea;
    rightScroll->setWidgetResizable(true);
    rightScroll->setWidget(rightWidget);
    rightScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bodySplitter->addWidget(rightScroll);

    bodySplitter->setStretchFactor(0, 2);
    bodySplitter->setStretchFactor(1, 5);
    bodySplitter->setStretchFactor(2, 2);
    root->addWidget(bodySplitter, 1);

    // Footer
    auto* footer = new QFrame;
    footer->setObjectName("footer");
    footer->setFixedHeight(28);
    auto* fLay = new QHBoxLayout(footer);
    fLay->setContentsMargins(12, 0, 12, 0);
    auto* fLeft = new QLabel(QString("%1  \u00b7  %2, %3")
                                 .arg(Theme::MADE_BY, Theme::COMPANY, Theme::ADDRESS));
    fLeft->setObjectName("footer_text");
    auto* fRight = new QLabel(QString("v%1").arg(Theme::VERSION));
    fRight->setObjectName("footer_text");
    fLay->addWidget(fLeft);
    fLay->addStretch();
    fLay->addWidget(fRight);
    root->addWidget(footer);

    // Wire signals
    connect(m_connPanel, &ConnectionPanel::connectRequested, this, &MainWindow::onConnect);
    connect(m_connPanel, &ConnectionPanel::disconnectRequested, this, &MainWindow::onDisconnect);
    connect(m_connPanel, &ConnectionPanel::baudrateSwitchRequested, this, &MainWindow::onBaudrateSwitch);
    connect(m_ctrlPanel, &ControlPanel::controlChanged, this, &MainWindow::onControlChanged);
    connect(m_ctrlPanel, &ControlPanel::instant360vRequested, this, &MainWindow::onInstant360v);
    connect(m_ctrlPanel, &ControlPanel::profileLoaded, this, &MainWindow::onProfileLoaded);
    connect(m_ctrlPanel, &ControlPanel::logMessage, m_logPanel, &LogPanel::append);
}

// --- About ---

void MainWindow::showAbout() {
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("About OBC Charger Controller");
    dlg->setFixedSize(420, 280);
    auto* lay = new QVBoxLayout(dlg);
    lay->setAlignment(Qt::AlignCenter);
    lay->setSpacing(12);

    auto* title = new QLabel("OBC Charger Controller");
    title->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;").arg(Theme::CYAN));
    title->setAlignment(Qt::AlignCenter);
    lay->addWidget(title);

    auto* ver = new QLabel(QString("Version %1 (C++)").arg(Theme::VERSION));
    ver->setAlignment(Qt::AlignCenter);
    lay->addWidget(ver);

    auto* company = new QLabel(QString("%1\n%2").arg(Theme::COMPANY, Theme::ADDRESS));
    company->setAlignment(Qt::AlignCenter);
    company->setStyleSheet(QString("color: %1;").arg(Theme::TEXT_DIM));
    lay->addWidget(company);

    auto* made = new QLabel(Theme::MADE_BY);
    made->setAlignment(Qt::AlignCenter);
    made->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::CYAN));
    lay->addWidget(made);

    lay->addStretch();
    dlg->exec();
    delete dlg;
}

// --- Connection ---

void MainWindow::onConnect(const QString& interface, const QString& channel,
                           int bitrate, bool simulate)
{
    m_simMode = simulate;

    if (simulate) {
        m_logPanel->append(QString::fromUtf8("Starting simulation mode \u2026"));
        m_simulator = new Simulator;
        connect(m_simulator, &Simulator::message2Received, this, &MainWindow::onMessage2);
        connect(m_simulator, &Simulator::logMessage, m_logPanel, &LogPanel::append);
        m_simulator->start();
        m_connPanel->setConnected(true);
        m_ctrlPanel->setEnabled(true);
        m_telePanel->updateSetpoints(m_ctrlPanel->getVoltage(), m_ctrlPanel->getCurrent());
        return;
    }

    m_worker = new CANWorker;
    m_worker->setConnectionParams(interface, channel, bitrate);
    m_worker->setSetpoints(m_ctrlPanel->getVoltage(), m_ctrlPanel->getCurrent());
    m_worker->setControl(m_ctrlPanel->getControl());
    auto [rampV, rampA] = m_ctrlPanel->getRampRates();
    m_worker->setRampConfig(m_ctrlPanel->getRampEnabled(), rampV, rampA);
    m_worker->enableTx(true);

    connect(m_worker, &CANWorker::connected, this, &MainWindow::onWorkerConnected);
    connect(m_worker, &CANWorker::disconnected, this, &MainWindow::onWorkerDisconnected);
    connect(m_worker, &CANWorker::error, this, &MainWindow::onWorkerError);
    connect(m_worker, &CANWorker::logMessage, m_logPanel, &LogPanel::append);
    connect(m_worker, &CANWorker::message2Received, this, &MainWindow::onMessage2);
    connect(m_worker, &CANWorker::timeoutAlarm, this, &MainWindow::onTimeoutAlarm);
    connect(m_worker, &CANWorker::txMessage, this, &MainWindow::onTxMessage);
    connect(m_worker, &CANWorker::rampState, this, &MainWindow::onRampState);
    connect(m_worker, &CANWorker::healthStats, this, &MainWindow::onHealthStats);
    connect(m_worker, &CANWorker::statusBitChanged, this, &MainWindow::onStatusBitChanged);

    m_worker->start();
}

void MainWindow::onDisconnect() {
    if (m_simMode && m_simulator) {
        m_simulator->requestStop();
        m_simulator->wait(3000);
        delete m_simulator;
        m_simulator = nullptr;
        m_connPanel->setConnected(false);
        m_ctrlPanel->setEnabled(false);
        m_telePanel->clear();
        m_logPanel->append("Simulation stopped.");
        return;
    }

    if (m_worker) {
        m_logPanel->append(QString::fromUtf8("Disconnecting \u2026"));
        m_worker->requestStop();
        m_worker->wait(10000);
        delete m_worker;
        m_worker = nullptr;
    }
}

// --- Worker signal handlers ---

void MainWindow::onWorkerConnected() {
    m_connPanel->setConnected(true);
    m_ctrlPanel->setEnabled(true);
}

void MainWindow::onWorkerDisconnected() {
    m_connPanel->setConnected(false);
    m_ctrlPanel->setEnabled(false);
    m_telePanel->clear();
    m_ctrlPanel->updateRampDisplay(false, 0, 0);
}

void MainWindow::onWorkerError(const QString& msg) {
    m_logPanel->append("ERROR: " + msg);
}

void MainWindow::onMessage2(Message2 msg) {
    m_telePanel->updateTelemetry(msg);
    m_graphPanel->addPoint(msg);
}

void MainWindow::onTimeoutAlarm() {
    m_telePanel->setAlarm("ALARM: No Message2 > 5 s!");
}

void MainWindow::onTxMessage(Message1 msg) {
    m_logPanel->append(QString("TX Message1: V=%1V I=%2A ctrl=%3")
                           .arg(msg.voltage_setpoint, 0, 'f', 1)
                           .arg(msg.current_setpoint, 0, 'f', 1)
                           .arg(QString::fromStdString(chargerControlName(msg.control))));
    m_telePanel->updateSetpoints(msg.voltage_setpoint, msg.current_setpoint);
}

void MainWindow::onRampState(bool active, double rampedV, double rampedA) {
    m_ctrlPanel->updateRampDisplay(active, rampedV, rampedA);
}

void MainWindow::onHealthStats(double txRate, double rxRate, double lastRxAge) {
    m_connPanel->updateHealth(txRate, rxRate, lastRxAge);
}

void MainWindow::onStatusBitChanged(int bit, const QString& name, bool isFault) {
    QString state = isFault ? "ON" : "OFF";
    QString severity = isFault ? "error" : "info";
    m_graphPanel->addEventMarker(QString("FAULT %1 %2").arg(name, state), severity);
    m_logPanel->append(QString("Status bit %1 (%2): %3").arg(bit).arg(name, state));
}

// --- Control panel -> worker ---

void MainWindow::onControlChanged(double voltage, double current, int control,
                                  bool rampEnabled, double rampV, double rampA)
{
    auto ctrl = static_cast<ChargerControl>(control);

    if (ctrl != m_prevControl) {
        QMap<ChargerControl, QString> labelMap = {
            {ChargerControl::STOP_OUTPUTTING,   "STOP"},
            {ChargerControl::START_CHARGING,    "START CHARGING"},
            {ChargerControl::HEATING_DC_SUPPLY, "HEATING/DC"},
        };
        QString label = labelMap.value(ctrl, "?");
        m_graphPanel->addEventMarker("Mode: " + label, "info");
        m_prevControl = ctrl;
    }

    if (m_simMode)
        m_telePanel->updateSetpoints(voltage, current);

    if (m_worker) {
        m_worker->setSetpoints(voltage, current);
        m_worker->setControl(ctrl);
        m_worker->setRampConfig(rampEnabled, rampV, rampA);
    }
}

void MainWindow::onProfileLoaded() {
    if (m_worker)
        m_worker->resetRamp();
}

// --- Instant 360V ---

void MainWindow::onInstant360v() {
    m_graphPanel->addEventMarker(QString::fromUtf8("\u26a1 360V/9A INSTANT"), "warning");
    if (m_worker) {
        m_worker->setSetpoints(360.0, 9.0);
        m_worker->setControl(ChargerControl::HEATING_DC_SUPPLY);
        m_worker->setRampConfig(false, 5.0, 0.5);
        m_worker->resetRamp();
    }
}

// --- Baudrate switch ---

void MainWindow::onBaudrateSwitch() {
    if (!m_worker || !m_worker->isBusConnected()) {
        m_logPanel->append("Cannot switch baudrate: CAN not connected.");
        return;
    }
    if (m_baudWorker && m_baudWorker->isRunning()) {
        m_logPanel->append("Baudrate switch already in progress.");
        return;
    }

    m_connPanel->setBaudSwitchBusy(true);
    m_ctrlPanel->setEnabled(false);
    m_graphPanel->addEventMarker("Baud switch START", "info");

    m_baudWorker = new BaudrateSwitchWorker(m_worker->getSocketFd());
    connect(m_baudWorker, &BaudrateSwitchWorker::progress, this, &MainWindow::onBaudProgress);
    connect(m_baudWorker, &BaudrateSwitchWorker::finishedOk, this, &MainWindow::onBaudDone);
    connect(m_baudWorker, &BaudrateSwitchWorker::error, this, &MainWindow::onBaudError);
    connect(m_baudWorker, &BaudrateSwitchWorker::logMessage, m_logPanel, &LogPanel::append);
    m_baudWorker->start();
}

void MainWindow::onBaudProgress(int step, int total) {
    m_connPanel->setBaudSwitchProgress(step, total);
}

void MainWindow::onBaudDone() {
    m_connPanel->setBaudSwitchDone();
    m_ctrlPanel->setEnabled(true);
    delete m_baudWorker;
    m_baudWorker = nullptr;
    m_graphPanel->addEventMarker("Baud switch DONE", "info");
}

void MainWindow::onBaudError(const QString& msg) {
    m_logPanel->append("ERROR: " + msg);
    m_connPanel->setBaudSwitchBusy(false);
    m_ctrlPanel->setEnabled(true);
    delete m_baudWorker;
    m_baudWorker = nullptr;
}

// --- Close ---

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_baudWorker && m_baudWorker->isRunning()) {
        m_baudWorker->requestStop();
        m_baudWorker->wait(5000);
        delete m_baudWorker;
        m_baudWorker = nullptr;
    }
    onDisconnect();
    QMainWindow::closeEvent(event);
}
