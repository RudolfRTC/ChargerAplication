#include "ui/connection_panel.h"
#include "ui/theme.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>

ConnectionPanel::ConnectionPanel(QWidget* parent)
    : QGroupBox("Connection", parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    // Read CLI/settings defaults from QApplication properties
    QString defBackend = qApp->property("can_backend").toString();
    QString defChannel = qApp->property("can_channel").toString();
    int     defBitrate = qApp->property("can_bitrate").toInt();

    // Row 1: backend
    auto* row1 = new QHBoxLayout;
    row1->addWidget(new QLabel("Backend:"));
    m_backendCombo = new QComboBox;
    m_backendCombo->addItems({"auto", "socketcan", "pcan"});
    if (!defBackend.isEmpty()) {
        int idx = m_backendCombo->findText(defBackend);
        if (idx >= 0) m_backendCombo->setCurrentIndex(idx);
    }
    row1->addWidget(m_backendCombo, 1);
    layout->addLayout(row1);

    // Row 2: channel
    auto* row2 = new QHBoxLayout;
    row2->addWidget(new QLabel("Channel:"));
    m_channelEdit = new QLineEdit(defChannel.isEmpty() ? "can0" : defChannel);
    row2->addWidget(m_channelEdit, 1);
    layout->addLayout(row2);

    // Row 3: bitrate
    auto* row3 = new QHBoxLayout;
    row3->addWidget(new QLabel("CAN bitrate:"));
    m_bitrateCombo = new QComboBox;
    m_bitrateCombo->addItems({"250000", "500000", "1000000", "125000"});
    if (defBitrate > 0) {
        QString brStr = QString::number(defBitrate);
        int idx = m_bitrateCombo->findText(brStr);
        if (idx >= 0) m_bitrateCombo->setCurrentIndex(idx);
    }
    row3->addWidget(m_bitrateCombo, 1);
    layout->addLayout(row3);

    // Row 4: simulate checkbox
    m_simCheck = new QCheckBox("Simulate (no HW)");
    layout->addWidget(m_simCheck);

    // Row 5: connect / disconnect
    auto* btnRow = new QHBoxLayout;
    m_connectBtn = new QPushButton("Connect");
    m_connectBtn->setObjectName("btn_connect");
    m_disconnectBtn = new QPushButton("Disconnect");
    m_disconnectBtn->setObjectName("btn_disconnect");
    m_disconnectBtn->setEnabled(false);
    btnRow->addWidget(m_connectBtn);
    btnRow->addWidget(m_disconnectBtn);
    layout->addLayout(btnRow);

    // Status label
    m_statusLabel = new QLabel("Disconnected");
    m_statusLabel->setObjectName("status_disconnected");
    layout->addWidget(m_statusLabel);

    // Baudrate switch
    m_baudSwitchBtn = new QPushButton(QString::fromUtf8("Baudrate \u2192 500k"));
    m_baudSwitchBtn->setObjectName("btn_baud");
    m_baudSwitchBtn->setToolTip("Send CAN sequence to switch OBC device to 500 kbps");
    m_baudSwitchBtn->setEnabled(false);
    layout->addWidget(m_baudSwitchBtn);

    m_baudStatus = new QLabel("");
    m_baudStatus->setObjectName("baud_status");
    layout->addWidget(m_baudStatus);

    // Health panel
    auto* healthGroup = new QGroupBox("Health");
    auto* healthLay = new QVBoxLayout(healthGroup);
    healthLay->setSpacing(4);

    QString mono = QString("font-family: 'Cascadia Code','Fira Code','Consolas',monospace; "
                           "font-size: 12px; color: %1;").arg(Theme::TEXT_DIM);

    m_txRateLbl  = new QLabel(QString::fromUtf8("TX: \u2014 /s"));
    m_rxRateLbl  = new QLabel(QString::fromUtf8("RX: \u2014 /s"));
    m_rxAgeLbl   = new QLabel(QString::fromUtf8("Last RX: \u2014 s"));
    m_commLbl    = new QLabel(QString::fromUtf8("Comm: \u2014"));
    m_bitrateLbl = new QLabel(QString::fromUtf8("Bitrate: \u2014"));

    for (auto* lbl : {m_txRateLbl, m_rxRateLbl, m_rxAgeLbl, m_commLbl, m_bitrateLbl}) {
        lbl->setStyleSheet(mono);
        healthLay->addWidget(lbl);
    }
    layout->addWidget(healthGroup);

    // Signals
    connect(m_connectBtn, &QPushButton::clicked, this, &ConnectionPanel::onConnect);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &ConnectionPanel::onDisconnect);
    connect(m_baudSwitchBtn, &QPushButton::clicked, this, &ConnectionPanel::onBaudSwitch);
    connect(m_backendCombo, &QComboBox::currentTextChanged, this, &ConnectionPanel::onBackendChanged);
}

void ConnectionPanel::onBackendChanged(const QString& text) {
    if (text == "pcan")
        m_channelEdit->setText("PCAN_USBBUS1");
    else if (text == "socketcan")
        m_channelEdit->setText("can0");
    else if (text == "auto") {
#ifdef _WIN32
        m_channelEdit->setText("PCAN_USBBUS1");
#else
        m_channelEdit->setText("can0");
#endif
    }
}

void ConnectionPanel::onConnect() {
    QString iface = m_backendCombo->currentText();
    QString channel = m_channelEdit->text().trimmed();
    int bitrate = m_bitrateCombo->currentText().toInt();
    bool sim = m_simCheck->isChecked();
    emit connectRequested(iface, channel, bitrate, sim);
}

void ConnectionPanel::onDisconnect() {
    emit disconnectRequested();
}

void ConnectionPanel::onBaudSwitch() {
    emit baudrateSwitchRequested();
}

void ConnectionPanel::setConnected(bool connected) {
    m_connectBtn->setEnabled(!connected);
    m_disconnectBtn->setEnabled(connected);
    m_backendCombo->setEnabled(!connected);
    m_channelEdit->setEnabled(!connected);
    m_bitrateCombo->setEnabled(!connected);
    m_simCheck->setEnabled(!connected);

    bool isSim = m_simCheck->isChecked();
    m_baudSwitchBtn->setEnabled(connected && !isSim);

    if (connected) {
        m_statusLabel->setText(QString::fromUtf8("\u25cf  Connected"));
        m_statusLabel->setObjectName("status_connected");
        int bitrate = m_bitrateCombo->currentText().toInt();
        m_bitrateLbl->setText(QString("Bitrate: %1k").arg(bitrate / 1000));
    } else {
        m_statusLabel->setText(QString::fromUtf8("\u25cb  Disconnected"));
        m_statusLabel->setObjectName("status_disconnected");
        resetHealth();
    }
    // Force style recalculation
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
    m_baudStatus->setText("");
}

void ConnectionPanel::updateHealth(double txRate, double rxRate, double lastRxAge) {
    m_txRateLbl->setText(QString("TX: %1 /s").arg(txRate, 0, 'f', 1));
    m_rxRateLbl->setText(QString("RX: %1 /s").arg(rxRate, 0, 'f', 1));
    m_rxAgeLbl->setText(QString("Last RX: %1 s").arg(lastRxAge, 0, 'f', 1));

    QString mono = "font-family: 'Cascadia Code','Fira Code','Consolas',monospace; font-size: 12px;";
    if (lastRxAge > 5.0) {
        m_commLbl->setText("Comm: TIMEOUT");
        m_commLbl->setStyleSheet(QString("color: %1; font-weight: bold; %2").arg(Theme::RED, mono));
    } else {
        m_commLbl->setText("Comm: OK");
        m_commLbl->setStyleSheet(QString("color: %1; font-weight: bold; %2").arg(Theme::GREEN, mono));
    }
}

void ConnectionPanel::resetHealth() {
    QString mono = QString("font-family: 'Cascadia Code','Fira Code','Consolas',monospace; "
                           "font-size: 12px; color: %1;").arg(Theme::TEXT_DIM);
    m_txRateLbl->setText(QString::fromUtf8("TX: \u2014 /s"));
    m_rxRateLbl->setText(QString::fromUtf8("RX: \u2014 /s"));
    m_rxAgeLbl->setText(QString::fromUtf8("Last RX: \u2014 s"));
    m_commLbl->setText(QString::fromUtf8("Comm: \u2014"));
    m_commLbl->setStyleSheet(mono);
    m_bitrateLbl->setText(QString::fromUtf8("Bitrate: \u2014"));
}

void ConnectionPanel::setBaudSwitchBusy(bool busy) {
    m_baudSwitchBtn->setEnabled(!busy);
}

void ConnectionPanel::setBaudSwitchProgress(int step, int total) {
    m_baudStatus->setText(QString::fromUtf8("Switching\u2026 (%1/%2)").arg(step).arg(total));
    m_baudStatus->setStyleSheet("color: #FF9800; font-weight: bold;");
}

void ConnectionPanel::setBaudSwitchDone() {
    m_baudStatus->setText(QString::fromUtf8("\u2713 Baudrate switch done"));
    m_baudStatus->setStyleSheet("color: #00e676; font-weight: bold;");
    m_baudSwitchBtn->setEnabled(true);
}
