#pragma once

#include <QMainWindow>

#include "can_protocol.h"
#include "can_worker.h"
#include "simulator.h"
#include "ui/connection_panel.h"
#include "ui/control_panel.h"
#include "ui/graph_panel.h"
#include "ui/log_panel.h"
#include "ui/telemetry_panel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onConnect(const QString& interface, const QString& channel, int bitrate, bool simulate);
    void onDisconnect();
    void onWorkerConnected();
    void onWorkerDisconnected();
    void onWorkerError(const QString& msg);
    void onMessage2(Message2 msg);
    void onTimeoutAlarm();
    void onTxMessage(Message1 msg);
    void onRampState(bool active, double rampedV, double rampedA);
    void onHealthStats(double txRate, double rxRate, double lastRxAge);
    void onStatusBitChanged(int bit, const QString& name, bool isFault);
    void onControlChanged(double voltage, double current, int control,
                          bool rampEnabled, double rampV, double rampA);
    void onProfileLoaded();
    void onInstant360v();
    void onBaudrateSwitch();
    void onBaudProgress(int step, int total);
    void onBaudDone();
    void onBaudError(const QString& msg);
    void showAbout();

private:
    CANWorker*              m_worker = nullptr;
    Simulator*              m_simulator = nullptr;
    BaudrateSwitchWorker*   m_baudWorker = nullptr;
    bool                    m_simMode = false;
    ChargerControl          m_prevControl = ChargerControl::STOP_OUTPUTTING;

    ConnectionPanel*        m_connPanel;
    ControlPanel*           m_ctrlPanel;
    GraphPanel*             m_graphPanel;
    LogPanel*               m_logPanel;
    TelemetryPanel*         m_telePanel;
};
