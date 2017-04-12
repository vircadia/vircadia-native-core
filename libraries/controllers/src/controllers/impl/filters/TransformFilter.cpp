//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TransformFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <StreamUtils.h>

using namespace controller;

bool TransformFilter::parseParameters(const QJsonValue& parameters) {
    return parseMat4Parameter(parameters, _transform);
}
