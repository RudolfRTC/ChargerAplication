#include "ui/control_panel.h"
#include "ui/theme.h"
#include "profiles.h"
#include "settings.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>

// Badge colour configs
struct BadgeStyle {
    const char* color;
    const char* bgStart;
    const char* bgEnd;
};

static const QMap<QString, BadgeStyle> BADGE_STYLES = {
    {"stop",     {Theme::RED,    "rgba(255,23,68,0.25)",   "rgba(255,23,68,0.08)"}},
    {"charging", {Theme::GREEN,  "rgba(0,230,118,0.25)",   "rgba(0,230,118,0.08)"}},
    {"heating",  {Theme::ORANGE, "rgba(255,145,0,0.25)",   "rgba(255,145,0,0.08)"}},
    {"instant",  {Theme::VIOLET, "rgba(124,77,255,0.30)",  "rgba(124,77,255,0.10)"}},
};

ControlPanel::ControlPanel(QWidget* parent)
    : QGroupBox("Control", parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    // Mode badge
    m_modeBadge = new QFrame;
    m_modeBadge->setFixedHeight(46);
    auto* badgeLay = new QHBoxLayout(m_modeBadge);
    badgeLay->setContentsMargins(0, 0, 0, 0);
    m_modeBadgeLabel = new QLabel("STOP");
    m_modeBadgeLabel->setAlignment(Qt::AlignCenter);
    badgeLay->addWidget(m_modeBadgeLabel);
    layout->addWidget(m_modeBadge);
    updateBadge("STOP", "stop");

    // Profiles section
    auto* profGroup = new QGroupBox("Profiles");
    auto* profLayout = new QVBoxLayout(profGroup);

    auto* profRow1 = new QHBoxLayout;
    profRow1->addWidget(new QLabel("Profile:"));
    m_profileCombo = new QComboBox;
    m_profileCombo->setMinimumWidth(120);
    profRow1->addWidget(m_profileCombo, 1);
    profLayout->addLayout(profRow1);

    auto* profRow2 = new QHBoxLayout;
    m_loadBtn = new QPushButton("Load");
    m_saveBtn = new QPushButton("Save");
    m_deleteBtn = new QPushButton("Delete");
    profRow2->addWidget(m_loadBtn);
    profRow2->addWidget(m_saveBtn);
    profRow2->addWidget(m_deleteBtn);
    profLayout->addLayout(profRow2);
    layout->addWidget(profGroup);

    // Setpoints
    auto* spGroup = new QGroupBox("Setpoints");
    auto* spLayout = new QVBoxLayout(spGroup);

    auto* rowV = new QHBoxLayout;
    rowV->addWidget(new QLabel("Voltage (V):"));
    m_voltageSpin = new QDoubleSpinBox;
    m_voltageSpin->setRange(0.0, 6553.5);
    m_voltageSpin->setDecimals(1);
    m_voltageSpin->setSingleStep(0.1);
    m_voltageSpin->setValue(320.0);
    rowV->addWidget(m_voltageSpin, 1);
    spLayout->addLayout(rowV);

    auto* rowI = new QHBoxLayout;
    rowI->addWidget(new QLabel("Current (A):"));
    m_currentSpin = new QDoubleSpinBox;
    m_currentSpin->setRange(0.0, 6553.5);
    m_currentSpin->setDecimals(1);
    m_currentSpin->setSingleStep(0.1);
    m_currentSpin->setValue(50.0);
    rowI->addWidget(m_currentSpin, 1);
    spLayout->addLayout(rowI);
    layout->addWidget(spGroup);

    // Mode buttons
    m_startBtn = new QPushButton("Start Charging");
    m_startBtn->setObjectName("btn_start");
    m_stopBtn = new QPushButton("Stop Outputting");
    m_stopBtn->setObjectName("btn_stop");
    m_heatBtn = new QPushButton("Heating / DC");
    m_heatBtn->setObjectName("btn_heat");
    layout->addWidget(m_startBtn);
    layout->addWidget(m_stopBtn);
    layout->addWidget(m_heatBtn);

    // Instant preset
    m_instantBtn = new QPushButton(QString::fromUtf8("\u26a1 360V / 9A Instant"));
    m_instantBtn->setObjectName("btn_instant");
    m_instantBtn->setToolTip("One-touch: 360V / 9A, ramp OFF, Heating/DC Supply mode");
    layout->addWidget(m_instantBtn);

    // Ramp section
    auto* rampGroup = new QGroupBox("Ramp (soft-start)");
    auto* rampLayout = new QVBoxLayout(rampGroup);

    m_rampCheck = new QCheckBox("Enable Ramp");
    rampLayout->addWidget(m_rampCheck);

    auto* rampVRow = new QHBoxLayout;
    rampVRow->addWidget(new QLabel("V/s:"));
    m_rampVSpin = new QDoubleSpinBox;
    m_rampVSpin->setRange(0.1, 500.0);
    m_rampVSpin->setDecimals(1);
    m_rampVSpin->setSingleStep(0.5);
    m_rampVSpin->setValue(5.0);
    rampVRow->addWidget(m_rampVSpin, 1);
    rampLayout->addLayout(rampVRow);

    auto* rampARow = new QHBoxLayout;
    rampARow->addWidget(new QLabel("A/s:"));
    m_rampASpin = new QDoubleSpinBox;
    m_rampASpin->setRange(0.1, 500.0);
    m_rampASpin->setDecimals(1);
    m_rampASpin->setSingleStep(0.1);
    m_rampASpin->setValue(0.5);
    rampARow->addWidget(m_rampASpin, 1);
    rampLayout->addLayout(rampARow);

    m_rampActiveLabel = new QLabel("");
    m_rampActiveLabel->setObjectName("ramp_active");
    rampLayout->addWidget(m_rampActiveLabel);

    auto* rampVals = new QHBoxLayout;
    m_rampVActual = new QLabel(QString::fromUtf8("V: \u2014"));
    m_rampAActual = new QLabel(QString::fromUtf8("A: \u2014"));
    rampVals->addWidget(m_rampVActual);
    rampVals->addWidget(m_rampAActual);
    rampLayout->addLayout(rampVals);
    layout->addWidget(rampGroup);

    // Wire signals
    connect(m_startBtn, &QPushButton::clicked, this, [this]() {
        setControlMode(ChargerControl::START_CHARGING);
    });
    connect(m_stopBtn, &QPushButton::clicked, this, [this]() {
        setControlMode(ChargerControl::STOP_OUTPUTTING);
    });
    connect(m_heatBtn, &QPushButton::clicked, this, [this]() {
        setControlMode(ChargerControl::HEATING_DC_SUPPLY);
    });

    connect(m_voltageSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { emitState(); });
    connect(m_currentSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { emitState(); });
    connect(m_rampCheck, &QCheckBox::toggled, this, [this]() { emitState(); });
    connect(m_rampVSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { emitState(); });
    connect(m_rampASpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { emitState(); });

    connect(m_instantBtn, &QPushButton::clicked, this, &ControlPanel::onInstant360v);
    connect(m_loadBtn, &QPushButton::clicked, this, &ControlPanel::onLoadProfile);
    connect(m_saveBtn, &QPushButton::clicked, this, &ControlPanel::onSaveProfile);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ControlPanel::onDeleteProfile);

    refreshProfiles();
    setEnabled(false);
}

void ControlPanel::updateBadge(const QString& text, const QString& styleKey) {
    auto it = BADGE_STYLES.find(styleKey);
    BadgeStyle bs = (it != BADGE_STYLES.end()) ? it.value() : BADGE_STYLES["stop"];

    m_modeBadgeLabel->setText(text);
    m_modeBadge->setStyleSheet(
        QString("QFrame { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, "
                "stop:0 %1, stop:1 %2); "
                "border: 2px solid %3; border-radius: 10px; }")
            .arg(bs.bgStart, bs.bgEnd, bs.color));
    m_modeBadgeLabel->setStyleSheet(
        QString("color: %1; font-size: 18px; font-weight: bold; "
                "letter-spacing: 3px; background: transparent;")
            .arg(bs.color));
}

void ControlPanel::onInstant360v() {
    auto settings = loadSettings();
    if (!settings.value("skip_instant_confirm").toBool(false)) {
        auto* dlg = new QMessageBox(this);
        dlg->setWindowTitle("Confirm Instant Preset");
        dlg->setText("This will immediately set 360 V / 9 A "
                     "in HEATING/DC mode.\nContinue?");
        dlg->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        dlg->setDefaultButton(QMessageBox::Cancel);
        auto* cb = new QCheckBox(QString::fromUtf8("Don\u2019t ask again"));
        dlg->setCheckBox(cb);
        int result = dlg->exec();
        if (result != QMessageBox::Yes) {
            delete dlg;
            return;
        }
        if (cb->isChecked())
            saveSetting("skip_instant_confirm", true);
        delete dlg;
    }

    m_voltageSpin->setValue(360.0);
    m_currentSpin->setValue(9.0);
    m_rampCheck->setChecked(false);
    setControlMode(ChargerControl::HEATING_DC_SUPPLY);
    updateBadge(QString::fromUtf8("\u26a1 360V / 9A"), "instant");
    emit instant360vRequested();
    emit logMessage("Instant preset: 360V / 9A, ramp OFF, Heating/DC Supply mode");
}

void ControlPanel::refreshProfiles() {
    QString current = m_profileCombo->currentText();
    m_profileCombo->clear();
    auto names = profileNames();
    for (const auto& name : names)
        m_profileCombo->addItem(name);
    int idx = m_profileCombo->findText(current);
    if (idx >= 0)
        m_profileCombo->setCurrentIndex(idx);
}

void ControlPanel::onSaveProfile() {
    QString defaultName = m_profileCombo->currentText();
    if (defaultName.isEmpty())
        defaultName = QString("Profile %1").arg(QDateTime::currentDateTime().toString("HHmmss"));

    bool ok;
    QString name = QInputDialog::getText(this, "Save Profile", "Profile name:",
                                         QLineEdit::Normal, defaultName, &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    QString mode = "charging";
    if (m_currentControl == ChargerControl::HEATING_DC_SUPPLY)
        mode = "heating";

    Profile p;
    p.name = name;
    p.voltage_set_v = m_voltageSpin->value();
    p.current_set_a = m_currentSpin->value();
    p.mode = mode;
    p.ramp_enabled = m_rampCheck->isChecked();
    p.ramp_rate_v_per_s = m_rampVSpin->value();
    p.ramp_rate_a_per_s = m_rampASpin->value();

    saveProfile(p);
    refreshProfiles();
    m_profileCombo->setCurrentText(name);
    emit logMessage(QString("Profile '%1' saved.").arg(name));
}

void ControlPanel::onLoadProfile() {
    QString name = m_profileCombo->currentText();
    if (name.isEmpty()) return;

    auto profiles = loadProfiles();
    auto it = profiles.find(name);
    if (it == profiles.end()) {
        emit logMessage(QString("Profile '%1' not found.").arg(name));
        return;
    }
    const Profile& p = it.value();

    m_voltageSpin->setValue(p.voltage_set_v);
    m_currentSpin->setValue(p.current_set_a);
    m_rampCheck->setChecked(p.ramp_enabled);
    m_rampVSpin->setValue(p.ramp_rate_v_per_s);
    m_rampASpin->setValue(p.ramp_rate_a_per_s);

    if (p.mode == "heating")
        setControlMode(ChargerControl::HEATING_DC_SUPPLY);
    else
        setControlMode(ChargerControl::START_CHARGING);

    emit profileLoaded();
    emit logMessage(QString("Profile '%1' loaded: %2V / %3A / %4 / ramp=%5")
                        .arg(name)
                        .arg(p.voltage_set_v, 0, 'f', 1)
                        .arg(p.current_set_a, 0, 'f', 1)
                        .arg(p.mode)
                        .arg(p.ramp_enabled ? "ON" : "OFF"));
}

void ControlPanel::onDeleteProfile() {
    QString name = m_profileCombo->currentText();
    if (name.isEmpty()) return;

    auto reply = QMessageBox::question(this, "Delete Profile",
                                       QString("Delete profile '%1'?").arg(name),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        deleteProfile(name);
        refreshProfiles();
        emit logMessage(QString("Profile '%1' deleted.").arg(name));
    }
}

void ControlPanel::setControlMode(ChargerControl ctrl) {
    m_currentControl = ctrl;
    struct BadgeInfo { QString text; QString key; };
    static const QMap<ChargerControl, BadgeInfo> badgeMap = {
        {ChargerControl::STOP_OUTPUTTING,   {"STOP", "stop"}},
        {ChargerControl::START_CHARGING,    {"CHARGING", "charging"}},
        {ChargerControl::HEATING_DC_SUPPLY, {"HEATING / DC", "heating"}},
    };
    auto it = badgeMap.find(ctrl);
    if (it != badgeMap.end())
        updateBadge(it->text, it->key);
    emitState();
}

void ControlPanel::emitState() {
    emit controlChanged(
        m_voltageSpin->value(),
        m_currentSpin->value(),
        static_cast<int>(m_currentControl),
        m_rampCheck->isChecked(),
        m_rampVSpin->value(),
        m_rampASpin->value());
}

void ControlPanel::updateRampDisplay(bool active, double rampedV, double rampedA) {
    if (active) {
        m_rampActiveLabel->setText(QString::fromUtf8("\u25b6 RAMP ACTIVE"));
        m_rampVActual->setText(QString("V: %1").arg(rampedV, 0, 'f', 1));
        m_rampAActual->setText(QString("A: %1").arg(rampedA, 0, 'f', 1));
    } else {
        m_rampActiveLabel->setText("");
        m_rampVActual->setText(QString::fromUtf8("V: \u2014"));
        m_rampAActual->setText(QString::fromUtf8("A: \u2014"));
    }
}

double ControlPanel::getVoltage() const { return m_voltageSpin->value(); }
double ControlPanel::getCurrent() const { return m_currentSpin->value(); }
ChargerControl ControlPanel::getControl() const { return m_currentControl; }
bool ControlPanel::getRampEnabled() const { return m_rampCheck->isChecked(); }
std::pair<double, double> ControlPanel::getRampRates() const {
    return {m_rampVSpin->value(), m_rampASpin->value()};
}
