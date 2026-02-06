#include "ui/telemetry_panel.h"
#include "ui/theme.h"

#include <QDateTime>
#include <QGridLayout>
#include <QVBoxLayout>

// --- LedPill ---

LedPill::LedPill(const QString& labelText, QWidget* parent)
    : QFrame(parent)
{
    setObjectName("led_pill");
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(8, 3, 10, 3);
    lay->setSpacing(6);
    m_led = new QLabel(QString::fromUtf8("\u25cf"));
    m_label = new QLabel(labelText);
    m_label->setStyleSheet(QString("font-size: 11px; color: %1;").arg(Theme::TEXT_DIM));
    lay->addWidget(m_led);
    lay->addWidget(m_label);
    setOk(true);
}

void LedPill::setOk(bool ok) {
    if (ok) {
        m_led->setStyleSheet(QString("color: %1; font-size: 16px; background: transparent;").arg(Theme::GREEN));
        setStyleSheet("QFrame#led_pill { background: rgba(0,230,118,0.1); "
                      "border: 1px solid rgba(0,230,118,0.3); border-radius: 12px; }");
    } else {
        m_led->setStyleSheet(QString("color: %1; font-size: 16px; background: transparent;").arg(Theme::RED));
        setStyleSheet("QFrame#led_pill { background: rgba(255,23,68,0.15); "
                      "border: 1px solid rgba(255,23,68,0.4); border-radius: 12px; }");
    }
}

// --- TelemetryPanel ---

TelemetryPanel::TelemetryPanel(QWidget* parent)
    : QGroupBox("Telemetry", parent)
{
    auto* layout = new QVBoxLayout(this);

    // Value grid
    auto* grid = new QGridLayout;
    grid->setSpacing(6);

    m_voutLabel = new QLabel(QString::fromUtf8("\u2014"));
    m_ioutLabel = new QLabel(QString::fromUtf8("\u2014"));
    m_vinLabel  = new QLabel(QString::fromUtf8("\u2014"));
    m_tempLabel = new QLabel(QString::fromUtf8("\u2014"));

    for (auto* lbl : {m_voutLabel, m_ioutLabel, m_vinLabel, m_tempLabel})
        lbl->setObjectName("tele_value");

    QStringList labels = {"Output V:", "Output A:", "Input V:", "Temp:"};
    QList<QLabel*> values = {m_voutLabel, m_ioutLabel, m_vinLabel, m_tempLabel};

    for (int i = 0; i < labels.size(); ++i) {
        auto* nameLbl = new QLabel(labels[i]);
        nameLbl->setObjectName("tele_label");
        grid->addWidget(nameLbl, 0, i * 2);
        grid->addWidget(values[i], 0, i * 2 + 1);
    }
    layout->addLayout(grid);

    // SET vs ACTUAL comparison
    m_spVLabel = new QLabel;
    m_spALabel = new QLabel;
    auto* spRow = new QVBoxLayout;
    spRow->setSpacing(2);
    spRow->addWidget(m_spVLabel);
    spRow->addWidget(m_spALabel);
    layout->addLayout(spRow);
    updateSpDisplay();

    // LED pill indicators
    m_hwInd    = new LedPill("HW");
    m_tempInd  = new LedPill("Temp");
    m_vinInd   = new LedPill("Vin");
    m_startInd = new LedPill("Start");
    m_commInd  = new LedPill("Comm");

    auto* pillRow = new QHBoxLayout;
    pillRow->setSpacing(6);
    for (auto* ind : {m_hwInd, m_tempInd, m_vinInd, m_startInd, m_commInd})
        pillRow->addWidget(ind);
    pillRow->addStretch();
    layout->addLayout(pillRow);

    // Last RX + alarm
    auto* bottom = new QHBoxLayout;
    m_lastRxLabel = new QLabel(QString::fromUtf8("Last RX: \u2014"));
    m_lastRxLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TEXT_DIM));
    m_alarmLabel = new QLabel("");
    m_alarmLabel->setObjectName("alarm_label");
    bottom->addWidget(m_lastRxLabel);
    bottom->addStretch();
    bottom->addWidget(m_alarmLabel);
    layout->addLayout(bottom);
}

void TelemetryPanel::updateSpDisplay() {
    auto fmtVal = [](double v) -> QString {
        return v < 0 ? QString::fromUtf8("\u2014") : QString::number(v, 'f', 1);
    };

    QString sv = fmtVal(m_setV);
    QString sa = fmtVal(m_setA);
    QString av = fmtVal(m_actualV);
    QString aa = fmtVal(m_actualA);

    m_spVLabel->setText(
        QString("<span style=\"color:%1\">SET </span>"
                "<span style=\"color:%2;font-weight:bold\">%3 V</span>"
                "<span style=\"color:%1\">  |  ACTUAL </span>"
                "<span style=\"color:%4;font-weight:bold\">%5 V</span>")
            .arg(Theme::TEXT_DIM, Theme::MAGENTA, sv, Theme::CYAN, av));

    m_spALabel->setText(
        QString("<span style=\"color:%1\">SET </span>"
                "<span style=\"color:%2;font-weight:bold\">%3 A</span>"
                "<span style=\"color:%1\">  |  ACTUAL </span>"
                "<span style=\"color:%4;font-weight:bold\">%5 A</span>")
            .arg(Theme::TEXT_DIM, Theme::MAGENTA, sa, Theme::CYAN, aa));
}

void TelemetryPanel::updateSetpoints(double setV, double setA) {
    m_setV = setV;
    m_setA = setA;
    updateSpDisplay();
}

void TelemetryPanel::updateTelemetry(const Message2& msg) {
    m_voutLabel->setText(QString("%1 V").arg(msg.output_voltage, 0, 'f', 1));
    m_ioutLabel->setText(QString("%1 A").arg(msg.output_current, 0, 'f', 1));
    m_vinLabel->setText(QString("%1 V").arg(msg.input_voltage, 0, 'f', 1));
    m_tempLabel->setText(QString("%1 \u00b0C").arg(msg.temperature, 0, 'f', 1));

    m_hwInd->setOk(!msg.status.hardware_failure);
    m_tempInd->setOk(!msg.status.over_temperature);
    m_vinInd->setOk(!msg.status.input_voltage_error);
    m_startInd->setOk(!msg.status.starting_state);
    m_commInd->setOk(!msg.status.communication_timeout);

    m_lastRxLabel->setText(QString("Last RX: %1")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
    m_alarmLabel->setText("");

    m_actualV = msg.output_voltage;
    m_actualA = msg.output_current;
    updateSpDisplay();
}

void TelemetryPanel::setAlarm(const QString& text) {
    m_alarmLabel->setText(text);
}

void TelemetryPanel::clear() {
    for (auto* lbl : {m_voutLabel, m_ioutLabel, m_vinLabel, m_tempLabel})
        lbl->setText(QString::fromUtf8("\u2014"));

    m_lastRxLabel->setText(QString::fromUtf8("Last RX: \u2014"));
    m_alarmLabel->setText("");
    m_setV = m_setA = m_actualV = m_actualA = -1;
    updateSpDisplay();

    for (auto* ind : {m_hwInd, m_tempInd, m_vinInd, m_startInd, m_commInd})
        ind->setOk(true);
}
