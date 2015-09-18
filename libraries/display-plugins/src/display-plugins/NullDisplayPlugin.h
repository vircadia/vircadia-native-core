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
    virtual const QString & getName() const override;

    void activate() override;
    void deactivate() override;
    void stop() override;

    virtual glm::uvec2 getRecommendedRenderSize() const override;
    virtual bool hasFocus() const override;
    virtual void preRender() override;
    virtual void preDisplay() override;
    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize) override;
    virtual void finishFrame() override;

private:
    static const QString NAME;
};
