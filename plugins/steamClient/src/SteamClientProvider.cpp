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
#include <plugins/SteamClientPlugin.h>

#include "SteamAPIPlugin.h"

class SteamAPIProvider : public QObject, public SteamClientProvider {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID SteamClientProvider_iid FILE "plugin.json")
    Q_INTERFACES(SteamClientProvider)

public:
    SteamAPIProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~SteamAPIProvider() {}

    virtual SteamClientPluginPointer getSteamClientPlugin() override {
        static std::once_flag once;
        std::call_once(once, [&] {
            _steamClientPlugin = std::make_shared<SteamAPIPlugin>();
        });
        return _steamClientPlugin;
    }

private:
    SteamClientPluginPointer _steamClientPlugin;
};

#include "SteamClientProvider.moc"
