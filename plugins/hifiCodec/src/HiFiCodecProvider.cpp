//
//  Created by Brad Hefta-Gaub on 7/10/2016
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
#include <plugins/CodecPlugin.h>

#include "HiFiCodec.h"

class HiFiCodecProvider : public QObject, public CodecProvider {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID CodecProvider_iid FILE "plugin.json")
    Q_INTERFACES(CodecProvider)

public:
    HiFiCodecProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~HiFiCodecProvider() {}

    virtual CodecPluginList getCodecPlugins() override {
        static std::once_flag once;
        std::call_once(once, [&] {

            CodecPluginPointer hiFiCodec(new HiFiCodec());
            if (hiFiCodec->isSupported()) {
                _codecPlugins.push_back(hiFiCodec);
            }

        });
        return _codecPlugins;
    }

private:
    CodecPluginList _codecPlugins;
};

#include "HiFiCodecProvider.moc"
