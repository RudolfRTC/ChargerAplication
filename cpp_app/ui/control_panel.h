#pragma once

#include <QGroupBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QFrame>

#include "can_protocol.h"

class ControlPanel : public QGroupBox {
    Q_OBJECT
public:
    explicit ControlPanel(QWidget* parent = nullptr);

    double getVoltage() const;
    double getCurrent() const;
    ChargerControl getControl() const;
    bool getRampEnabled() const;
    std::pair<double, double> getRampRates() const;

    void updateRampDisplay(bool active, double rampedV, double rampedA);

signals:
    void controlChanged(double voltage, double current, int control,
                        bool rampEnabled, double rampV, double rampA);
    void instant360vRequested();
    void profileLoaded();
    void logMessage(const QString& msg);

private slots:
    void onInstant360v();
    void onSaveProfile();
    void onLoadProfile();
    void onDeleteProfile();

private:
    void setControlMode(ChargerControl ctrl);
    void emitState();
    void updateBadge(const QString& text, const QString& styleKey);
    void refreshProfiles();

    ChargerControl m_currentControl = ChargerControl::STOP_OUTPUTTING;

    QFrame*         m_modeBadge;
    QLabel*         m_modeBadgeLabel;
    QComboBox*      m_profileCombo;
    QPushButton*    m_loadBtn;
    QPushButton*    m_saveBtn;
    QPushButton*    m_deleteBtn;
    QDoubleSpinBox* m_voltageSpin;
    QDoubleSpinBox* m_currentSpin;
    QPushButton*    m_startBtn;
    QPushButton*    m_stopBtn;
    QPushButton*    m_heatBtn;
    QPushButton*    m_instantBtn;
    QCheckBox*      m_rampCheck;
    QDoubleSpinBox* m_rampVSpin;
    QDoubleSpinBox* m_rampASpin;
    QLabel*         m_rampActiveLabel;
    QLabel*         m_rampVActual;
    QLabel*         m_rampAActual;
};
