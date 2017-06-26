//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InputConfiguration_h
#define hifi_InputConfiguration_h

#include <mutex>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <DependencyManager.h>

class InputConfiguration : public QObject, public Dependency {
    Q_OBJECT
public:
    InputConfiguration();

    Q_INVOKABLE QStringList inputPlugins();
    Q_INVOKABLE QStringList activeInputPlugins();
    Q_INVOKABLE QString configurationLayout(QString pluginName);
    Q_INVOKABLE void setConfigurationSettings(QJsonObject configurationSettings, QString pluginName);
    Q_INVOKABLE void calibratePlugin(QString pluginName);
    Q_INVOKABLE QJsonObject configurationSettings(QString pluginName);
    Q_INVOKABLE bool uncalibratePlugin(QString pluginName);

signals:
    void calibrationStatus(const QJsonObject& status);
};

#endif
