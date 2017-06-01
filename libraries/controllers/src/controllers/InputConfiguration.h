//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InputConfiguration_h
#define hifi_InputConfiguration_h

#include <mutex>

#include <DependencyManager.h>
#include <RegisteredMetaTypes.h>

namespace controller {

    class InputConfiguration : public QObject, public Dependency {
    public:
        InputConfiguration();
        ~InputConfiguration() {}
    };
}

#endif
