//
//  LegacyDisplayPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "SimpleDisplayPlugin.h"
#include "GLCanvas.h"

class LegacyDisplayPlugin : public SimpleDisplayPlugin<GLCanvas> {
    Q_OBJECT
public:
    static const QString NAME;
    virtual const QString & getName();

    virtual void activate();
    virtual void deactivate();

    virtual QSize getDeviceSize() const;
    virtual glm::ivec2 getCanvasSize() const;
    virtual bool hasFocus() const;
    virtual PickRay computePickRay(const glm::vec2 & pos) const;
    virtual bool isMouseOnScreen() const { return true; }
    virtual bool isThrottled() const;

protected:
    virtual void makeCurrent();
    virtual void doneCurrent();
    virtual void swapBuffers();
    virtual void idle();
};
