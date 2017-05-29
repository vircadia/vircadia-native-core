//
//  Created by Andrzej Kapolka on 4/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FaceTracker_h
#define hifi_FaceTracker_h

#include <QObject>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <SettingHandle.h>

/// Base class for face trackers (DDE, BinaryVR).
class FaceTracker : public QObject {
    Q_OBJECT
    
public:
    virtual bool isActive() const { return false; }
    virtual bool isTracking() const { return false; }
    
    virtual void init();
    virtual void update(float deltaTime);
    virtual void reset();
    
    float getFadeCoefficient() const;
    
    const glm::vec3 getHeadTranslation() const;
    const glm::quat getHeadRotation() const;
    
    float getEstimatedEyePitch() const { return _estimatedEyePitch; }
    float getEstimatedEyeYaw() const { return _estimatedEyeYaw; } 
    
    int getNumBlendshapes() const { return _blendshapeCoefficients.size(); }
    bool isValidBlendshapeIndex(int index) const { return index >= 0 && index < getNumBlendshapes(); }
    const QVector<float>& getBlendshapeCoefficients() const;
    float getBlendshapeCoefficient(int index) const;

    static bool isMuted() { return _isMuted; }
    static void setIsMuted(bool isMuted) { _isMuted = isMuted; }

    static float getEyeDeflection() { return _eyeDeflection.get(); }
    static void setEyeDeflection(float eyeDeflection);

    static void updateFakeCoefficients(float leftBlink,
                                float rightBlink,
                                float browUp,
                                float jawOpen,
                                float mouth2,
                                float mouth3,
                                float mouth4,
                                QVector<float>& coefficients);

signals:
    void muteToggled();

public slots:
    virtual void setEnabled(bool enabled) = 0;
    void toggleMute();
    bool getMuted() { return _isMuted; }

protected:
    virtual ~FaceTracker() {};

    bool _isInitialized = false;
    static bool _isMuted;

    glm::vec3 _headTranslation = glm::vec3(0.0f);
    glm::quat _headRotation = glm::quat();
    float _estimatedEyePitch = 0.0f;
    float _estimatedEyeYaw = 0.0f;
    QVector<float> _blendshapeCoefficients;
    
    float _relaxationStatus = 0.0f; // Between 0.0f and 1.0f
    float _fadeCoefficient = 0.0f; // Between 0.0f and 1.0f

    void countFrame();

private slots:
    void startFPSTimer();
    void finishFPSTimer();

private:
    bool _isCalculatingFPS = false;
    int _frameCount = 0;

    // see http://support.faceshift.com/support/articles/35129-export-of-blendshapes
    static const int _leftBlinkIndex = 0;
    static const int _rightBlinkIndex = 1;
    static const int _leftEyeOpenIndex = 8;
    static const int _rightEyeOpenIndex = 9;

    // Brows
    static const int _browDownLeftIndex = 14;
    static const int _browDownRightIndex = 15;
    static const int _browUpCenterIndex = 16;
    static const int _browUpLeftIndex = 17;
    static const int _browUpRightIndex = 18;

    static const int _mouthSmileLeftIndex = 28;
    static const int _mouthSmileRightIndex = 29;

    static const int _jawOpenIndex = 21;

    static Setting::Handle<float> _eyeDeflection;
};

#endif // hifi_FaceTracker_h
