//
//  Created by Bradley Austin Davis on 2015/05/30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

class DisplayPlugin;

#include <QVector>
#include <QSharedPointer>

using DisplayPluginPointer = QSharedPointer<DisplayPlugin>;
using DisplayPluginList = QVector<DisplayPluginPointer>;

const DisplayPluginList& getDisplayPlugins();