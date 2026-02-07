#pragma once

#include <QString>
#include <QMap>
#include <QStringList>

struct Profile {
    QString name;
    double voltage_set_v    = 320.0;
    double current_set_a    = 50.0;
    QString mode            = "charging";  // "charging" | "heating"
    bool ramp_enabled       = false;
    double ramp_rate_v_per_s = 5.0;
    double ramp_rate_a_per_s = 0.5;
};

QString configDir();
QMap<QString, Profile> loadProfiles();
void saveProfiles(const QMap<QString, Profile>& profiles);
QMap<QString, Profile> saveProfile(const Profile& profile);
QMap<QString, Profile> deleteProfile(const QString& name);
QStringList profileNames();
