//
//  Created by Bradley Austin Davis on 2019/08/22
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "../PlatformHelper.h"

#if defined(Q_OS_ANDROID)

// FIXME support sleep/wake notifications
class AndroidHelper : public PlatformHelper {
public:
    AndroidHelper() {
    }

    ~AndroidHelper() {
    }
};

void PlatformHelper::setup() {
    DependencyManager::set<PlatformHelper, AndroidHelper>();
}

#endif
