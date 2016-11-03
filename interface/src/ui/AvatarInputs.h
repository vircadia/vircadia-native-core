//
//  Created by Bradley Austin Davis 2015/06/19
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarInputs_h
#define hifi_AvatarInputs_h

#include <QQuickItem>
#include <OffscreenUi.h>

#define AI_PROPERTY(type, name, initialValue) \
    Q_PROPERTY(type name READ name NOTIFY name##Changed) \
public: \
    type name() { return _##name; }; \
private: \
    type _##name{ initialValue };

class AvatarInputs : public QQuickItem {
    Q_OBJECT
    HIFI_QML_DECL

    AI_PROPERTY(bool, cameraEnabled, false)
    AI_PROPERTY(bool, cameraMuted, false)
    AI_PROPERTY(bool, audioMuted, false)
    AI_PROPERTY(bool, audioClipping, false)
    AI_PROPERTY(float, audioLevel, 0)
    AI_PROPERTY(bool, mirrorVisible, false)
    AI_PROPERTY(bool, mirrorZoomed, true)
    AI_PROPERTY(bool, isHMD, false)
    AI_PROPERTY(bool, showAudioTools, true)

public:
    static AvatarInputs* getInstance();
    AvatarInputs(QQuickItem* parent = nullptr);
    void update();

signals:
    void cameraEnabledChanged();
    void cameraMutedChanged();
    void audioMutedChanged();
    void audioClippingChanged();
    void audioLevelChanged();
    void mirrorVisibleChanged();
    void mirrorZoomedChanged();
    void isHMDChanged();
    void showAudioToolsChanged();

protected:
    Q_INVOKABLE void resetSensors();
    Q_INVOKABLE void toggleCameraMute();
    Q_INVOKABLE void toggleAudioMute();
    Q_INVOKABLE void toggleZoom();
    Q_INVOKABLE void closeMirror();

private: 
    float _trailingAudioLoudness{ 0 };
};

#endif // hifi_AvatarInputs_h
