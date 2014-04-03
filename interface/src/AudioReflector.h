//
//  AudioReflector.h
//  interface
//
//  Created by Brad Hefta-Gaub on 4/2/2014
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AudioReflector__
#define __interface__AudioReflector__

#include <VoxelTree.h>

#include "Audio.h"
#include "avatar/MyAvatar.h"

class AudioReflector : public QObject {
    Q_OBJECT
public:
    AudioReflector(QObject* parent = 0) : QObject(parent) { };

    void setVoxels(VoxelTree* voxels) { _voxels = voxels; }
    void setMyAvatar(MyAvatar* myAvatar) { _myAvatar = myAvatar; }
    void setAudio(Audio* audio) { _audio = audio; }

    void render();
    
public slots:
    void processSpatialAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);
    
signals:
    
private:
    VoxelTree* _voxels; // used to access voxel scene
    MyAvatar* _myAvatar; // access to listener
    Audio* _audio; // access to audio API

    void drawRays();
    void drawVector(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
    void drawReflections(const glm::vec3& origin, const glm::vec3& direction, int bounces, const glm::vec3& color);
    void echoReflections(const glm::vec3& origin, const glm::vec3& originalDirection, 
                                        int bounces, const QByteArray& samples, 
                                        unsigned int sampleTime, int sampleRate);

    QVector<glm::vec3> calculateReflections(const glm::vec3& origin, const glm::vec3& originalDirection, int maxBounces);
    void newDrawReflections(const glm::vec3& origin, const glm::vec3& originalColor, const QVector<glm::vec3>& reflections);
};


#endif /* defined(__interface__AudioReflector__) */
