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
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <AvatarData.h>
#include <SharedUtil.h>

class Recording;

/// Stores the different values associated to one recording frame
class RecordingFrame {
public:
    QVector<float> getBlendshapeCoefficients() const { return _blendshapeCoefficients; }
    QVector<glm::quat> getJointRotations() const { return _jointRotations; }
    glm::vec3 getTranslation() const { return _translation; }
    
protected:
    void setBlendshapeCoefficients(QVector<float> blendshapeCoefficients);
    void setJointRotations(QVector<glm::quat> jointRotations);
    void setTranslation(glm::vec3 translation);
    
private:
    QVector<float> _blendshapeCoefficients;
    QVector<glm::quat> _jointRotations;
    glm::vec3 _translation;
    
    friend class Recorder;
    friend void writeRecordingToFile(Recording& recording, QString file);
    friend Recording* readRecordingFromFile(QString file);
};

/// Stores a recording
class Recording {
public:
    bool isEmpty() const { return _timestamps.isEmpty(); }
    int getLength() const { return _timestamps.last(); } // in ms
    
    qint32 getFrameTimestamp(int i) const { return _timestamps[i]; }
    const RecordingFrame& getFrame(int i) const { return _frames[i]; }
    
protected:
    void addFrame(int timestamp, RecordingFrame& frame);
    void clear();
    
private:
    QVector<qint32> _timestamps;
    QVector<RecordingFrame> _frames;
    
    friend class Recorder;
    friend class Player;
    friend void writeRecordingToFile(Recording& recording, QString file);
    friend Recording* readRecordingFromFile(QString file);
};


/// Records a recording
class Recorder {
public:
    Recorder(AvatarData* avatar);
    
    bool isRecording() const;
    qint64 elapsed() const;
    
public slots:
    void startRecording();
    void stopRecording();
    void saveToFile(QString file);
    void record();
    
private:
    QElapsedTimer _timer;
    Recording _recording;

    AvatarData* _avatar;
};

/// Plays back a recording
class Player {
public:
    Player(AvatarData* avatar);
    
    bool isPlaying() const;
    qint64 elapsed() const;
    
public slots:
    void startPlaying();
    void stopPlaying();
    void loadFromFile(QString file);
    void play();
    
private:
    QElapsedTimer _timer;
    Recording _recording;
    
    AvatarData* _avatar;
};

void writeRecordingToFile(Recording& recording, QString file);
Recording& readRecordingFromFile(Recording& recording, QString file);

#endif // hifi_Recorder_h