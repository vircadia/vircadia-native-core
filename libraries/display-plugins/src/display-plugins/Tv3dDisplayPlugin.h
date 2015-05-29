//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "GlWindowDisplayPlugin.h"

class Tv3dDisplayPlugin : public GlWindowDisplayPlugin {
    Q_OBJECT
public:
    static const QString NAME;
    virtual const QString & getName();
    Tv3dDisplayPlugin();
    virtual bool isStereo() const final { return true; }
    virtual bool isSupported() const final;
    void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
        GLuint overlayTexture, const glm::uvec2& overlaySize);
    virtual void activate(PluginContainer * container);
    virtual void deactivate();
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const;
    virtual glm::mat4 getModelview(Eye eye, const glm::mat4& baseModelview) const;

protected:
    virtual void customizeWindow();
    virtual void customizeContext();

    //virtual std::function<QPointF(const QPointF&)> getMouseTranslator();
    //virtual glm::ivec2 trueMouseToUiMouse(const glm::ivec2 & position) const;
};
