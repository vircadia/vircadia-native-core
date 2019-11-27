//
//  Created by Bradley Austin Davis on 2019/09/05
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LocalFileAccessGate.h"

#include <QtCore/QThreadStorage>

static QThreadStorage<bool> localAccessSafe;

void hifi::scripting::setLocalAccessSafeThread(bool safe) {
    localAccessSafe.setLocalData(safe);
}

bool hifi::scripting::isLocalAccessSafeThread() {
    return localAccessSafe.hasLocalData() && localAccessSafe.localData();
}
