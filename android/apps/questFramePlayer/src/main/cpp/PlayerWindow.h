//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <QtGui/QWindow>
#include "RenderThread.h"

class PlayerWindow : public QWindow {
public:
    PlayerWindow();
    virtual ~PlayerWindow() {}

private:
    RenderThread _renderThread;
};
