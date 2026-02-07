#include "profiles.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

QString configDir() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty())
        base = QDir::homePath() + "/.config";
    return base + "/OBC_Controller";
}

static QString profilesPath() {
    return configDir() + "/profiles.json";
}

QMap<QString, Profile> loadProfiles() {
    QMap<QString, Profile> result;
    QFile file(profilesPath());
    if (!file.exists())
        return result;
    if (!file.open(QIODevice::ReadOnly))
        return result;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return result;

    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.value().isObject()) continue;
        QJsonObject obj = it.value().toObject();
        Profile p;
        p.name = it.key();
        p.voltage_set_v     = obj.value("voltage_set_v").toDouble(320.0);
        p.current_set_a     = obj.value("current_set_a").toDouble(50.0);
        p.mode              = obj.value("mode").toString("charging");
        p.ramp_enabled      = obj.value("ramp_enabled").toBool(false);
        p.ramp_rate_v_per_s = obj.value("ramp_rate_v_per_s").toDouble(5.0);
        p.ramp_rate_a_per_s = obj.value("ramp_rate_a_per_s").toDouble(0.5);
        result.insert(p.name, p);
    }
    return result;
}

void saveProfiles(const QMap<QString, Profile>& profiles) {
    QJsonObject root;
    for (auto it = profiles.begin(); it != profiles.end(); ++it) {
        const Profile& p = it.value();
        QJsonObject obj;
        obj["voltage_set_v"]     = p.voltage_set_v;
        obj["current_set_a"]     = p.current_set_a;
        obj["mode"]              = p.mode;
        obj["ramp_enabled"]      = p.ramp_enabled;
        obj["ramp_rate_v_per_s"] = p.ramp_rate_v_per_s;
        obj["ramp_rate_a_per_s"] = p.ramp_rate_a_per_s;
        root.insert(it.key(), obj);
    }

    QDir().mkpath(configDir());
    QFile file(profilesPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

QMap<QString, Profile> saveProfile(const Profile& profile) {
    auto profiles = loadProfiles();
    profiles.insert(profile.name, profile);
    saveProfiles(profiles);
    return profiles;
}

QMap<QString, Profile> deleteProfile(const QString& name) {
    auto profiles = loadProfiles();
    profiles.remove(name);
    saveProfiles(profiles);
    return profiles;
}

QStringList profileNames() {
    auto profiles = loadProfiles();
    QStringList names = profiles.keys();
    names.sort();
    return names;
}
