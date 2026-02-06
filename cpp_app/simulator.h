#pragma once

#include <QThread>
#include <QMutex>

#include "can_protocol.h"

class Simulator : public QThread {
    Q_OBJECT
public:
    explicit Simulator(QObject* parent = nullptr);
    void requestStop();

signals:
    void message2Received(Message2 msg);
    void logMessage(const QString& msg);

protected:
    void run() override;

private:
    QMutex m_mutex;
    bool m_running = false;
};
