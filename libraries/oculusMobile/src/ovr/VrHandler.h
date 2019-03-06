//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <glm/glm.hpp>
#include <jni.h>
#include <VrApi_Types.h>

#include "TaskQueue.h"

typedef struct ovrMobile ovrMobile;
namespace ovr {

class VrHandler {
public:
    using HandlerTask = std::function<void(VrHandler*)>;
    using OvrMobileTask = std::function<void(ovrMobile*)>;
    using OvrJavaTask = std::function<void(const ovrJava*)>;
    static void setHandler(VrHandler* handler, bool noError = false);
    static bool withOvrMobile(const OvrMobileTask& task);

protected:
    static void initVr(const char* appId = nullptr);
    static void shutdownVr();
    static bool withOvrJava(const OvrJavaTask& task);

    uint32_t currentPresentIndex() const;
    ovrTracking2 beginFrame();
    void presentFrame(uint32_t textureId, const glm::uvec2& size, const ovrTracking2& tracking) const;

    bool vrActive() const;
    void pollTask();
    void makeCurrent();
    void doneCurrent();
};

}





