//
//  CodecPlugin.h
//  plugins/src/plugins
//
//  Created by Brad Hefta-Gaub on 6/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "Plugin.h"

class CodecPlugin : public Plugin {
public:
    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) = 0;
};

