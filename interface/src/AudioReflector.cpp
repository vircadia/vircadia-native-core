//
//  AudioReflector.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 4/2/2014
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "AudioReflector.h"

void AudioReflector::render() {
    if (!_myAvatar) {
        return; // exit early if not set up correctly
    }

    
    /*
    glm::vec3 position = _myAvatar->getHead()->getPosition();
    const float radius = 0.25f;
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glutWireSphere(radius, 15, 15);
    glPopMatrix();
    */
    
    drawRays();
}


// delay = 1ms per foot
//       = 3ms per meter
// attenuation =
//        BOUNCE_ATTENUATION_FACTOR [0.5] * (1/(1+distance))

int getDelayFromDistance(float distance) {
    const int MS_DELAY_PER_METER = 3;
    return MS_DELAY_PER_METER * distance;
}

const float BOUNCE_ATTENUATION_FACTOR = 0.125f;

float getDistanceAttenuationCoefficient(float distance) {
    const float DISTANCE_SCALE = 2.5f;
    const float GEOMETRIC_AMPLITUDE_SCALAR = 0.3f;
    const float DISTANCE_LOG_BASE = 2.5f;
    const float DISTANCE_SCALE_LOG = logf(DISTANCE_SCALE) / logf(DISTANCE_LOG_BASE);
    
    float distanceSquareToSource = distance * distance;

    // calculate the distance coefficient using the distance to this node
    float distanceCoefficient = powf(GEOMETRIC_AMPLITUDE_SCALAR,
                                     DISTANCE_SCALE_LOG +
                                     (0.5f * logf(distanceSquareToSource) / logf(DISTANCE_LOG_BASE)) - 1);
    distanceCoefficient = std::min(1.0f, distanceCoefficient);
    
    return distanceCoefficient;
}

glm::vec3 getFaceNormal(BoxFace face) {
    if (face == MIN_X_FACE) {
        return glm::vec3(-1, 0, 0);
    } else if (face == MAX_X_FACE) {
        return glm::vec3(1, 0, 0);
    } else if (face == MIN_Y_FACE) {
        return glm::vec3(0, -1, 0);
    } else if (face == MAX_Y_FACE) {
        return glm::vec3(0, 1, 0);
    } else if (face == MIN_Z_FACE) {
        return glm::vec3(0, 0, -1);
    } else if (face == MAX_Z_FACE) {
        return glm::vec3(0, 0, 1);
    }
    return glm::vec3(0, 0, 0); //error case
}

QVector<glm::vec3> AudioReflector::calculateReflections(const glm::vec3& origin, const glm::vec3& originalDirection, int maxBounces) {
    QVector<glm::vec3> reflectionPoints;
    glm::vec3 start = origin;
    glm::vec3 direction = originalDirection;
    OctreeElement* elementHit;
    float distance;
    BoxFace face;
    const float SLIGHTLY_SHORT = 0.999f; // slightly inside the distance so we're on the inside of the reflection point
    
    for (int i = 0; i < maxBounces; i++) {
        if (_voxels->findRayIntersection(start, direction, elementHit, distance, face)) {
            glm::vec3 end = start + (direction * (distance * SLIGHTLY_SHORT));
            
            reflectionPoints.push_back(end);
        
            glm::vec3 faceNormal = getFaceNormal(face);
            direction = glm::normalize(glm::reflect(direction,faceNormal));
            start = end;
        }
    }
    return reflectionPoints;
}


void AudioReflector::newDrawReflections(const glm::vec3& origin, const glm::vec3& originalColor, const QVector<glm::vec3>& reflections) {
                                        
    glm::vec3 start = origin;
    glm::vec3 color = originalColor;
    const float COLOR_ADJUST_PER_BOUNCE = 0.75f;
    
    foreach (glm::vec3 end, reflections) {
        drawVector(start, end, color);
        start = end;
        color = color * COLOR_ADJUST_PER_BOUNCE;
    }
}

void AudioReflector::drawReflections(const glm::vec3& origin, const glm::vec3& originalDirection, 
                                        int bounces, const glm::vec3& originalColor) {
                                        
    glm::vec3 start = origin;
    glm::vec3 direction = originalDirection;
    glm::vec3 color = originalColor;
    OctreeElement* elementHit;
    float distance;
    BoxFace face;
    const float SLIGHTLY_SHORT = 0.999f; // slightly inside the distance so we're on the inside of the reflection point
    const float COLOR_ADJUST_PER_BOUNCE = 0.75f;
    
    for (int i = 0; i < bounces; i++) {
        if (_voxels->findRayIntersection(start, direction, elementHit, distance, face)) {
            glm::vec3 end = start + (direction * (distance * SLIGHTLY_SHORT));
            drawVector(start, end, color);
        
            glm::vec3 faceNormal = getFaceNormal(face);
            direction = glm::normalize(glm::reflect(direction,faceNormal));
            start = end;
            color = color * COLOR_ADJUST_PER_BOUNCE;
        }
    }
}
// set up our buffers for our attenuated and delayed samples
const int NUMBER_OF_CHANNELS = 2;
void AudioReflector::echoReflections(const glm::vec3& origin, const glm::vec3& originalDirection,
                                        int bounces, const QByteArray& originalSamples, 
                                        unsigned int sampleTime, int sampleRate) {
                                        
    int samplesTouched = 0;
                                        
    glm::vec3 rightEarPosition = _myAvatar->getHead()->getRightEarPosition();
    glm::vec3 leftEarPosition = _myAvatar->getHead()->getLeftEarPosition();
    glm::vec3 start = origin;
    glm::vec3 direction = originalDirection;
    OctreeElement* elementHit;
    float distance;
    BoxFace face;
    const float SLIGHTLY_SHORT = 0.999f; // slightly inside the distance so we're on the inside of the reflection point

    int totalNumberOfSamples = originalSamples.size() / sizeof(int16_t);
    int totalNumberOfStereoSamples = originalSamples.size() / (sizeof(int16_t) * NUMBER_OF_CHANNELS);

    const int16_t* originalSamplesData = (const int16_t*)originalSamples.constData();
    QByteArray attenuatedLeftSamples;
    QByteArray attenuatedRightSamples;
    attenuatedLeftSamples.resize(originalSamples.size());
    attenuatedRightSamples.resize(originalSamples.size());

    int16_t* attenuatedLeftSamplesData = (int16_t*)attenuatedLeftSamples.data();
    int16_t* attenuatedRightSamplesData = (int16_t*)attenuatedRightSamples.data();
    
    AudioRingBuffer attenuatedLeftBuffer(totalNumberOfSamples);
    AudioRingBuffer attenuatedRightBuffer(totalNumberOfSamples);
    

    for (int bounceNumber = 1; bounceNumber <= bounces; bounceNumber++) {
        if (_voxels->findRayIntersection(start, direction, elementHit, distance, face)) {
            glm::vec3 end = start + (direction * (distance * SLIGHTLY_SHORT));

            glm::vec3 faceNormal = getFaceNormal(face);
            direction = glm::normalize(glm::reflect(direction,faceNormal));
            start = end;
            
            // calculate the distance to the ears
            float rightEarDistance = glm::distance(end, rightEarPosition);
            float leftEarDistance = glm::distance(end, leftEarPosition);
            int rightEarDelayMsecs = getDelayFromDistance(rightEarDistance);
            int leftEarDelayMsecs = getDelayFromDistance(leftEarDistance);
            int rightEarDelay = rightEarDelayMsecs * sampleRate / MSECS_PER_SECOND;
            int leftEarDelay = leftEarDelayMsecs * sampleRate / MSECS_PER_SECOND;

            float rightEarAttenuation = getDistanceAttenuationCoefficient(rightEarDistance) * (bounceNumber * BOUNCE_ATTENUATION_FACTOR);
            float leftEarAttenuation = getDistanceAttenuationCoefficient(leftEarDistance) * (bounceNumber * BOUNCE_ATTENUATION_FACTOR);
            //qDebug() << "leftEarAttenuation=" << leftEarAttenuation << "rightEarAttenuation=" << rightEarAttenuation;

            // run through the samples, and attenuate them                                                            
            for (int sample = 0; sample < totalNumberOfStereoSamples; sample++) {
                int16_t leftSample = originalSamplesData[sample * NUMBER_OF_CHANNELS];
                int16_t rightSample = originalSamplesData[(sample * NUMBER_OF_CHANNELS) + 1];

                //qDebug() << "leftSample=" << leftSample << "rightSample=" << rightSample;
                
                attenuatedLeftSamplesData[sample * NUMBER_OF_CHANNELS] = leftSample * leftEarAttenuation;
                attenuatedLeftSamplesData[sample * NUMBER_OF_CHANNELS + 1] = 0;

                attenuatedRightSamplesData[sample * NUMBER_OF_CHANNELS] = 0;
                attenuatedRightSamplesData[sample * NUMBER_OF_CHANNELS + 1] = rightSample * rightEarAttenuation;

                //qDebug() << "attenuated... leftSample=" << (leftSample * leftEarAttenuation) << "rightSample=" << (rightSample * rightEarAttenuation);
                
                samplesTouched++;
            }
            
            // now inject the attenuated array with the appropriate delay
            
            unsigned int sampleTimeLeft = sampleTime + leftEarDelay;
            unsigned int sampleTimeRight = sampleTime + rightEarDelay;
            
            //qDebug() << "sampleTimeLeft=" << sampleTimeLeft << "sampleTimeRight=" << sampleTimeRight;

            attenuatedLeftBuffer.writeSamples(attenuatedLeftSamplesData, totalNumberOfSamples);
            attenuatedRightBuffer.writeSamples(attenuatedRightSamplesData, totalNumberOfSamples);
            
            _audio->addSpatialAudioToBuffer(sampleTimeLeft, attenuatedLeftBuffer);
            _audio->addSpatialAudioToBuffer(sampleTimeRight, attenuatedRightBuffer);
            attenuatedLeftBuffer.reset();
            attenuatedRightBuffer.reset();
        }
    }
}

void AudioReflector::processSpatialAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format) {
    bool doNothing = true;
    bool doSimpleEcho = false;

    if (doNothing) {
        return;
    } else if (doSimpleEcho) {
        int totalNumberOfSamples = samples.size() / (sizeof(int16_t));
        AudioRingBuffer samplesRingBuffer(totalNumberOfSamples);
        samplesRingBuffer.writeData(samples.constData(),samples.size());
        _audio->addSpatialAudioToBuffer(sampleTime + 12000, samplesRingBuffer);
        return;
    } else {
        quint64 start = usecTimestampNow();

        glm::vec3 origin = _myAvatar->getHead()->getPosition();

        glm::quat orientation = _myAvatar->getOrientation(); // _myAvatar->getHead()->getOrientation();
        glm::vec3 right = glm::normalize(orientation * IDENTITY_RIGHT);
        glm::vec3 up = glm::normalize(orientation * IDENTITY_UP);
        glm::vec3 front = glm::normalize(orientation * IDENTITY_FRONT);
        glm::vec3 left = -right;
        glm::vec3 down = -up;
        glm::vec3 back = -front;
        glm::vec3 frontRightUp = glm::normalize(front + right + up);
        glm::vec3 frontLeftUp = glm::normalize(front + left + up);
        glm::vec3 backRightUp = glm::normalize(back + right + up);
        glm::vec3 backLeftUp = glm::normalize(back + left + up);
        glm::vec3 frontRightDown = glm::normalize(front + right + down);
        glm::vec3 frontLeftDown = glm::normalize(front + left + down);
        glm::vec3 backRightDown = glm::normalize(back + right + down);
        glm::vec3 backLeftDown = glm::normalize(back + left + down);

        const int BOUNCE_COUNT = 5;

        echoReflections(origin, frontRightUp, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, frontLeftUp, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, backRightUp, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, backLeftUp, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, frontRightDown, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, frontLeftDown, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, backRightDown, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, backLeftDown, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());

        echoReflections(origin, front, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, back, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, left, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, right, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, up, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        echoReflections(origin, down, BOUNCE_COUNT, samples, sampleTime, format.sampleRate());
        quint64 end = usecTimestampNow();

        qDebug() << "AudioReflector::addSamples()... elapsed=" << (end - start);
    }
}

void AudioReflector::drawRays() {
    glm::vec3 origin = _myAvatar->getHead()->getPosition();
    //glm::vec3 origin = _myAvatar->getHead()->getRightEarPosition();

    glm::quat orientation = _myAvatar->getOrientation(); // _myAvatar->getHead()->getOrientation();
    glm::vec3 right = glm::normalize(orientation * IDENTITY_RIGHT);
    glm::vec3 up = glm::normalize(orientation * IDENTITY_UP);
    glm::vec3 front = glm::normalize(orientation * IDENTITY_FRONT);
    glm::vec3 left = -right;
    glm::vec3 down = -up;
    glm::vec3 back = -front;
    glm::vec3 frontRightUp = glm::normalize(front + right + up);
    glm::vec3 frontLeftUp = glm::normalize(front + left + up);
    glm::vec3 backRightUp = glm::normalize(back + right + up);
    glm::vec3 backLeftUp = glm::normalize(back + left + up);
    glm::vec3 frontRightDown = glm::normalize(front + right + down);
    glm::vec3 frontLeftDown = glm::normalize(front + left + down);
    glm::vec3 backRightDown = glm::normalize(back + right + down);
    glm::vec3 backLeftDown = glm::normalize(back + left + down);


    const glm::vec3 RED(1,0,0);
    const glm::vec3 GREEN(0,1,0);
    const glm::vec3 BLUE(0,0,1);
    const glm::vec3 PURPLE(1,0,1);
    const glm::vec3 YELLOW(1,1,0);
    const glm::vec3 CYAN(0,1,1);
    const glm::vec3 DARK_RED(0.8f,0.2f,0.2f);
    const glm::vec3 DARK_GREEN(0.2f,0.8f,0.2f);
    const glm::vec3 DARK_BLUE(0.2f,0.2f,0.8f);
    const glm::vec3 DARK_PURPLE(0.8f,0.2f,0.8f);
    const glm::vec3 DARK_YELLOW(0.8f,0.8f,0.2f);
    const glm::vec3 DARK_CYAN(0.2f,0.8f,0.8f);

    const glm::vec3 WHITE(1,1,1);
    const glm::vec3 GRAY(0.5f,0.5f,0.5f);
    
    const int BOUNCE_COUNT = 5;

    bool oldWay = false;
    
    if (oldWay) {
        drawReflections(origin, frontRightUp, BOUNCE_COUNT, RED);
        drawReflections(origin, frontLeftUp, BOUNCE_COUNT, GREEN);
        drawReflections(origin, backRightUp, BOUNCE_COUNT, BLUE);
        drawReflections(origin, backLeftUp, BOUNCE_COUNT, CYAN);
        drawReflections(origin, frontRightDown, BOUNCE_COUNT, PURPLE);
        drawReflections(origin, frontLeftDown, BOUNCE_COUNT, YELLOW);
        drawReflections(origin, backRightDown, BOUNCE_COUNT, WHITE);
        drawReflections(origin, backLeftDown, BOUNCE_COUNT, DARK_RED);

        drawReflections(origin, front, BOUNCE_COUNT, DARK_GREEN);
        drawReflections(origin, back, BOUNCE_COUNT, DARK_BLUE);
        drawReflections(origin, left, BOUNCE_COUNT, DARK_CYAN);
        drawReflections(origin, right, BOUNCE_COUNT, DARK_PURPLE);
        drawReflections(origin, up, BOUNCE_COUNT, DARK_YELLOW);
        drawReflections(origin, down, BOUNCE_COUNT, GRAY);
    } else {
        QVector<glm::vec3> frontRightUpReflections = calculateReflections(origin, frontRightUp, BOUNCE_COUNT);
        QVector<glm::vec3> frontLeftUpReflections = calculateReflections(origin, frontLeftUp, BOUNCE_COUNT);
        QVector<glm::vec3> backRightUpReflections = calculateReflections(origin, backRightUp, BOUNCE_COUNT);
        QVector<glm::vec3> backLeftUpReflections = calculateReflections(origin, backLeftUp, BOUNCE_COUNT);
        QVector<glm::vec3> frontRightDownReflections = calculateReflections(origin, frontRightDown, BOUNCE_COUNT);
        QVector<glm::vec3> frontLeftDownReflections = calculateReflections(origin, frontLeftDown, BOUNCE_COUNT);
        QVector<glm::vec3> backRightDownReflections = calculateReflections(origin, backRightDown, BOUNCE_COUNT);
        QVector<glm::vec3> backLeftDownReflections = calculateReflections(origin, backLeftDown, BOUNCE_COUNT);
        QVector<glm::vec3> frontReflections = calculateReflections(origin, front, BOUNCE_COUNT);
        QVector<glm::vec3> backReflections = calculateReflections(origin, back, BOUNCE_COUNT);
        QVector<glm::vec3> leftReflections = calculateReflections(origin, left, BOUNCE_COUNT);
        QVector<glm::vec3> rightReflections = calculateReflections(origin, right, BOUNCE_COUNT);
        QVector<glm::vec3> upReflections = calculateReflections(origin, up, BOUNCE_COUNT);
        QVector<glm::vec3> downReflections = calculateReflections(origin, down, BOUNCE_COUNT);

        glm::vec3 frontRightUpColor = RED;
        glm::vec3 frontLeftUpColor = GREEN;
        glm::vec3 backRightUpColor = BLUE;
        glm::vec3 backLeftUpColor = CYAN;
        glm::vec3 frontRightDownColor = PURPLE;
        glm::vec3 frontLeftDownColor = YELLOW;
        glm::vec3 backRightDownColor = WHITE;
        glm::vec3 backLeftDownColor = DARK_RED;
        glm::vec3 frontColor = GRAY;
        glm::vec3 backColor = DARK_GREEN;
        glm::vec3 leftColor = DARK_BLUE;
        glm::vec3 rightColor = DARK_CYAN;
        glm::vec3 upColor = DARK_PURPLE;
        glm::vec3 downColor = DARK_YELLOW;
        
        // attempt to determine insidness/outsideness based on number of directional rays that reflect
        bool inside = false;
        
        bool blockedUp = (frontRightUpReflections.size() > 0) && 
                         (frontLeftUpReflections.size() > 0) && 
                         (backRightUpReflections.size() > 0) && 
                         (backLeftUpReflections.size() > 0) && 
                         (upReflections.size() > 0);

        bool blockedDown = (frontRightDownReflections.size() > 0) && 
                         (frontLeftDownReflections.size() > 0) && 
                         (backRightDownReflections.size() > 0) && 
                         (backLeftDownReflections.size() > 0) && 
                         (downReflections.size() > 0);

        bool blockedFront = (frontRightUpReflections.size() > 0) && 
                         (frontLeftUpReflections.size() > 0) && 
                         (frontRightDownReflections.size() > 0) && 
                         (frontLeftDownReflections.size() > 0) && 
                         (frontReflections.size() > 0);

        bool blockedBack = (backRightUpReflections.size() > 0) && 
                         (backLeftUpReflections.size() > 0) && 
                         (backRightDownReflections.size() > 0) && 
                         (backLeftDownReflections.size() > 0) && 
                         (backReflections.size() > 0);
        
        bool blockedLeft = (frontLeftUpReflections.size() > 0) && 
                         (backLeftUpReflections.size() > 0) && 
                         (frontLeftDownReflections.size() > 0) && 
                         (backLeftDownReflections.size() > 0) && 
                         (leftReflections.size() > 0);

        bool blockedRight = (frontRightUpReflections.size() > 0) && 
                         (backRightUpReflections.size() > 0) && 
                         (frontRightDownReflections.size() > 0) && 
                         (backRightDownReflections.size() > 0) && 
                         (rightReflections.size() > 0);

        inside = blockedUp && blockedDown && blockedFront && blockedBack && blockedLeft && blockedRight;
           
        if (inside) {
            frontRightUpColor = RED;
            frontLeftUpColor = RED;
            backRightUpColor = RED;
            backLeftUpColor = RED;
            frontRightDownColor = RED;
            frontLeftDownColor = RED;
            backRightDownColor = RED;
            backLeftDownColor = RED;
            frontColor = RED;
            backColor = RED;
            leftColor = RED;
            rightColor = RED;
            upColor = RED;
            downColor = RED;
        }

        newDrawReflections(origin, frontRightUpColor, frontRightUpReflections);
        newDrawReflections(origin, frontLeftUpColor, frontLeftUpReflections);
        newDrawReflections(origin, backRightUpColor, backRightUpReflections);
        newDrawReflections(origin, backLeftUpColor, backLeftUpReflections);
        newDrawReflections(origin, frontRightDownColor, frontRightDownReflections);
        newDrawReflections(origin, frontLeftDownColor, frontLeftDownReflections);
        newDrawReflections(origin, backRightDownColor, backRightDownReflections);
        newDrawReflections(origin, backLeftDownColor, backLeftDownReflections);

        newDrawReflections(origin, frontColor, frontReflections);
        newDrawReflections(origin, backColor, backReflections);
        newDrawReflections(origin, leftColor, leftReflections);
        newDrawReflections(origin, rightColor, rightReflections);
        newDrawReflections(origin, upColor, upReflections);
        newDrawReflections(origin, downColor, downReflections);

    }
}

void AudioReflector::drawVector(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
    glDisable(GL_LIGHTING); // ??
    glLineWidth(2.0);

    // Draw the vector itself
    glBegin(GL_LINES);
    glColor3f(color.x,color.y,color.z);
    glVertex3f(start.x, start.y, start.z);
    glVertex3f(end.x, end.y, end.z);
    glEnd();

    glEnable(GL_LIGHTING); // ??
}
