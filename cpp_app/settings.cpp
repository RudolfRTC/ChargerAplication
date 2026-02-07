#include "settings.h"
#include "profiles.h"  // for configDir()

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
