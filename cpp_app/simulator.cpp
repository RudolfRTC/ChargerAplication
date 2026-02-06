#include "simulator.h"

#include <QMutexLocker>
#include <cmath>
#include <random>
#include <chrono>

Simulator::Simulator(QObject* parent) : QThread(parent) {}

void Simulator::requestStop() {
    QMutexLocker lock(&m_mutex);
    m_running = false;
}

void Simulator::run() {
    {
        QMutexLocker lock(&m_mutex);
        m_running = true;
    }
    emit logMessage("Simulator started (500 ms cycle).");

    auto t0 = std::chrono::steady_clock::now();
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    while (true) {
        {
            QMutexLocker lock(&m_mutex);
            if (!m_running) break;
        }

        double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();

        double vout = 320.0 + 10.0 * std::sin(elapsed / 30.0);
        double iout = 50.0 + 5.0 * std::sin(elapsed / 20.0 + 1.0);
        double vin  = 220.0 + 3.0 * std::sin(elapsed / 60.0);
        double temp = 45.0 + 10.0 * std::sin(elapsed / 40.0 + 2.0);

        bool hwFail = dist(rng) < 0.005;

        StatusFlags status;
        status.hardware_failure = hwFail;

        Message2 msg2;
        msg2.output_voltage = std::round(vout * 10.0) / 10.0;
        msg2.output_current = std::round(iout * 10.0) / 10.0;
        msg2.status = status;
        msg2.input_voltage = std::round(vin * 10.0) / 10.0;
        msg2.temperature = std::round(temp * 10.0) / 10.0;

        emit message2Received(msg2);

        QThread::msleep(CYCLE_MS);
    }

    emit logMessage("Simulator stopped.");
}
