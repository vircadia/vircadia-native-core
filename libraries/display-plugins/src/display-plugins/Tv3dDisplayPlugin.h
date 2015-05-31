//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OpenGlDisplayPlugin.h"

class Tv3dDisplayPlugin : public OpenGlDisplayPlugin {
    Q_OBJECT
public:
    Tv3dDisplayPlugin();
    virtual const QString & getName();
    virtual bool isStereo() const final { return true; }
    virtual bool isSupported() const final;

    virtual void activate(PluginContainer * container);

    void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
        GLuint overlayTexture, const glm::uvec2& overlaySize);

    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const;
    virtual glm::mat4 getModelview(Eye eye, const glm::mat4& baseModelview) const;

protected:
    virtual void customizeWindow(PluginContainer * container);

    //virtual std::function<QPointF(const QPointF&)> getMouseTranslator();
    //virtual glm::ivec2 trueMouseToUiMouse(const glm::ivec2 & position) const;
private:
    static const QString NAME;
};
