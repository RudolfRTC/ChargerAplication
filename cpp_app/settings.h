#pragma once

#include <QJsonObject>
#include <QVariant>
#include <QString>

#include <string>

QJsonObject loadSettings();
void saveSetting(const QString& key, const QJsonValue& value);

/// CAN-specific settings (loaded from settings.json + CLI overrides)
struct CanSettings {
    std::string backend  = "auto";       // auto|socketcan|pcan
    std::string channel;                 // e.g. "can0" or "PCAN_USBBUS1" (empty = OS default)
    int         bitrate  = 250000;       // bps
    bool        extendedDefault = true;

    /// Resolve empty channel to OS-specific default
    void applyDefaults();
};

/// Load CAN settings from settings.json
CanSettings loadCanSettings();

/// Save CAN settings to settings.json
void saveCanSettings(const CanSettings& s);
