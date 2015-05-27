//
//  Tv3dDisplayPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include "SimpleDisplayPlugin.h"
#include <QTimer>

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
    virtual void activate();
    virtual void deactivate();

    //virtual std::function<QPointF(const QPointF&)> getMouseTranslator();
    //virtual glm::ivec2 trueMouseToUiMouse(const glm::ivec2 & position) const;

private:
    QTimer _timer;
};
