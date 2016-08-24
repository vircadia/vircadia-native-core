//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "DisplayPlugin.h"

class NullDisplayPlugin : public DisplayPlugin {
public:
    ~NullDisplayPlugin() final {}
    const QString& getName() const override { return NAME; }
    grouping getGrouping() const override { return DEVELOPER; }

    glm::uvec2 getRecommendedRenderSize() const override;
    bool hasFocus() const override;
    void submitFrame(const gpu::FramePointer& newFrame) override;
    QImage getScreenshot() const override;
private:
    static const QString NAME;
};
