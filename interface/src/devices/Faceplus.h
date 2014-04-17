//
//  Faceplus.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 4/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Faceplus_h
#define hifi_Faceplus_h

#include <QMultiHash>
#include <QPair>
#include <QVector>

#include "FaceTracker.h"

class FaceplusReader;

/// Interface for Mixamo FacePlus.
class Faceplus : public FaceTracker {
    Q_OBJECT
    
public:
    
    Faceplus();
    virtual ~Faceplus();
    
    void init();
    void reset();

    bool isActive() const { return _active; }
    
    Q_INVOKABLE void setState(const glm::vec3& headTranslation, const glm::quat& headRotation,
        float estimatedEyePitch, float estimatedEyeYaw, const QVector<float>& blendshapeCoefficients);
    
public slots:

    void updateEnabled();
    
private:
    
    void setEnabled(bool enabled);
    
    bool _enabled;
    bool _active;
    
    FaceplusReader* _reader;
};

Q_DECLARE_METATYPE(QVector<float>)

/// The reader object that lives in its own thread.
class FaceplusReader : public QObject {
    Q_OBJECT

public:
    
    virtual ~FaceplusReader();

    Q_INVOKABLE void init();
    Q_INVOKABLE void shutdown();
    Q_INVOKABLE void update();
    Q_INVOKABLE void reset();

private:
    
#ifdef HAVE_FACEPLUS
    QMultiHash<int, QPair<int, float> > _channelIndexMap; 
    QVector<float> _outputVector;
    int _headRotationIndices[3];
    int _leftEyeRotationIndices[2];
    int _rightEyeRotationIndices[2];
    float _referenceX;
    float _referenceY;
    float _referenceScale;
    bool _referenceInitialized;
    QVector<float> _blendshapeCoefficients;
#endif
};

#endif // hifi_Faceplus_h
