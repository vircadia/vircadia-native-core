//
//  Recording.h
//
//
//  Created by Clement on 9/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Recording_h
#define hifi_Recording_h

#include <QString>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

template<class C>
class QSharedPointer;

class AttachmentData;
class Recording;
class RecordingFrame;
class Sound;

typedef QSharedPointer<Recording> RecordingPointer;

/// Stores recordings static data
class RecordingContext {
public:
    quint64 globalTimestamp;
    QString domain;
    glm::vec3 position;
    glm::quat orientation;
    float scale;
    QString headModel;
    QString skeletonModel;
    QString displayName;
    QVector<AttachmentData> attachments;
    
    // This avoids recomputation every frame while recording.
    glm::quat orientationInv;
};

/// Stores a recording
class Recording {
public:
    Recording();
    ~Recording();
    
    bool isEmpty() const { return _timestamps.isEmpty(); }
    int getLength() const; // in ms
    
    RecordingContext& getContext() { return _context; }
    int getFrameNumber() const { return _frames.size(); }
    qint32 getFrameTimestamp(int i) const;
    const RecordingFrame& getFrame(int i) const;
    Sound* getAudio() const { return _audio; }
    
protected:
    void addFrame(int timestamp, RecordingFrame& frame);
    void addAudioPacket(QByteArray byteArray);
    void clear();
    
private:
    RecordingContext _context;
    QVector<qint32> _timestamps;
    QVector<RecordingFrame> _frames;
    
    Sound* _audio;
    
    friend class Recorder;
    friend class Player;
    friend void writeRecordingToFile(RecordingPointer recording, QString file);
    friend RecordingPointer readRecordingFromFile(RecordingPointer recording, QString file);
    friend RecordingPointer readRecordingFromRecFile(RecordingPointer recording, QString filename, QByteArray byteArray);
};

/// Stores the different values associated to one recording frame
class RecordingFrame {
public:
    QVector<float> getBlendshapeCoefficients() const { return _blendshapeCoefficients; }
    QVector<glm::quat> getJointRotations() const { return _jointRotations; }
    glm::vec3 getTranslation() const { return _translation; }
    glm::quat getRotation() const { return _rotation; }
    float getScale() const { return _scale; }
    glm::quat getHeadRotation() const { return _headRotation; }
    float getLeanSideways() const { return _leanSideways; }
    float getLeanForward() const { return _leanForward; }
    glm::vec3 getLookAtPosition() const { return _lookAtPosition; }
    
protected:
    void setBlendshapeCoefficients(QVector<float> blendshapeCoefficients);
    void setJointRotations(QVector<glm::quat> jointRotations) { _jointRotations = jointRotations; }
    void setTranslation(glm::vec3 translation) { _translation = translation; }
    void setRotation(glm::quat rotation) { _rotation = rotation; }
    void setScale(float scale) { _scale = scale; }
    void setHeadRotation(glm::quat headRotation) { _headRotation = headRotation; }
    void setLeanSideways(float leanSideways) { _leanSideways = leanSideways; }
    void setLeanForward(float leanForward) { _leanForward = leanForward; }
    void setLookAtPosition(glm::vec3 lookAtPosition) { _lookAtPosition = lookAtPosition; }
    
private:
    QVector<float> _blendshapeCoefficients;
    QVector<glm::quat> _jointRotations;
    glm::vec3 _translation;
    glm::quat _rotation;
    float _scale;
    glm::quat _headRotation;
    float _leanSideways;
    float _leanForward;
    glm::vec3 _lookAtPosition;
    
    friend class Recorder;
    friend void writeRecordingToFile(RecordingPointer recording, QString file);
    friend RecordingPointer readRecordingFromFile(RecordingPointer recording, QString file);
    friend RecordingPointer readRecordingFromRecFile(RecordingPointer recording, QString filename, QByteArray byteArray);
};

void writeRecordingToFile(RecordingPointer recording, QString filename);
RecordingPointer readRecordingFromFile(RecordingPointer recording, QString filename);
RecordingPointer readRecordingFromRecFile(RecordingPointer recording, QString filename, QByteArray byteArray);

#endif // hifi_Recording_h