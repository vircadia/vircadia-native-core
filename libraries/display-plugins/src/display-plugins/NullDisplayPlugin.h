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

    virtual ~NullDisplayPlugin() final {}
    virtual const QString& getName() const override { return NAME; }
    virtual grouping getGrouping() const override { return DEVELOPER; }

    void stop() override;

    virtual glm::uvec2 getRecommendedRenderSize() const override;
    virtual bool hasFocus() const override;
    virtual void submitSceneTexture(uint32_t frameIndex, const gpu::TexturePointer& sceneTexture) override;
    virtual void submitOverlayTexture(const gpu::TexturePointer& overlayTexture) override;
    virtual QImage getScreenshot() const override;
private:
    static const QString NAME;
};
