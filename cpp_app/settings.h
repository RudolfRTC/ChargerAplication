#pragma once

#include <QJsonObject>
#include <QVariant>
#include <QString>

QJsonObject loadSettings();
void saveSetting(const QString& key, const QJsonValue& value);
