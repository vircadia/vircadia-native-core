//
//  Created by Sam Gondelman on 7/16/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <plugins/PluginContainer.h>

class InputPlugin;

#include <QVector>
#include <QSharedPointer>

using InputPluginPointer = QSharedPointer<InputPlugin>;
using InputPluginList = QVector<InputPluginPointer>;

const InputPluginList& getInputPlugins();