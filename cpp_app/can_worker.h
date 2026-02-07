#pragma once

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <deque>
#include <memory>

#include "can_protocol.h"
#include "can_iface.h"

// Baudrate-switch CAN sequence constants
constexpr uint32_t BAUD_SWITCH_ID = 0x01002100;
constexpr int BAUD_FRAME2_COUNT = 7;
constexpr double BAUD_FRAME2_INTERVAL_S = 0.5;
constexpr int SAFE_STOP_CYCLES = 5;

// Status bit names
inline const char* statusBitName(int bit) {
    switch (bit) {
        case 0: return "HW_FAIL";
        case 1: return "OVER_TEMP";
        case 2: return "INPUT_V_ERR";
        case 3: return "STARTING";
        case 4: return "COMM_TIMEOUT";
        default: return "UNKNOWN";
    }
}

class BaudrateSwitchWorker : public QThread {
    Q_OBJECT
public:
    explicit BaudrateSwitchWorker(ICanIface* iface, QObject* parent = nullptr);
    void requestStop();

signals:
    void progress(int currentStep, int total);
    void finishedOk();
    void error(const QString& msg);
    void logMessage(const QString& msg);

protected:
    void run() override;

private:
    ICanIface* m_iface = nullptr;
    bool m_running = true;
};

class CANWorker : public QThread {
    Q_OBJECT
public:
    /// Construct with an already-opened CAN interface (takes ownership).
    explicit CANWorker(std::unique_ptr<ICanIface> iface, QObject* parent = nullptr);

    // Thread-safe setters (called from UI thread)
    void setSetpoints(double voltage, double current);
    void setControl(ChargerControl ctrl);
    void setRampConfig(bool enabled, double rateV, double rateA);
    void resetRamp();
    void enableTx(bool enabled);
    void requestStop();

    /// Access the underlying CAN interface (for BaudrateSwitchWorker).
    ICanIface* getInterface() const;
    bool isBusConnected() const;

signals:
    void connected();
    void disconnected();
    void error(const QString& msg);
    void logMessage(const QString& msg);
    void message2Received(Message2 msg);
    void timeoutAlarm();
    void txMessage(Message1 msg);
    void rampState(bool active, double rampedV, double rampedA);
    void healthStats(double txRate, double rxRate, double lastRxAge);
    void statusBitChanged(int bitIdx, const QString& name, bool isFault);

protected:
    void run() override;

private:
    void safeStop();
    double calcRate(const std::deque<double>& times, double now, double window = 2.0) const;

    std::unique_ptr<ICanIface> m_iface;
    mutable QMutex m_mutex;
    bool m_running = false;

    // TX target state
    double m_targetVoltage = 0.0;
    double m_targetCurrent = 0.0;
    ChargerControl m_control = ChargerControl::STOP_OUTPUTTING;
    bool m_txEnabled = false;

    // Ramp config
    bool m_rampEnabled = false;
    double m_rampRateV = 5.0;
    double m_rampRateA = 0.5;
    bool m_rampResetFlag = false;

    // Health tracking
    std::deque<double> m_txTimes;
    std::deque<double> m_rxTimes;
    int m_prevStatusByte = -1;
};
