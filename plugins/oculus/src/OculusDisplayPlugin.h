//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OculusBaseDisplayPlugin.h"

class OculusDisplayPlugin : public OculusBaseDisplayPlugin {
    using Parent = OculusBaseDisplayPlugin;
public:
    OculusDisplayPlugin();
    ~OculusDisplayPlugin();
    const QString getName() const override { return NAME; }

    void init() override;

    QString getPreferredAudioInDevice() const override;
    QString getPreferredAudioOutDevice() const override;
    
    virtual QJsonObject getHardwareStats() const;

protected:
    QThread::Priority getPresentPriority() override { return QThread::TimeCriticalPriority; }

    bool internalActivate() override;
    void hmdPresent() override;
    bool isHmdMounted() const override;
    void customizeContext() override;
    void uncustomizeContext() override;
    void cycleDebugOutput() override;

private:
    static const char* NAME;
    ovrTextureSwapChain _textureSwapChain;
    gpu::FramebufferPointer _outputFramebuffer;
    bool _customized { false };

    std::atomic_int _compositorDroppedFrames;
    std::atomic_int _appDroppedFrames;
};

