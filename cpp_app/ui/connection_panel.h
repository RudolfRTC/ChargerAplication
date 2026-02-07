#pragma once

#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

class ConnectionPanel : public QGroupBox {
    Q_OBJECT
public:
    explicit ConnectionPanel(QWidget* parent = nullptr);

    void setConnected(bool connected);
    void updateHealth(double txRate, double rxRate, double lastRxAge);
    void setBaudSwitchBusy(bool busy);
    void setBaudSwitchProgress(int step, int total);
    void setBaudSwitchDone();

signals:
    void connectRequested(const QString& iface, const QString& channel, int bitrate, bool simulate);
    void disconnectRequested();
    void baudrateSwitchRequested();

private slots:
    void onBackendChanged(const QString& text);
    void onConnect();
    void onDisconnect();
    void onBaudSwitch();

private:
    void resetHealth();

    QComboBox*   m_backendCombo;
    QLineEdit*   m_channelEdit;
    QComboBox*   m_bitrateCombo;
    QCheckBox*   m_simCheck;
    QPushButton* m_connectBtn;
    QPushButton* m_disconnectBtn;
    QLabel*      m_statusLabel;
    QPushButton* m_baudSwitchBtn;
    QLabel*      m_baudStatus;

    // Health labels
    QLabel* m_txRateLbl;
    QLabel* m_rxRateLbl;
    QLabel* m_rxAgeLbl;
    QLabel* m_commLbl;
    QLabel* m_bitrateLbl;
};
