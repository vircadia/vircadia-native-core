//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "WindowOpenGLDisplayPlugin.h"

const float TARGET_FRAMERATE_Basic2DWindowOpenGL = 60.0f;

class QScreen;
class QAction;

class Basic2DWindowOpenGLDisplayPlugin : public WindowOpenGLDisplayPlugin {
    Q_OBJECT

public:
    virtual const QString& getName() const override { return NAME; }

    virtual float getTargetFrameRate() override { return  _framerateTarget ? (float) _framerateTarget : TARGET_FRAMERATE_Basic2DWindowOpenGL; }

    virtual void activate() override;

    virtual void submitSceneTexture(uint32_t frameIndex, const gpu::TexturePointer& sceneTexture) override;

    virtual void internalPresent() override;

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
    bool _wantVsync { true };
};
