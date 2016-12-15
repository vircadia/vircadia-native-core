//
//  SettingHelpers.h
//  libraries/shared/src
//
//  Created by Clement on 9/13/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SettingHelpers_h
#define hifi_SettingHelpers_h

#include <QSettings>

bool readJSONFile(QIODevice& device, QSettings::SettingsMap& map);
bool writeJSONFile(QIODevice& device, const QSettings::SettingsMap& map);

static const auto JSON_FORMAT = QSettings::registerFormat("json", readJSONFile, writeJSONFile);

void loadOldINIFile(QSettings& settings);


#endif // hifi_SettingHelpers_h
