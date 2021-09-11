//
//  Created by Michael Bailey on 12/20/2019
//  Copyright 2019 Michael Bailey
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

#include "OpusCodecManager.h"

class AthenaOpusCodecProvider : public QObject, public CodecProvider {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID CodecProvider_iid FILE "plugin.json")
    Q_INTERFACES(CodecProvider)

public:
    AthenaOpusCodecProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~AthenaOpusCodecProvider() {}

    virtual CodecPluginList getCodecPlugins() override {
        static std::once_flag once;
        std::call_once(once, [&] {

            CodecPluginPointer opusCodec(std::make_shared<AthenaOpusCodec>());
            if (opusCodec->isSupported()) {
                _codecPlugins.push_back(opusCodec);
            }
        });
        return _codecPlugins;
    }

private:
    CodecPluginList _codecPlugins;
};

#include "OpusCodecProvider.moc"
