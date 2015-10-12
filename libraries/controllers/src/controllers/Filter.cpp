//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Filter.h"

#include <QtCore/QObject>
#include <QtScript/QScriptValue>

namespace controller {

    Filter::Pointer Filter::parse(const QJsonObject& json) {
        // FIXME parse the json object and determine the instance type to create
        return Filter::Pointer();
    }
}

