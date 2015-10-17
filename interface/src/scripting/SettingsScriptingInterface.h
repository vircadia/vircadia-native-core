//
//  SettingsScriptingInterface.h
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 3/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SettingsScriptingInterface_h
#define hifi_SettingsScriptingInterface_h

#include <QObject>
#include <QString>

class SettingsScriptingInterface : public QObject {
    Q_OBJECT
    SettingsScriptingInterface() { };
public:
    static SettingsScriptingInterface* getInstance();

public slots:
    QVariant getValue(const QString& setting);
    QVariant getValue(const QString& setting, const QVariant& defaultValue);
    void setValue(const QString& setting, const QVariant& value);
};

#endif // hifi_SettingsScriptingInterface_h
