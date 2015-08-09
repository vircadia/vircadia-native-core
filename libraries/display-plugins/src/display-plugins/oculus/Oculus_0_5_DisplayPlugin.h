//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OculusBaseDisplayPlugin.h"

#include <QTimer>

class Oculus_0_5_DisplayPlugin : public OculusBaseDisplayPlugin {
public:
    virtual bool isSupported() const override;
    virtual const QString & getName() const override;

    virtual void activate() override;
    virtual void deactivate() override;

    virtual bool eventFilter(QObject* receiver, QEvent* event) override;

    virtual int getHmdScreen() const override;

protected:
    virtual void preRender() override;
    virtual void preDisplay() override;
    virtual void display(GLuint finalTexture, const glm::uvec2& sceneSize) override;
    // Do not perform swap in finish
    virtual void finishFrame() override;

private:
    static const QString NAME;
};


