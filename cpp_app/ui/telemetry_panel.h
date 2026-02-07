#pragma once

#include <QGroupBox>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>

#include "can_protocol.h"

class LedPill : public QFrame {
    Q_OBJECT
public:
    explicit LedPill(const QString& labelText, QWidget* parent = nullptr);
    void setOk(bool ok);

private:
    QLabel* m_led;
    QLabel* m_label;
};

class TelemetryPanel : public QGroupBox {
    Q_OBJECT
public:
    explicit TelemetryPanel(QWidget* parent = nullptr);

    void updateTelemetry(const Message2& msg);
    void updateSetpoints(double setV, double setA);
    void setAlarm(const QString& text);
    void clear();

private:
    void updateSpDisplay();

    QLabel* m_voutLabel;
    QLabel* m_ioutLabel;
    QLabel* m_vinLabel;
    QLabel* m_tempLabel;

    QLabel* m_spVLabel;
    QLabel* m_spALabel;

    LedPill* m_hwInd;
    LedPill* m_tempInd;
    LedPill* m_vinInd;
    LedPill* m_startInd;
    LedPill* m_commInd;

    QLabel* m_lastRxLabel;
    QLabel* m_alarmLabel;

    double m_setV = -1;
    double m_setA = -1;
    double m_actualV = -1;
    double m_actualA = -1;
};
