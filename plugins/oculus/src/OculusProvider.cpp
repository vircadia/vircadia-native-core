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
#include <plugins/InputPlugin.h>

#include "OculusDisplayPlugin.h"
#include "OculusDebugDisplayPlugin.h"
#include "OculusPlatformPlugin.h"
#include "OculusControllerManager.h"

class OculusProvider : public QObject, public DisplayProvider, InputProvider, OculusPlatformProvider
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DisplayProvider_iid FILE "oculus.json")
    Q_INTERFACES(DisplayProvider)
    Q_PLUGIN_METADATA(IID InputProvider_iid FILE "oculus.json")
    Q_INTERFACES(InputProvider)
    Q_PLUGIN_METADATA(IID OculusPlatformProvider_iid FILE "oculus.json")
    Q_INTERFACES(OculusPlatformProvider)

public:
    OculusProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~OculusProvider() {}

    virtual DisplayPluginList getDisplayPlugins() override {
        static std::once_flag once;
        std::call_once(once, [&] {
            DisplayPluginPointer plugin(std::make_shared<OculusDisplayPlugin>());
            if (plugin->isSupported()) {
                _displayPlugins.push_back(plugin);
            }

            // Windows Oculus Simulator... uses head tracking and the same rendering 
            // as the connected hardware, but without using the SDK to display to the 
            // Rift.  Useful for debugging Rift performance with nSight.
            plugin = std::make_shared<OculusDebugDisplayPlugin>();
            if (plugin->isSupported()) {
                _displayPlugins.push_back(plugin);
            }
        });
        return _displayPlugins;
    }

    virtual InputPluginList getInputPlugins() override {
        static std::once_flag once;
        std::call_once(once, [&] {
            InputPluginPointer plugin(std::make_shared<OculusControllerManager>());
            if (plugin->isSupported()) {
                _inputPlugins.push_back(plugin);
            }
        });
        return _inputPlugins;
    }

    virtual OculusPlatformPluginPointer getOculusPlatformPlugin() override {
        static std::once_flag once;
        std::call_once(once, [&] {
            _oculusPlatformPlugin = std::make_shared<OculusAPIPlugin>();
            
        });
        return _oculusPlatformPlugin;
    }

    virtual void destroyInputPlugins() override {
        _inputPlugins.clear();
    }

    virtual void destroyDisplayPlugins() override {
        _displayPlugins.clear();
    }

private:
    DisplayPluginList _displayPlugins;
    InputPluginList _inputPlugins;
    OculusPlatformPluginPointer _oculusPlatformPlugin;
};

#include "OculusProvider.moc"
