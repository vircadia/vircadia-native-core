//
//  SettingsScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/22/14
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__SettingsScriptingInterface__
#define __hifi__SettingsScriptingInterface__

#include <QDebug>
#include <QObject>
#include <QString>

#include "Application.h"

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

#endif /* defined(__hifi__SettingsScriptingInterface__) */
