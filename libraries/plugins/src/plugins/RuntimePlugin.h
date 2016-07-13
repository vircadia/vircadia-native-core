//
//  Created by Bradley Austin Davis on 2015/10/24
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <assert.h>

#include <QString>
#include <QObject>

#include "Forward.h"

class DisplayProvider {
public:
    virtual ~DisplayProvider() {}

    virtual DisplayPluginList getDisplayPlugins() = 0;
};

#define DisplayProvider_iid "com.highfidelity.plugins.display"
Q_DECLARE_INTERFACE(DisplayProvider, DisplayProvider_iid)


class InputProvider {
public:
    virtual ~InputProvider() {}
    virtual InputPluginList getInputPlugins() = 0;
};

#define InputProvider_iid "com.highfidelity.plugins.input"
Q_DECLARE_INTERFACE(InputProvider, InputProvider_iid)

class CodecProvider {
public:
    virtual ~CodecProvider() {}
    virtual CodecPluginList getCodecPlugins() = 0;
};

#define CodecProvider_iid "com.highfidelity.plugins.codec"
Q_DECLARE_INTERFACE(CodecProvider, CodecProvider_iid)

