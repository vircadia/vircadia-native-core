//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "WindowOpenGLDisplayPlugin.h"

class QScreen;
class Basic2DWindowOpenGLDisplayPlugin : public WindowOpenGLDisplayPlugin {
    Q_OBJECT

public:
    virtual const QString & getName() const override;

    virtual void activate() override;
    virtual void deactivate() override;

    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize) override;

    virtual bool isThrottled() const override;

protected:
    int getDesiredInterval() const;
    mutable bool _isThrottled = false;

private:
    void updateFramerate();
    static const QString NAME;
    QScreen* getFullscreenTarget();
    uint32_t _framerateTarget{ 0 };
    int _fullscreenTarget{ -1 };
};
