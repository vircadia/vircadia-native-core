//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OpenGLDisplayPlugin.h"

const float TARGET_FRAMERATE_Basic2DWindowOpenGL = 60.0f;

class QScreen;
class QAction;

class Basic2DWindowOpenGLDisplayPlugin : public OpenGLDisplayPlugin {
    Q_OBJECT
    using Parent = OpenGLDisplayPlugin;
public:
    virtual const QString getName() const override { return NAME; }

    virtual float getTargetFrameRate() const override { return  _framerateTarget ? (float) _framerateTarget : TARGET_FRAMERATE_Basic2DWindowOpenGL; }

    virtual bool internalActivate() override;

    virtual bool isThrottled() const override;

protected:
    mutable bool _isThrottled = false;

private:
    static const QString NAME;
    QScreen* getFullscreenTarget();
    std::vector<QAction*> _framerateActions;
    QAction* _vsyncAction { nullptr };
    uint32_t _framerateTarget { 0 };
    int _fullscreenTarget{ -1 };
};
