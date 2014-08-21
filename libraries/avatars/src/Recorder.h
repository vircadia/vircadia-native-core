//
//  Recorder.h
//
//
//  Created by Clement on 8/7/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Recorder_h
#define hifi_Recorder_h

#include <QBitArray>
#include <QElapsedTimer>
#include <QHash>
#include <QSharedPointer>
#include <QVector>
#include <QWeakPointer>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <AudioInjector.h>
#include <SharedUtil.h>
#include <Sound.h>

class AvatarData;
class Recorder;
class Recording;
class Player;

typedef QSharedPointer<Recording> RecordingPointer;
typedef QSharedPointer<Recorder> RecorderPointer;
typedef QWeakPointer<Recorder> WeakRecorderPointer;
typedef QSharedPointer<Player> PlayerPointer;
typedef QWeakPointer<Player> WeakPlayerPointer;

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
    
protected:
    void setBlendshapeCoefficients(QVector<float> blendshapeCoefficients);
    void setJointRotations(QVector<glm::quat> jointRotations);
    void setTranslation(glm::vec3 translation);
    void setRotation(glm::quat rotation);
    void setScale(float scale);
    void setHeadRotation(glm::quat headRotation);
    void setLeanSideways(float leanSideways);
    void setLeanForward(float leanForward);
    
private:
    QVector<float> _blendshapeCoefficients;
    QVector<glm::quat> _jointRotations;
    glm::vec3 _translation;
    glm::quat _rotation;
    float _scale;
    glm::quat _headRotation;
    float _leanSideways;
    float _leanForward;
    
    friend class Recorder;
    friend void writeRecordingToFile(RecordingPointer recording, QString file);
    friend RecordingPointer readRecordingFromFile(RecordingPointer recording, QString file);
};

/// Stores a recording
class Recording {
public:
    Recording();
    ~Recording();
    
    bool isEmpty() const { return _timestamps.isEmpty(); }
    int getLength() const { return _timestamps.last(); } // in ms

    int getFrameNumber() const { return _frames.size(); }
    qint32 getFrameTimestamp(int i) const { return _timestamps[i]; }
    const RecordingFrame& getFrame(int i) const { return _frames[i]; }
    Sound* getAudio() const { return _audio; }
    
protected:
    void addFrame(int timestamp, RecordingFrame& frame);
    void addAudioPacket(QByteArray byteArray);
    void clear();
    
private:
    QVector<qint32> _timestamps;
    QVector<RecordingFrame> _frames;
    
    Sound* _audio;
    
    friend class Recorder;
    friend class Player;
    friend void writeRecordingToFile(RecordingPointer recording, QString file);
    friend RecordingPointer readRecordingFromFile(RecordingPointer recording, QString file);
};

/// Records a recording
class Recorder {
public:
    Recorder(AvatarData* avatar);
    
    bool isRecording() const;
    qint64 elapsed() const;
    
    RecordingPointer getRecording() const { return _recording; }
    
public slots:
    void startRecording();
    void stopRecording();
    void saveToFile(QString file);
    void record();
    void record(char* samples, int size);
    
private:
    QElapsedTimer _timer;
    RecordingPointer _recording;

    AvatarData* _avatar;
};

/// Plays back a recording
class Player {
public:
    Player(AvatarData* avatar);
    
    bool isPlaying() const;
    qint64 elapsed() const;
    
    RecordingPointer getRecording() const { return _recording; }
    
    // Those should only be called if isPlaying() returns true
    glm::quat getHeadRotation();
    float getLeanSideways();
    float getLeanForward();
    
    
public slots:
    void startPlaying();
    void stopPlaying();
    void loadFromFile(QString file);
    void loadRecording(RecordingPointer recording);
    void play();
    
private:
    bool computeCurrentFrame();
    
    QElapsedTimer _timer;
    RecordingPointer _recording;
    int _currentFrame;
    
    QSharedPointer<AudioInjector> _injector;
    AudioInjectorOptions _options;
    
    AvatarData* _avatar;
    QThread* _audioThread;
};

void writeRecordingToFile(RecordingPointer recording, QString file);
RecordingPointer readRecordingFromFile(RecordingPointer recording, QString file);

#endif // hifi_Recorder_h