//
//  Created by Clément Brisset on 12/14/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <mutex>

#include <QtCore/QObject>
#include <QtCore/QtPlugin>
#include <QtCore/QStringList>

#include <plugins/RuntimePlugin.h>

#include "OculusAPIPlugin.h"

class OculusAPIProvider : public QObject, public OculusPlatformProvider {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OculusPlatformProvider_iid FILE "plugin.json")
    Q_INTERFACES(OculusPlatformProvider)

public:
    OculusAPIProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~OculusAPIProvider() {}

    virtual OculusPlatformPluginPointer getOculusPlatformPlugin() override {
        static std::once_flag once;
        std::call_once(once, [&] {
            _oculusPlatformPlugin = std::make_shared<OculusAPIPlugin>();
        });
        return _oculusPlatformPlugin;
    }

private:
    OculusPlatformPluginPointer _oculusPlatformPlugin;
};

#include "OculusPlatformProvider.moc"
