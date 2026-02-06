#include "can_worker.h"

#include <QThread>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <chrono>

#ifdef __linux__
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#endif

static double monoNow() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

static double moveTowards(double current, double target, double maxStep) {
    double diff = target - current;
    if (std::abs(diff) <= maxStep)
        return target;
    return current + (diff > 0 ? maxStep : -maxStep);
}

// === BaudrateSwitchWorker ===

static const uint8_t BAUD_FRAME1_DATA[] = {0x07, 0x01, 0x00, 0x00, 0x3D, 0x8A, 0x09, 0x00};
static const uint8_t BAUD_FRAME2_DATA[] = {0x07, 0x02, 0x0E, 0x00, 0x71, 0xB7, 0x0F, 0x00};

BaudrateSwitchWorker::BaudrateSwitchWorker(int socketFd, QObject* parent)
    : QThread(parent), m_socketFd(socketFd) {}

void BaudrateSwitchWorker::requestStop() {
    m_running = false;
}

void BaudrateSwitchWorker::run() {
#ifdef __linux__
    int total = 1 + BAUD_FRAME2_COUNT;

    // Step 1: send frame #1
    struct can_frame frame{};
    frame.can_id = BAUD_SWITCH_ID | CAN_EFF_FLAG;
    frame.can_dlc = 8;
    std::memcpy(frame.data, BAUD_FRAME1_DATA, 8);

    if (::write(m_socketFd, &frame, sizeof(frame)) < 0) {
        emit error(QString("Baudrate switch frame #1 TX error: %1").arg(strerror(errno)));
        return;
    }
    emit logMessage(QString("Baudrate switch: sent frame #1 (ID=0x%1)")
                        .arg(BAUD_SWITCH_ID, 8, 16, QChar('0')));
    emit progress(1, total);

    // Steps 2-8: send frame #2 seven times @ 500ms
    std::memcpy(frame.data, BAUD_FRAME2_DATA, 8);
    for (int i = 0; i < BAUD_FRAME2_COUNT; ++i) {
        if (!m_running) {
            emit logMessage("Baudrate switch aborted.");
            return;
        }
        QThread::msleep(static_cast<unsigned long>(BAUD_FRAME2_INTERVAL_S * 1000));
        if (::write(m_socketFd, &frame, sizeof(frame)) < 0) {
            emit error(QString("Baudrate switch frame #2 [%1] TX error: %2")
                           .arg(i + 1).arg(strerror(errno)));
            return;
        }
        emit logMessage(QString("Baudrate switch: sent frame #2 (%1/%2)")
                            .arg(i + 1).arg(BAUD_FRAME2_COUNT));
        emit progress(2 + i, total);
    }

    emit logMessage("Baudrate switch sequence completed.");
    emit finishedOk();
#else
    emit error("Baudrate switch not supported on this platform (Linux only).");
#endif
}

// === CANWorker ===

CANWorker::CANWorker(QObject* parent) : QThread(parent) {}

void CANWorker::setConnectionParams(const QString& interface, const QString& channel, int bitrate) {
    QMutexLocker lock(&m_mutex);
    m_interface = interface;
    m_channel = channel;
    m_bitrate = bitrate;
}

void CANWorker::setSetpoints(double voltage, double current) {
    QMutexLocker lock(&m_mutex);
    m_targetVoltage = voltage;
    m_targetCurrent = current;
}

void CANWorker::setControl(ChargerControl ctrl) {
    QMutexLocker lock(&m_mutex);
    m_control = ctrl;
}

void CANWorker::setRampConfig(bool enabled, double rateV, double rateA) {
    QMutexLocker lock(&m_mutex);
    m_rampEnabled = enabled;
    m_rampRateV = rateV;
    m_rampRateA = rateA;
}

void CANWorker::resetRamp() {
    QMutexLocker lock(&m_mutex);
    m_rampResetFlag = true;
}

void CANWorker::enableTx(bool enabled) {
    QMutexLocker lock(&m_mutex);
    m_txEnabled = enabled;
}

void CANWorker::requestStop() {
    QMutexLocker lock(&m_mutex);
    m_running = false;
}

int CANWorker::getSocketFd() const {
    return m_socketFd;
}

bool CANWorker::isBusConnected() const {
    return m_socketFd >= 0;
}

double CANWorker::calcRate(const std::deque<double>& times, double now, double window) const {
    double cutoff = now - window;
    int count = 0;
    for (double t : times) {
        if (t >= cutoff) ++count;
    }
    return static_cast<double>(count) / window;
}

bool CANWorker::sendCanFrame(uint32_t id, const uint8_t* data, uint8_t dlc) {
#ifdef __linux__
    struct can_frame frame{};
    frame.can_id = id | CAN_EFF_FLAG;  // Extended frame
    frame.can_dlc = dlc;
    std::memcpy(frame.data, data, dlc);
    ssize_t nbytes = ::write(m_socketFd, &frame, sizeof(frame));
    return nbytes == sizeof(frame);
#else
    (void)id; (void)data; (void)dlc;
    return false;
#endif
}

void CANWorker::run() {
#ifdef __linux__
    // Read connection params
    QString iface, chan;
    int brate;
    {
        QMutexLocker lock(&m_mutex);
        iface = m_interface;
        chan = m_channel;
        brate = m_bitrate;
    }

    // Open SocketCAN
    m_socketFd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_socketFd < 0) {
        QString msg = QString("CAN socket creation failed: %1").arg(strerror(errno));
        emit logMessage(msg);
        emit error(msg);
        emit disconnected();
        return;
    }

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, chan.toStdString().c_str(), IFNAMSIZ - 1);
    if (::ioctl(m_socketFd, SIOCGIFINDEX, &ifr) < 0) {
        QString msg = QString("CAN interface '%1' not found: %2").arg(chan, strerror(errno));
        emit logMessage(msg);
        emit error(msg);
        ::close(m_socketFd);
        m_socketFd = -1;
        emit disconnected();
        return;
    }

    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (::bind(m_socketFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        QString msg = QString("CAN bind failed: %1").arg(strerror(errno));
        emit logMessage(msg);
        emit error(msg);
        ::close(m_socketFd);
        m_socketFd = -1;
        emit disconnected();
        return;
    }

    // Set non-blocking for recv with poll
    int flags = ::fcntl(m_socketFd, F_GETFL, 0);
    ::fcntl(m_socketFd, F_SETFL, flags | O_NONBLOCK);

    emit logMessage(QString("CAN bus connected on %1.").arg(chan));
    emit connected();

    {
        QMutexLocker lock(&m_mutex);
        m_running = true;
    }

    double lastTxTime = 0.0;
    double lastRxTime = monoNow();
    bool alarmActive = false;

    m_txTimes.clear();
    m_rxTimes.clear();
    m_prevStatusByte = -1;

    double rampedV = 0.0;
    double rampedA = 0.0;
    ChargerControl prevControl = ChargerControl::STOP_OUTPUTTING;

    while (true) {
        double now = monoNow();

        // Read UI state
        bool txEn;
        ChargerControl ctrl;
        double tgtV, tgtA;
        bool rampEn;
        double rampRV, rampRA;
        bool doReset;
        {
            QMutexLocker lock(&m_mutex);
            if (!m_running) break;
            txEn = m_txEnabled;
            ctrl = m_control;
            tgtV = m_targetVoltage;
            tgtA = m_targetCurrent;
            rampEn = m_rampEnabled;
            rampRV = m_rampRateV;
            rampRA = m_rampRateA;
            doReset = m_rampResetFlag;
            m_rampResetFlag = false;
        }

        // Ramp reset
        if (doReset) {
            rampedV = 0.0;
            rampedA = 0.0;
        }
        if (ctrl != ChargerControl::STOP_OUTPUTTING
            && prevControl == ChargerControl::STOP_OUTPUTTING) {
            rampedV = 0.0;
            rampedA = 0.0;
        }
        prevControl = ctrl;

        // TX
        if (txEn && (now - lastTxTime) >= (CYCLE_MS / 1000.0)) {
            double dt = (lastTxTime > 0) ? (now - lastTxTime) : (CYCLE_MS / 1000.0);

            double sendV, sendA;
            bool rampActive;
            if (ctrl == ChargerControl::STOP_OUTPUTTING || !rampEn) {
                sendV = tgtV;
                sendA = tgtA;
                rampActive = false;
            } else {
                double stepV = rampRV * dt;
                double stepA = rampRA * dt;
                rampedV = moveTowards(rampedV, tgtV, stepV);
                rampedA = moveTowards(rampedA, tgtA, stepA);
                sendV = std::round(rampedV * 10.0) / 10.0;
                sendA = std::round(rampedA * 10.0) / 10.0;
                rampActive = (sendV != std::round(tgtV * 10.0) / 10.0
                           || sendA != std::round(tgtA * 10.0) / 10.0);
            }

            Message1 msg1;
            msg1.voltage_setpoint = sendV;
            msg1.current_setpoint = sendA;
            msg1.control = ctrl;

            auto payload = msg1.encode();
            if (sendCanFrame(MSG1_ID, payload.data(), 8)) {
                lastTxTime = now;
                m_txTimes.push_back(now);
                if (m_txTimes.size() > 20) m_txTimes.pop_front();
                emit txMessage(msg1);
                emit rampState(rampActive, sendV, sendA);
            } else {
                emit logMessage(QString("TX error: %1").arg(strerror(errno)));
            }

            double txRate = calcRate(m_txTimes, now);
            double rxRate = calcRate(m_rxTimes, now);
            double rxAge = now - lastRxTime;
            emit healthStats(txRate, rxRate, rxAge);
        }

        // RX (poll with 50ms timeout)
        struct pollfd pfd{};
        pfd.fd = m_socketFd;
        pfd.events = POLLIN;
        int ret = ::poll(&pfd, 1, 50);

        if (ret > 0 && (pfd.revents & POLLIN)) {
            struct can_frame rxFrame{};
            ssize_t nbytes = ::read(m_socketFd, &rxFrame, sizeof(rxFrame));
            if (nbytes == sizeof(rxFrame)) {
                uint32_t rxId = rxFrame.can_id & CAN_EFF_MASK;
                if (rxId == MSG2_ID) {
                    try {
                        Message2 msg2 = Message2::decode(rxFrame.data, rxFrame.can_dlc);
                        emit message2Received(msg2);
                        lastRxTime = now;
                        m_rxTimes.push_back(now);
                        if (m_rxTimes.size() > 20) m_rxTimes.pop_front();

                        if (alarmActive) {
                            alarmActive = false;
                            emit logMessage("Message2 received \u2014 timeout cleared.");
                        }

                        // Detect status bit changes
                        int newStatus = msg2.status.toByte();
                        if (m_prevStatusByte >= 0) {
                            int xorBits = newStatus ^ m_prevStatusByte;
                            for (int bit = 0; bit < 5; ++bit) {
                                if (xorBits & (1 << bit)) {
                                    bool isFault = (newStatus & (1 << bit)) != 0;
                                    emit statusBitChanged(bit, QString(statusBitName(bit)), isFault);
                                }
                            }
                        }
                        m_prevStatusByte = newStatus;
                    } catch (const std::exception& e) {
                        emit logMessage(QString("Message2 decode error: %1").arg(e.what()));
                    }
                }
            }
        }

        // Timeout check
        if (txEn && (now - lastRxTime) > TIMEOUT_S) {
            if (!alarmActive) {
                alarmActive = true;
                emit timeoutAlarm();
                emit logMessage("ALARM: No Message2 for > 5 s!");
            }
        }
    }

    safeStop();
    closeBus();
    emit disconnected();

#else
    emit error("CAN not supported on this platform (Linux SocketCAN only).");
    emit disconnected();
#endif
}

void CANWorker::safeStop() {
    if (m_socketFd < 0) return;
    emit logMessage("Safe-stop: sending Control=STOP \u2026");

    Message1 msg1;
    msg1.voltage_setpoint = 0;
    msg1.current_setpoint = 0;
    msg1.control = ChargerControl::STOP_OUTPUTTING;
    auto payload = msg1.encode();

    for (int i = 0; i < SAFE_STOP_CYCLES; ++i) {
        if (!sendCanFrame(MSG1_ID, payload.data(), 8))
            break;
        QThread::msleep(CYCLE_MS);
    }
    emit logMessage("Safe-stop complete.");
}

void CANWorker::closeBus() {
#ifdef __linux__
    if (m_socketFd >= 0) {
        ::close(m_socketFd);
        m_socketFd = -1;
        emit logMessage("CAN bus closed.");
    }
#endif
}
