#include "settings.h"
#include "profiles.h"  // for configDir()
#include "can_factory.h" // for parseBitrateString()

#include <QDir>
#include <QFile>
#include <QJsonDocument>

static QString settingsPath() {
    return configDir() + "/settings.json";
}

QJsonObject loadSettings() {
    QFile file(settingsPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return {};

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return {};
    return doc.object();
}

void saveSetting(const QString& key, const QJsonValue& value) {
    QJsonObject settings = loadSettings();
    settings.insert(key, value);

    QDir().mkpath(configDir());
    QFile file(settingsPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(settings).toJson(QJsonDocument::Indented));
        file.close();
    }
}

// --- CAN settings ---

void CanSettings::applyDefaults() {
    if (channel.empty()) {
#ifdef _WIN32
        channel = "PCAN_USBBUS1";
#else
        channel = "can0";
#endif
    }
}

CanSettings loadCanSettings() {
    CanSettings s;
    QJsonObject obj = loadSettings();

    if (obj.contains("can_backend"))
        s.backend = obj.value("can_backend").toString("auto").toStdString();
    if (obj.contains("can_channel"))
        s.channel = obj.value("can_channel").toString().toStdString();
    if (obj.contains("can_bitrate")) {
        QString val = obj.value("can_bitrate").toString();
        int bps = parseBitrateString(val.toStdString());
        if (bps > 0) s.bitrate = bps;
    }
    if (obj.contains("can_extended"))
        s.extendedDefault = obj.value("can_extended").toBool(true);

    return s;
}

void saveCanSettings(const CanSettings& s) {
    saveSetting("can_backend",  QString::fromStdString(s.backend));
    saveSetting("can_channel",  QString::fromStdString(s.channel));
    saveSetting("can_extended", s.extendedDefault);

    // Store bitrate as human-readable string
    QString brStr;
    switch (s.bitrate) {
        case 1000000: brStr = "1M";   break;
        case 800000:  brStr = "800K"; break;
        case 500000:  brStr = "500K"; break;
        case 250000:  brStr = "250K"; break;
        case 125000:  brStr = "125K"; break;
        case 100000:  brStr = "100K"; break;
        case 50000:   brStr = "50K";  break;
        case 20000:   brStr = "20K";  break;
        case 10000:   brStr = "10K";  break;
        default:      brStr = QString::number(s.bitrate); break;
    }
    saveSetting("can_bitrate", brStr);
}
