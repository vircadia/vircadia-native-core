//
//  Created by Bradley Austin Davis on 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <mutex>

#include <QtCore/QObject>
#include <QtCore/QtPlugin>
#include <QtCore/QStringList>

#include <plugins/RuntimePlugin.h>
#include <plugins/DisplayPlugin.h>

#include "OculusLegacyDisplayPlugin.h"

class OculusProvider : public QObject, public DisplayProvider
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DisplayProvider_iid FILE "oculus.json")
    Q_INTERFACES(DisplayProvider)

public:
    OculusProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~OculusProvider() {}

    virtual DisplayPluginList getDisplayPlugins() override {
        static std::once_flag once;
        std::call_once(once, [&] {
            DisplayPluginPointer plugin(new OculusLegacyDisplayPlugin());
            if (plugin->isSupported()) {
                _displayPlugins.push_back(plugin);
            }
        });
        return _displayPlugins;
    }

    virtual void destroyDisplayPlugins() override {
        _displayPlugins.clear();
    }

private:
    DisplayPluginList _displayPlugins;
};

#include "OculusProvider.moc"
