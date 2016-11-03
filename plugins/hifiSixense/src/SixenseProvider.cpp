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
#include <plugins/InputPlugin.h>

#include "SixenseManager.h"

class SixenseProvider : public QObject, public InputProvider
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID InputProvider_iid FILE "plugin.json")
    Q_INTERFACES(InputProvider)

public:
    SixenseProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~SixenseProvider() {}

    virtual InputPluginList getInputPlugins() override {
        static std::once_flag once;
        std::call_once(once, [&] {
            InputPluginPointer plugin(new SixenseManager());
            if (plugin->isSupported()) {
                _inputPlugins.push_back(plugin);
            }
        });
        return _inputPlugins;
    }

    virtual void destroyInputPlugins() override {
        _inputPlugins.clear();
    }
private:
    InputPluginList _inputPlugins;
};

#include "SixenseProvider.moc"
