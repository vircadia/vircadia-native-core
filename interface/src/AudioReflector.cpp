//
//  AudioReflector.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 4/2/2014
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <QMutexLocker>

#include "AudioReflector.h"
#include "Menu.h"


const float DEFAULT_PRE_DELAY = 20.0f; // this delay in msecs will always be added to all reflections
const float DEFAULT_MS_DELAY_PER_METER = 3.0f;
const float MINIMUM_ATTENUATION_TO_REFLECT = 1.0f / 256.0f;
const float DEFAULT_DISTANCE_SCALING_FACTOR = 2.0f; 


AudioReflector::AudioReflector(QObject* parent) : 
    QObject(parent),
    _preDelay(DEFAULT_PRE_DELAY),
    _soundMsPerMeter(DEFAULT_MS_DELAY_PER_METER),
    _distanceAttenuationScalingFactor(DEFAULT_DISTANCE_SCALING_FACTOR)
{
    reset();
}


void AudioReflector::render() {
    if (!_myAvatar) {
        return; // exit early if not set up correctly
    }

    if (_audio->getProcessSpatialAudio()) {
        drawRays();
    }
}



// delay = 1ms per foot
//       = 3ms per meter
// attenuation =
//        BOUNCE_ATTENUATION_FACTOR [0.5] * (1/(1+distance))

float AudioReflector::getDelayFromDistance(float distance) {
    float delay = (_soundMsPerMeter * distance);

    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingPreDelay)) {
        delay += _preDelay;
    }
    
    
    return delay;
}

// **option 1**: this is what we're using
const float PER_BOUNCE_ATTENUATION_FACTOR = 0.5f; 

// **option 2**: we're not using these
//const float BOUNCE_ATTENUATION_FACTOR = 0.125f;
// each bounce we adjust our attenuation by this factor, the result is an asymptotically decreasing attenuation...
// 0.125, 0.25, 0.5, ...
//const float PER_BOUNCE_ATTENUATION_ADJUSTMENT = 2.0f;
// we don't grow larger than this, which means by the 4th bounce we don't get that much less quiet
//const float MAX_BOUNCE_ATTENUATION = 0.99f;

float AudioReflector::getDistanceAttenuationCoefficient(float distance) {
    const float DISTANCE_SCALE = 2.5f;
    const float GEOMETRIC_AMPLITUDE_SCALAR = 0.3f;
    const float DISTANCE_LOG_BASE = 2.5f;
    const float DISTANCE_SCALE_LOG = logf(DISTANCE_SCALE) / logf(DISTANCE_LOG_BASE);
    
    float distanceSquareToSource = distance * distance;

    // calculate the distance coefficient using the distance to this node
    float distanceCoefficient = powf(GEOMETRIC_AMPLITUDE_SCALAR,
                                     DISTANCE_SCALE_LOG +
                                     (0.5f * logf(distanceSquareToSource) / logf(DISTANCE_LOG_BASE)) - 1);

    distanceCoefficient = std::min(1.0f, distanceCoefficient * getDistanceAttenuationScalingFactor());
    
    return distanceCoefficient;
}

float getBounceAttenuationCoefficient(int bounceCount) {
    return PER_BOUNCE_ATTENUATION_FACTOR * bounceCount;
}

glm::vec3 getFaceNormal(BoxFace face) {
    float surfaceRandomness = randFloatInRange(0.99,1.0);
    float surfaceRemainder = (1.0f - surfaceRandomness)/2.0f;
    if (face == MIN_X_FACE) {
        return glm::vec3(-surfaceRandomness, surfaceRemainder, surfaceRemainder);
    } else if (face == MAX_X_FACE) {
        return glm::vec3(surfaceRandomness, surfaceRemainder, surfaceRemainder);
    } else if (face == MIN_Y_FACE) {
        return glm::vec3(surfaceRemainder, -surfaceRandomness, surfaceRemainder);
    } else if (face == MAX_Y_FACE) {
        return glm::vec3(surfaceRemainder, surfaceRandomness, surfaceRemainder);
    } else if (face == MIN_Z_FACE) {
        return glm::vec3(surfaceRemainder, surfaceRemainder, -surfaceRandomness);
    } else if (face == MAX_Z_FACE) {
        return glm::vec3(surfaceRemainder, surfaceRemainder, surfaceRandomness);
    }
    return glm::vec3(0, 0, 0); //error case
}

void AudioReflector::reset() {
    _reflections = 0;
    _averageAttenuation = 0.0f;
    _maxAttenuation = 0.0f;
    _minAttenuation = 0.0f;
    _averageDelay = 0;
    _maxDelay = 0;
    _minDelay = 0;

    _reflections = _frontRightUpReflections.size() +
        _frontLeftUpReflections.size() +
        _backRightUpReflections.size() +
        _backLeftUpReflections.size() +
        _frontRightDownReflections.size() +
        _frontLeftDownReflections.size() +
        _backRightDownReflections.size() +
        _backLeftDownReflections.size() +
        _frontReflections.size() +
        _backReflections.size() +
        _leftReflections.size() +
        _rightReflections.size() +
        _upReflections.size() +
        _downReflections.size();

}

void AudioReflector::calculateAllReflections() {

    // only recalculate when we've moved...
    // TODO: what about case where new voxels are added in front of us???
    bool wantHeadOrientation = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingHeadOriented);
    glm::quat orientation = wantHeadOrientation ? _myAvatar->getHead()->getFinalOrientation() : _myAvatar->getOrientation();
    
    bool shouldRecalc = _reflections == 0 || _myAvatar->getHead()->getPosition() != _origin || (orientation != _orientation);

    /*
    qDebug() << "wantHeadOrientation=" << wantHeadOrientation;
    
    qDebug("   _myAvatar->getHead()->getPosition()=%f,%f,%f",
                    _myAvatar->getHead()->getPosition().x,
                    _myAvatar->getHead()->getPosition().y,
                    _myAvatar->getHead()->getPosition().z);

    qDebug("   _origin=%f,%f,%f",
                    _origin.x,
                    _origin.y,
                    _origin.z);

    qDebug("   orientation=%f,%f,%f,%f",
                    orientation.x,
                    orientation.y,
                    orientation.z,
                    orientation.w);

    qDebug("   _orientation=%f,%f,%f,%f",
                    _orientation.x,
                    _orientation.y,
                    _orientation.z,
                    _orientation.w);
    */
    if (shouldRecalc) {
        //qDebug() << "origin or orientation has changed...";

        QMutexLocker locker(&_mutex);



        quint64 start = usecTimestampNow();        

        _origin = _myAvatar->getHead()->getPosition();
        glm::vec3 averageEarPosition = _myAvatar->getHead()->getPosition();

        _orientation = orientation;
        glm::vec3 right = glm::normalize(_orientation * IDENTITY_RIGHT);
        glm::vec3 up = glm::normalize(_orientation * IDENTITY_UP);
        glm::vec3 front = glm::normalize(_orientation * IDENTITY_FRONT);
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

        _frontRightUpReflections = calculateReflections(averageEarPosition, _origin, frontRightUp);
        _frontLeftUpReflections = calculateReflections(averageEarPosition, _origin, frontLeftUp);
        _backRightUpReflections = calculateReflections(averageEarPosition, _origin, backRightUp);
        _backLeftUpReflections = calculateReflections(averageEarPosition, _origin, backLeftUp);
        _frontRightDownReflections = calculateReflections(averageEarPosition, _origin, frontRightDown);
        _frontLeftDownReflections = calculateReflections(averageEarPosition, _origin, frontLeftDown);
        _backRightDownReflections = calculateReflections(averageEarPosition, _origin, backRightDown);
        _backLeftDownReflections = calculateReflections(averageEarPosition, _origin, backLeftDown);
        _frontReflections = calculateReflections(averageEarPosition, _origin, front);
        _backReflections = calculateReflections(averageEarPosition, _origin, back);
        _leftReflections = calculateReflections(averageEarPosition, _origin, left);
        _rightReflections = calculateReflections(averageEarPosition, _origin, right);
        _upReflections = calculateReflections(averageEarPosition, _origin, up);
        _downReflections = calculateReflections(averageEarPosition, _origin, down);

        quint64 end = usecTimestampNow();

        reset();

        //qDebug() << "Reflections recalculated in " << (end - start) << "usecs";
        
    }
}

QVector<glm::vec3> AudioReflector::calculateReflections(const glm::vec3& earPosition, const glm::vec3& origin, const glm::vec3& originalDirection) {
    QVector<glm::vec3> reflectionPoints;
    glm::vec3 start = origin;
    glm::vec3 direction = originalDirection;
    OctreeElement* elementHit;
    float distance;
    BoxFace face;
    const float SLIGHTLY_SHORT = 0.999f; // slightly inside the distance so we're on the inside of the reflection point
    float currentAttenuation = 1.0f;
    float totalDistance = 0.0f;
    int bounceCount = 1;
    
    while (currentAttenuation > MINIMUM_ATTENUATION_TO_REFLECT) {
        if (_voxels->findRayIntersection(start, direction, elementHit, distance, face)) {
            glm::vec3 end = start + (direction * (distance * SLIGHTLY_SHORT));

            totalDistance += glm::distance(start, end);
            float earDistance = glm::distance(end, earPosition);
            float totalDistance = earDistance + distance;
            currentAttenuation = getDistanceAttenuationCoefficient(totalDistance) * getBounceAttenuationCoefficient(bounceCount);
            
            if (currentAttenuation > MINIMUM_ATTENUATION_TO_REFLECT) {
                reflectionPoints.push_back(end);
                glm::vec3 faceNormal = getFaceNormal(face);
                direction = glm::normalize(glm::reflect(direction,faceNormal));
                start = end;
                bounceCount++;
            }
        } else {
            currentAttenuation = 0.0f;
        }
    }
    return reflectionPoints;
}


void AudioReflector::drawReflections(const glm::vec3& origin, const glm::vec3& originalColor, const QVector<glm::vec3>& reflections) {
                                        
    glm::vec3 start = origin;
    glm::vec3 color = originalColor;
    const float COLOR_ADJUST_PER_BOUNCE = 0.75f;
    
    foreach (glm::vec3 end, reflections) {
        drawVector(start, end, color);
        start = end;
        color = color * COLOR_ADJUST_PER_BOUNCE;
    }
}

// set up our buffers for our attenuated and delayed samples
const int NUMBER_OF_CHANNELS = 2;


void AudioReflector::echoReflections(const glm::vec3& origin, const QVector<glm::vec3>& reflections, const QByteArray& samples, 
                                    unsigned int sampleTime, int sampleRate) {

    bool wantEarSeparation = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingSeparateEars);
    bool wantStereo = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingStereoSource);
    glm::vec3 rightEarPosition = wantEarSeparation ? _myAvatar->getHead()->getRightEarPosition() : 
                                    _myAvatar->getHead()->getPosition();
    glm::vec3 leftEarPosition = wantEarSeparation ? _myAvatar->getHead()->getLeftEarPosition() :
                                    _myAvatar->getHead()->getPosition();
    glm::vec3 start = origin;

    int totalNumberOfSamples = samples.size() / sizeof(int16_t);
    int totalNumberOfStereoSamples = samples.size() / (sizeof(int16_t) * NUMBER_OF_CHANNELS);

    const int16_t* originalSamplesData = (const int16_t*)samples.constData();
    QByteArray attenuatedLeftSamples;
    QByteArray attenuatedRightSamples;
    attenuatedLeftSamples.resize(samples.size());
    attenuatedRightSamples.resize(samples.size());

    int16_t* attenuatedLeftSamplesData = (int16_t*)attenuatedLeftSamples.data();
    int16_t* attenuatedRightSamplesData = (int16_t*)attenuatedRightSamples.data();
    
    float rightDistance = 0;
    float leftDistance = 0;
    int bounceCount = 0;

    foreach (glm::vec3 end, reflections) {
        bounceCount++;

        rightDistance += glm::distance(start, end);
        leftDistance += glm::distance(start, end);

        // calculate the distance to the ears
        float rightEarDistance = glm::distance(end, rightEarPosition);
        float leftEarDistance = glm::distance(end, leftEarPosition);

        float rightTotalDistance = rightEarDistance + rightDistance;
        float leftTotalDistance = leftEarDistance + leftDistance;
        
        float rightEarDelayMsecs = getDelayFromDistance(rightTotalDistance);
        float leftEarDelayMsecs = getDelayFromDistance(leftTotalDistance);
        
        _totalDelay += rightEarDelayMsecs + leftEarDelayMsecs;
        _delayCount += 2;
        _maxDelay = std::max(_maxDelay,rightEarDelayMsecs);
        _maxDelay = std::max(_maxDelay,leftEarDelayMsecs);
        _minDelay = std::min(_minDelay,rightEarDelayMsecs);
        _minDelay = std::min(_minDelay,leftEarDelayMsecs);
        
        int rightEarDelay = rightEarDelayMsecs * sampleRate / MSECS_PER_SECOND;
        int leftEarDelay = leftEarDelayMsecs * sampleRate / MSECS_PER_SECOND;

        //qDebug() << "leftTotalDistance=" << leftTotalDistance << "rightTotalDistance=" << rightTotalDistance;
        //qDebug() << "leftEarDelay=" << leftEarDelay << "rightEarDelay=" << rightEarDelay;

        float bounceAttenuation = getBounceAttenuationCoefficient(bounceCount);
        float rightEarAttenuation = getDistanceAttenuationCoefficient(rightTotalDistance) * bounceAttenuation;
        float leftEarAttenuation = getDistanceAttenuationCoefficient(leftTotalDistance) * bounceAttenuation;

        _totalAttenuation += rightEarAttenuation + leftEarAttenuation;
        _attenuationCount += 2;
        _maxAttenuation = std::max(_maxAttenuation,rightEarAttenuation);
        _maxAttenuation = std::max(_maxAttenuation,leftEarAttenuation);
        _minAttenuation = std::min(_minAttenuation,rightEarAttenuation);
        _minAttenuation = std::min(_minAttenuation,leftEarAttenuation);

        // run through the samples, and attenuate them                                                            
        for (int sample = 0; sample < totalNumberOfStereoSamples; sample++) {
            int16_t leftSample = originalSamplesData[sample * NUMBER_OF_CHANNELS];
            int16_t rightSample = leftSample;
            if (wantStereo) {
                rightSample = originalSamplesData[(sample * NUMBER_OF_CHANNELS) + 1];
            }

            //qDebug() << "leftSample=" << leftSample << "rightSample=" << rightSample;
            attenuatedLeftSamplesData[sample * NUMBER_OF_CHANNELS] = leftSample * leftEarAttenuation;
            attenuatedLeftSamplesData[sample * NUMBER_OF_CHANNELS + 1] = 0;

            attenuatedRightSamplesData[sample * NUMBER_OF_CHANNELS] = 0;
            attenuatedRightSamplesData[sample * NUMBER_OF_CHANNELS + 1] = rightSample * rightEarAttenuation;

            //qDebug() << "attenuated... leftSample=" << (leftSample * leftEarAttenuation) << "rightSample=" << (rightSample * rightEarAttenuation);
        }
        
        // now inject the attenuated array with the appropriate delay
        
        unsigned int sampleTimeLeft = sampleTime + leftEarDelay;
        unsigned int sampleTimeRight = sampleTime + rightEarDelay;
        
        //qDebug() << "sampleTimeLeft=" << sampleTimeLeft << "sampleTimeRight=" << sampleTimeRight;

        _audio->addSpatialAudioToBuffer(sampleTimeLeft, attenuatedLeftSamples, totalNumberOfSamples);
        _audio->addSpatialAudioToBuffer(sampleTimeRight, attenuatedRightSamples, totalNumberOfSamples);


        start = end;
    }
}

void AudioReflector::processSpatialAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format) {
    //quint64 start = usecTimestampNow();

    _maxDelay = 0;
    _maxAttenuation = 0.0f;
    _minDelay = std::numeric_limits<int>::max();
    _minAttenuation = std::numeric_limits<float>::max();
    _totalDelay = 0.0f;
    _delayCount = 0;
    _totalAttenuation = 0.0f;
    _attenuationCount = 0;

    QMutexLocker locker(&_mutex);

    echoReflections(_origin, _frontRightUpReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _frontLeftUpReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _backRightUpReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _backLeftUpReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _frontRightDownReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _frontLeftDownReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _backRightDownReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _backLeftDownReflections, samples, sampleTime, format.sampleRate());

    echoReflections(_origin, _frontReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _backReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _leftReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _rightReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _upReflections, samples, sampleTime, format.sampleRate());
    echoReflections(_origin, _downReflections, samples, sampleTime, format.sampleRate());


    _averageDelay = _delayCount == 0 ? 0 : _totalDelay / _delayCount;
    _averageAttenuation = _attenuationCount == 0 ? 0 : _totalAttenuation / _attenuationCount;

    //quint64 end = usecTimestampNow();
    //qDebug() << "AudioReflector::addSamples()... elapsed=" << (end - start);
}

void AudioReflector::drawRays() {
    //qDebug() << "AudioReflector::drawRays()";

    calculateAllReflections();

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
    
    bool blockedUp = (_frontRightUpReflections.size() > 0) && 
                     (_frontLeftUpReflections.size() > 0) && 
                     (_backRightUpReflections.size() > 0) && 
                     (_backLeftUpReflections.size() > 0) && 
                     (_upReflections.size() > 0);

    bool blockedDown = (_frontRightDownReflections.size() > 0) && 
                     (_frontLeftDownReflections.size() > 0) && 
                     (_backRightDownReflections.size() > 0) && 
                     (_backLeftDownReflections.size() > 0) && 
                     (_downReflections.size() > 0);

    bool blockedFront = (_frontRightUpReflections.size() > 0) && 
                     (_frontLeftUpReflections.size() > 0) && 
                     (_frontRightDownReflections.size() > 0) && 
                     (_frontLeftDownReflections.size() > 0) && 
                     (_frontReflections.size() > 0);

    bool blockedBack = (_backRightUpReflections.size() > 0) && 
                     (_backLeftUpReflections.size() > 0) && 
                     (_backRightDownReflections.size() > 0) && 
                     (_backLeftDownReflections.size() > 0) && 
                     (_backReflections.size() > 0);
    
    bool blockedLeft = (_frontLeftUpReflections.size() > 0) && 
                     (_backLeftUpReflections.size() > 0) && 
                     (_frontLeftDownReflections.size() > 0) && 
                     (_backLeftDownReflections.size() > 0) && 
                     (_leftReflections.size() > 0);

    bool blockedRight = (_frontRightUpReflections.size() > 0) && 
                     (_backRightUpReflections.size() > 0) && 
                     (_frontRightDownReflections.size() > 0) && 
                     (_backRightDownReflections.size() > 0) && 
                     (_rightReflections.size() > 0);

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

    QMutexLocker locker(&_mutex);

    drawReflections(_origin, frontRightUpColor, _frontRightUpReflections);
    drawReflections(_origin, frontLeftUpColor, _frontLeftUpReflections);
    drawReflections(_origin, backRightUpColor, _backRightUpReflections);
    drawReflections(_origin, backLeftUpColor, _backLeftUpReflections);
    drawReflections(_origin, frontRightDownColor, _frontRightDownReflections);
    drawReflections(_origin, frontLeftDownColor, _frontLeftDownReflections);
    drawReflections(_origin, backRightDownColor, _backRightDownReflections);
    drawReflections(_origin, backLeftDownColor, _backLeftDownReflections);
    drawReflections(_origin, frontColor, _frontReflections);
    drawReflections(_origin, backColor, _backReflections);
    drawReflections(_origin, leftColor, _leftReflections);
    drawReflections(_origin, rightColor, _rightReflections);
    drawReflections(_origin, upColor, _upReflections);
    drawReflections(_origin, downColor, _downReflections);

    /*
    qDebug() << "_reflections:" << _reflections
            << "_averageDelay:" << _averageDelay
            << "_maxDelay:" << _maxDelay
            << "_minDelay:" << _minDelay;

    qDebug() << "_averageAttenuation:" << _averageAttenuation
            << "_maxAttenuation:" << _maxAttenuation
            << "_minAttenuation:" << _minAttenuation;
    */
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
