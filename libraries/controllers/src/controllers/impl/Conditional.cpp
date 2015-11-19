//
//  Created by Bradley Austin Davis 2015/10/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Conditional.h"

#include <QtCore/QJsonValue>

#include "Endpoint.h"

namespace controller {

    Conditional::Pointer Conditional::parse(const QJsonValue& json) {
        return Conditional::Pointer();
    }

}
