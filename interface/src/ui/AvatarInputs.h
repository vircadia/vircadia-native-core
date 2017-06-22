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
    AI_PROPERTY(bool, isHMD, false)

    Q_PROPERTY(bool showAudioTools READ showAudioTools WRITE setShowAudioTools NOTIFY showAudioToolsChanged)

public:
    static AvatarInputs* getInstance();
    Q_INVOKABLE float loudnessToAudioLevel(float loudness);
    AvatarInputs(QQuickItem* parent = nullptr);
    void update();
    bool showAudioTools() const   { return _showAudioTools; }

public slots:
    void setShowAudioTools(bool showAudioTools);

signals:
    void cameraEnabledChanged();
    void cameraMutedChanged();
    void isHMDChanged();
    void showAudioToolsChanged(bool show);

protected:
    Q_INVOKABLE void resetSensors();
    Q_INVOKABLE void toggleCameraMute();

private: 
    float _trailingAudioLoudness{ 0 };
    bool _showAudioTools { false };
};

#endif // hifi_AvatarInputs_h
