//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "StereoDisplayPlugin.h"

class InterleavedStereoDisplayPlugin : public StereoDisplayPlugin {
    Q_OBJECT
    using Parent = StereoDisplayPlugin;
public:
    const QString getName() const override { return NAME; }
    grouping getGrouping() const override { return ADVANCED; }
    glm::uvec2 getRecommendedRenderSize() const override;

protected:
    // initialize OpenGL context settings needed by the plugin
    void customizeContext() override;
    void uncustomizeContext() override;
    void internalPresent() override;

private:
    static const QString NAME;
    gpu::PipelinePointer _interleavedPresentPipeline;
    gpu::BufferPointer _textureDataBuffer;
};
