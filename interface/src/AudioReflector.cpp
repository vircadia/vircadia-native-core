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
const float MAXIMUM_DELAY_MS = 1000.0 * 20.0f; // stop reflecting after path is this long
const int DEFAULT_DIFFUSION_FANOUT = 5;
const int ABSOLUTE_MAXIMUM_BOUNCE_COUNT = 10;

const float SLIGHTLY_SHORT = 0.999f; // slightly inside the distance so we're on the inside of the reflection point

const float DEFAULT_ABSORPTION_RATIO = 0.125; // 12.5% is absorbed
const float DEFAULT_DIFFUSION_RATIO = 0.125; // 12.5% is diffused

AudioReflector::AudioReflector(QObject* parent) : 
    QObject(parent),
    _preDelay(DEFAULT_PRE_DELAY),
    _soundMsPerMeter(DEFAULT_MS_DELAY_PER_METER),
    _distanceAttenuationScalingFactor(DEFAULT_DISTANCE_SCALING_FACTOR),
    _diffusionFanout(DEFAULT_DIFFUSION_FANOUT),
    _absorptionRatio(DEFAULT_ABSORPTION_RATIO),
    _diffusionRatio(DEFAULT_DIFFUSION_RATIO)
{
    reset();
}


void AudioReflector::render() {
    if (!_myAvatar) {
        return; // exit early if not set up correctly
    }

    if (_audio->getProcessSpatialAudio()) {
        if (Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingWithDiffusions)) {
            newDrawRays();
        } else {
            drawRays();
        }
    }
}



// delay = 1ms per foot
//       = 3ms per meter
// attenuation =
//        BOUNCE_ATTENUATION_FACTOR [0.5] * (1/(1+distance))

float AudioReflector::getDelayFromDistance(float distance) {
    float delay = (_soundMsPerMeter * distance);

    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingPreDelay) &&
        !Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingWithDiffusions)) {

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
    glm::vec3 slightlyRandomFaceNormal;

    float surfaceRandomness = randFloatInRange(0.99f,1.0f);
    float surfaceRemainder = (1.0f - surfaceRandomness)/2.0f;
    float altRemainderSignA = (randFloatInRange(-1.0f,1.0f) < 0.0f) ? -1.0 : 1.0;
    float altRemainderSignB = (randFloatInRange(-1.0f,1.0f) < 0.0f) ? -1.0 : 1.0;

    if (face == MIN_X_FACE) {
        slightlyRandomFaceNormal = glm::vec3(-surfaceRandomness, surfaceRemainder * altRemainderSignA, surfaceRemainder * altRemainderSignB);
    } else if (face == MAX_X_FACE) {
        slightlyRandomFaceNormal = glm::vec3(surfaceRandomness, surfaceRemainder * altRemainderSignA, surfaceRemainder * altRemainderSignB);
    } else if (face == MIN_Y_FACE) {
        slightlyRandomFaceNormal = glm::vec3(surfaceRemainder * altRemainderSignA, -surfaceRandomness, surfaceRemainder * altRemainderSignB);
    } else if (face == MAX_Y_FACE) {
        slightlyRandomFaceNormal = glm::vec3(surfaceRemainder * altRemainderSignA, surfaceRandomness, surfaceRemainder * altRemainderSignB);
    } else if (face == MIN_Z_FACE) {
        slightlyRandomFaceNormal = glm::vec3(surfaceRemainder * altRemainderSignA, surfaceRemainder * altRemainderSignB, -surfaceRandomness);
    } else if (face == MAX_Z_FACE) {
        slightlyRandomFaceNormal = glm::vec3(surfaceRemainder * altRemainderSignA, surfaceRemainder * altRemainderSignB, surfaceRandomness);
    }
    return slightlyRandomFaceNormal;
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
        _listenerPosition = averageEarPosition;
qDebug() << "_listenerPosition:" << _listenerPosition.x << "," << _listenerPosition.y << "," << _listenerPosition.z;
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

QVector<glm::vec3> AudioReflector::calculateReflections(const glm::vec3& earPosition, 
                                const glm::vec3& origin, const glm::vec3& originalDirection) {
                                
    QVector<glm::vec3> reflectionPoints;
    glm::vec3 start = origin;
    glm::vec3 direction = originalDirection;
    OctreeElement* elementHit;
    float distance;
    BoxFace face;
    const float SLIGHTLY_SHORT = 0.999f; // slightly inside the distance so we're on the inside of the reflection point
    float currentAttenuation = 1.0f;
    float totalDistance = 0.0f;
    float totalDelay = 0.0f;
    int bounceCount = 1;
    
    while (currentAttenuation > MINIMUM_ATTENUATION_TO_REFLECT && totalDelay < MAXIMUM_DELAY_MS && bounceCount < ABSOLUTE_MAXIMUM_BOUNCE_COUNT) {
        if (_voxels->findRayIntersection(start, direction, elementHit, distance, face)) {
            glm::vec3 end = start + (direction * (distance * SLIGHTLY_SHORT));

            totalDistance += glm::distance(start, end);
            float earDistance = glm::distance(end, earPosition);
            float totalDistance = earDistance + distance;
            totalDelay = getDelayFromDistance(totalDistance);
            currentAttenuation = getDistanceAttenuationCoefficient(totalDistance) * getBounceAttenuationCoefficient(bounceCount);
            
            if (currentAttenuation > MINIMUM_ATTENUATION_TO_REFLECT && totalDelay < MAXIMUM_DELAY_MS) {
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

void AudioReflector::injectAudiblePoint(const AudioPoint& audiblePoint,
                        const QByteArray& samples, unsigned int sampleTime, int sampleRate) {

    bool wantEarSeparation = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingSeparateEars);
    bool wantStereo = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingStereoSource);
    glm::vec3 rightEarPosition = wantEarSeparation ? _myAvatar->getHead()->getRightEarPosition() : 
                                    _myAvatar->getHead()->getPosition();
    glm::vec3 leftEarPosition = wantEarSeparation ? _myAvatar->getHead()->getLeftEarPosition() :
                                    _myAvatar->getHead()->getPosition();

    int totalNumberOfSamples = samples.size() / sizeof(int16_t);
    int totalNumberOfStereoSamples = samples.size() / (sizeof(int16_t) * NUMBER_OF_CHANNELS);

    const int16_t* originalSamplesData = (const int16_t*)samples.constData();
    QByteArray attenuatedLeftSamples;
    QByteArray attenuatedRightSamples;
    attenuatedLeftSamples.resize(samples.size());
    attenuatedRightSamples.resize(samples.size());

    int16_t* attenuatedLeftSamplesData = (int16_t*)attenuatedLeftSamples.data();
    int16_t* attenuatedRightSamplesData = (int16_t*)attenuatedRightSamples.data();
    
    // calculate the distance to the ears
    float rightEarDistance = glm::distance(audiblePoint.location, rightEarPosition);
    float leftEarDistance = glm::distance(audiblePoint.location, leftEarPosition);

    float rightEarDelayMsecs = getDelayFromDistance(rightEarDistance) + audiblePoint.delay;
    float leftEarDelayMsecs = getDelayFromDistance(leftEarDistance) + audiblePoint.delay;
        
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

    float rightEarAttenuation = getDistanceAttenuationCoefficient(rightEarDistance) * audiblePoint.attenuation;
    float leftEarAttenuation = getDistanceAttenuationCoefficient(leftEarDistance) * audiblePoint.attenuation;

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
}
void AudioReflector::processLocalAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format) {
    // nothing yet, but will do local reflections too...
}

void AudioReflector::processInboundAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format) {
    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingWithDiffusions)) {
        newEchoAudio(sampleTime, samples, format);
    } else {
        oldEchoAudio(sampleTime, samples, format);
    }
}

void AudioReflector::newEchoAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format) {
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

    foreach(const AudioPoint& audiblePoint, _audiblePoints) {
        injectAudiblePoint(audiblePoint, samples, sampleTime, format.sampleRate());
    }

    _averageDelay = _delayCount == 0 ? 0 : _totalDelay / _delayCount;
    _averageAttenuation = _attenuationCount == 0 ? 0 : _totalAttenuation / _attenuationCount;
    _reflections = _audiblePoints.size();

    //quint64 end = usecTimestampNow();
    //qDebug() << "AudioReflector::addSamples()... elapsed=" << (end - start);
}

void AudioReflector::oldEchoAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format) {
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
    calculateAllReflections();

    const glm::vec3 RED(1,0,0);
    
    QMutexLocker locker(&_mutex);

    drawReflections(_origin, RED, _frontRightUpReflections);
    drawReflections(_origin, RED, _frontLeftUpReflections);
    drawReflections(_origin, RED, _backRightUpReflections);
    drawReflections(_origin, RED, _backLeftUpReflections);
    drawReflections(_origin, RED, _frontRightDownReflections);
    drawReflections(_origin, RED, _frontLeftDownReflections);
    drawReflections(_origin, RED, _backRightDownReflections);
    drawReflections(_origin, RED, _backLeftDownReflections);
    drawReflections(_origin, RED, _frontReflections);
    drawReflections(_origin, RED, _backReflections);
    drawReflections(_origin, RED, _leftReflections);
    drawReflections(_origin, RED, _rightReflections);
    drawReflections(_origin, RED, _upReflections);
    drawReflections(_origin, RED, _downReflections);
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



AudioPath::AudioPath(const glm::vec3& origin, const glm::vec3& direction, float attenuation, float delay, int bounceCount) :
    startPoint(origin),
    startDirection(direction),
    startDelay(delay),
    startAttenuation(attenuation),

    lastPoint(origin),
    lastDirection(direction),
    lastDistance(0.0f),
    lastDelay(delay),
    lastAttenuation(attenuation),
    bounceCount(bounceCount),

    finalized(false),
    reflections()
{
}


void AudioReflector::addSoundSource(const glm::vec3& origin, const glm::vec3& initialDirection, 
                                        float initialAttenuation, float initialDelay) {
                                        
    AudioPath* path = new AudioPath(origin, initialDirection, initialAttenuation, initialDelay, 0);
    _audioPaths.push_back(path);
}

void AudioReflector::newCalculateAllReflections() {
    // only recalculate when we've moved...
    // TODO: what about case where new voxels are added in front of us???
    bool wantHeadOrientation = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingHeadOriented);
    glm::quat orientation = wantHeadOrientation ? _myAvatar->getHead()->getFinalOrientation() : _myAvatar->getOrientation();
    glm::vec3 origin = _myAvatar->getHead()->getPosition();
    glm::vec3 listenerPosition = _myAvatar->getHead()->getPosition();
    
    bool shouldRecalc = _audiblePoints.size() == 0 
                            || !isSimilarPosition(origin, _origin) 
                            || !isSimilarOrientation(orientation, _orientation) 
                            || !isSimilarPosition(listenerPosition, _listenerPosition);

    if (shouldRecalc) {
    
        /*
        qDebug() << "_audiblePoints.size()=" << _audiblePoints.size();
        qDebug() << "isSimilarPosition(origin, _origin)=" << isSimilarPosition(origin, _origin);
        qDebug() << "isSimilarPosition(listenerPosition, _listenerPosition)=" << isSimilarPosition(listenerPosition, _listenerPosition);
        qDebug() << "isSimilarOrientation(orientation, _orientation)=" << isSimilarOrientation(orientation, _orientation);
        if (!isSimilarOrientation(orientation, _orientation)) {
            qDebug() << "    orientation=" << orientation.x << "," << orientation.y << ","
                        << orientation.y << "," << orientation.w;

            qDebug() << "    _orientation=" << _orientation.x << "," << _orientation.y << ","
                        << _orientation.y << "," << _orientation.w;
        }
        */

        QMutexLocker locker(&_mutex);
        quint64 start = usecTimestampNow();        
        _origin = origin;
        _orientation = orientation;
        _listenerPosition = listenerPosition;
        
        anylizePaths(); // actually does the work
        
        quint64 end = usecTimestampNow();
    }
}    

void AudioReflector::newDrawRays() {
    newCalculateAllReflections();

    const glm::vec3 RED(1,0,0);
    const glm::vec3 GREEN(0,1,0);
    
    int diffusionNumber = 0;
    
    QMutexLocker locker(&_mutex);
    foreach(AudioPath* const& path, _audioPaths) {
    
        // if this is an original reflection, draw it in RED
        if (path->startPoint == _origin) {
            drawPath(path, RED);
        } else {
            diffusionNumber++;
            drawPath(path, GREEN);
        }
    }
}

void AudioReflector::drawPath(AudioPath* path, const glm::vec3& originalColor) {
    glm::vec3 start = path->startPoint;
    glm::vec3 color = originalColor;
    const float COLOR_ADJUST_PER_BOUNCE = 0.75f;
    
    foreach (glm::vec3 end, path->reflections) {
        drawVector(start, end, color);
        start = end;
        color = color * COLOR_ADJUST_PER_BOUNCE;
    }
}


void AudioReflector::anylizePaths() {
    // clear our _audioPaths
    foreach(AudioPath* const& path, _audioPaths) {
        delete path;
    }
    _audioPaths.clear();
    _audiblePoints.clear(); // clear our audible points
    
    // add our initial paths
    glm::vec3 averageEarPosition = _myAvatar->getHead()->getPosition();
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

    float initialAttenuation = 1.0f;    

    float preDelay = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingPreDelay) ? _preDelay : 0.0f;
        
    addSoundSource(_origin, right, initialAttenuation, preDelay);
    addSoundSource(_origin, front, initialAttenuation, preDelay);
    addSoundSource(_origin, up, initialAttenuation, preDelay);
    addSoundSource(_origin, down, initialAttenuation, preDelay);
    addSoundSource(_origin, back, initialAttenuation, preDelay);
    addSoundSource(_origin, left, initialAttenuation, preDelay);

    addSoundSource(_origin, frontRightUp, initialAttenuation, preDelay);
    addSoundSource(_origin, frontLeftUp, initialAttenuation, preDelay);
    addSoundSource(_origin, backRightUp, initialAttenuation, preDelay);
    addSoundSource(_origin, backLeftUp, initialAttenuation, preDelay);
    addSoundSource(_origin, frontRightDown, initialAttenuation, preDelay);
    addSoundSource(_origin, frontLeftDown, initialAttenuation, preDelay);
    addSoundSource(_origin, backRightDown, initialAttenuation, preDelay);
    addSoundSource(_origin, backLeftDown, initialAttenuation, preDelay);

    // loop through all our 
    int steps = 0;
    int acitvePaths = _audioPaths.size(); // when we start, all paths are active
    while(acitvePaths > 0) {
        acitvePaths = anylizePathsSingleStep();
        steps++;
    }
    _reflections = _audiblePoints.size();
}

int AudioReflector::anylizePathsSingleStep() {
    // iterate all the active sound paths, calculate one step per active path
    
    int activePaths = 0;
    foreach(AudioPath* const& path, _audioPaths) {
    
        bool isDiffusion = (path->startPoint != _origin);
        
        glm::vec3 start = path->lastPoint;
        glm::vec3 direction = path->lastDirection;
        OctreeElement* elementHit; // output from findRayIntersection
        float distance; // output from findRayIntersection
        BoxFace face; // output from findRayIntersection

        float currentAttenuation = path->lastAttenuation;
        float currentDelay = path->lastDelay; // start with our delay so far
        float pathDistance = path->lastDistance;
        float totalDelay = path->lastDelay; // start with our delay so far
        unsigned int bounceCount = path->bounceCount;

        if (!path->finalized) {
            activePaths++;
            
            // quick hack to stop early reflections right away...
            //if (!isDiffusion && path->bounceCount > 1) {
            //    path->finalized = true;
            //    qDebug() << "stopping reflections on first bounce!";
            //} else 
            if (path->bounceCount > ABSOLUTE_MAXIMUM_BOUNCE_COUNT) {
                path->finalized = true;
                /*
                if (isDiffusion) {
                    qDebug() << "diffusion bounceCount too high!";
                }
                */
            } else if (_voxels->findRayIntersection(start, direction, elementHit, distance, face)) {
                glm::vec3 end = start + (direction * (distance * SLIGHTLY_SHORT));

                pathDistance += glm::distance(start, end);
                

                /*                    
                qDebug() << "ray intersection... "
                    << " startPoint=[" << path->startPoint.x << "," << path->startPoint.y << "," << path->startPoint.z << "]"
                    << " bouceCount= " << path->bounceCount
                    << " end=[" << end.x << "," << end.y << "," << end.z << "]" 
                    << " pathDistance=" << pathDistance;
                */

            
                // We aren't using this... should we be????
                float toListenerDistance = glm::distance(end, _listenerPosition);
                float totalDistance = toListenerDistance + pathDistance;
            
                // adjust our current delay by just the delay from the most recent ray
                currentDelay += getDelayFromDistance(distance);

                // adjust our previous attenuation based on the distance traveled in last ray
                currentAttenuation *= getDistanceAttenuationCoefficient(distance); 

                // now we know the current attenuation for the "perfect" reflection case, but we now incorporate
                // our surface materials to determine how much of this ray is absorbed, reflected, and diffused
                SurfaceCharacteristics material = getSurfaceCharacteristics(elementHit);
            
                float reflectiveAttenuation = currentAttenuation * material.reflectiveRatio;
                float totalDiffusionAttenuation = currentAttenuation * material.diffusionRatio;
                float partialDiffusionAttenuation = _diffusionFanout < 1 ? 0.0f : totalDiffusionAttenuation / _diffusionFanout;

                // total delay includes the bounce back to listener
                totalDelay = getDelayFromDistance(totalDistance);
                float toListenerAttenuation = getDistanceAttenuationCoefficient(toListenerDistance);
                //qDebug() << "toListenerDistance=" << toListenerDistance;
                //qDebug() << "toListenerAttenuation=" << toListenerAttenuation;
            
                // if our resulting partial diffusion attenuation, is still above our minimum attenuation
                // then we add new paths for each diffusion point
                if ((partialDiffusionAttenuation * toListenerAttenuation) > MINIMUM_ATTENUATION_TO_REFLECT
                        && totalDelay < MAXIMUM_DELAY_MS) {
                        
                    // add sound sources for the normal
                    //glm::vec3 faceNormal = getFaceNormal(face);
                    //addSoundSource(end, faceNormal, partialDiffusionAttenuation, currentDelay);
                    
                    // diffusions fan out from random places on the semisphere of the collision point
                    for(int i = 0; i < _diffusionFanout; i++) {
                        glm::vec3 randomDirection;

                        float surfaceRandomness = randFloatInRange(0.5f,1.0f);
                        float surfaceRemainder = (1.0f - surfaceRandomness)/2.0f;
                        float altRemainderSignA = (randFloatInRange(-1.0f,1.0f) < 0.0f) ? -1.0 : 1.0;
                        float altRemainderSignB = (randFloatInRange(-1.0f,1.0f) < 0.0f) ? -1.0 : 1.0;

                        if (face == MIN_X_FACE) {
                            randomDirection = glm::vec3(-surfaceRandomness, surfaceRemainder * altRemainderSignA, surfaceRemainder * altRemainderSignB);
                        } else if (face == MAX_X_FACE) {
                            randomDirection = glm::vec3(surfaceRandomness, surfaceRemainder * altRemainderSignA, surfaceRemainder * altRemainderSignB);
                        } else if (face == MIN_Y_FACE) {
                            randomDirection = glm::vec3(surfaceRemainder * altRemainderSignA, -surfaceRandomness, surfaceRemainder * altRemainderSignB);
                        } else if (face == MAX_Y_FACE) {
                            randomDirection = glm::vec3(surfaceRemainder * altRemainderSignA, surfaceRandomness, surfaceRemainder * altRemainderSignB);
                        } else if (face == MIN_Z_FACE) {
                            randomDirection = glm::vec3(surfaceRemainder * altRemainderSignA, surfaceRemainder * altRemainderSignB, -surfaceRandomness);
                        } else if (face == MAX_Z_FACE) {
                            randomDirection = glm::vec3(surfaceRemainder * altRemainderSignA, surfaceRemainder * altRemainderSignB, surfaceRandomness);
                        }
                        
                        randomDirection = glm::normalize(randomDirection);

                        /*
                        qDebug() << "DIFFUSION... addSoundSource()... partialDiffusionAttenuation=" << partialDiffusionAttenuation << "\n" << 
                                        "    MINIMUM_ATTENUATION_TO_REFLECT=" << MINIMUM_ATTENUATION_TO_REFLECT << "\n" << 
                                        "    previous direction=[" << direction.x << "," << direction.y << "," << direction.z << "]" << "\n" << 
                                        "    randomDirection=[" << randomDirection.x << "," << randomDirection.y << "," << randomDirection.z << "]" << "\n" << 
                                        "    end=[" << end.x << "," << end.y << "," << end.z << "]";
                        */

                        // add sound sources for these diffusions
                        addSoundSource(end, randomDirection, partialDiffusionAttenuation, currentDelay);
                    }
                }
            
                // if our reflective attenuation is above our minimum, then add our reflection point and
                // allow our path to continue
                /*
                if (isDiffusion) {
                    qDebug() << "checking diffusion";
                    qDebug() << "reflectiveAttenuation=" << reflectiveAttenuation;
                    qDebug() << "totalDiffusionAttenuation=" << totalDiffusionAttenuation;
                    qDebug() << "toListenerAttenuation=" << toListenerAttenuation;
                    qDebug() << "(reflectiveAttenuation + totalDiffusionAttenuation) * toListenerAttenuation=" << ((reflectiveAttenuation + totalDiffusionAttenuation) * toListenerAttenuation);
                }
                */
                
                if (((reflectiveAttenuation + totalDiffusionAttenuation) * toListenerAttenuation) > MINIMUM_ATTENUATION_TO_REFLECT
                        && totalDelay < MAXIMUM_DELAY_MS) {
            
                    // add this location, as the reflective attenuation as well as the total diffusion attenuation
                    AudioPoint point = { end, totalDelay, reflectiveAttenuation + totalDiffusionAttenuation };
                    _audiblePoints.push_back(point);
                
                    // add this location to the path points, so we can visualize it
                    path->reflections.push_back(end);
                
                    // now, if our reflective attenuation is over our minimum then keep going...
                    if (reflectiveAttenuation * toListenerAttenuation > MINIMUM_ATTENUATION_TO_REFLECT) {
                        glm::vec3 faceNormal = getFaceNormal(face);
                        path->lastDirection = glm::normalize(glm::reflect(direction,faceNormal));
                        path->lastPoint = end;
                        path->lastAttenuation = reflectiveAttenuation;
                        path->lastDelay = currentDelay;
                        path->lastDistance = pathDistance;
                        path->bounceCount++;
                    } else {
                        path->finalized = true; // if we're too quiet, then we're done
                    }
                } else {
                    path->finalized = true; // if we're too quiet, then we're done
                    /*
                    if (isDiffusion) {
                        qDebug() << "diffusion too quiet!";
                    }
                    */
                }
            } else {
                //qDebug() << "whichPath=" << activePaths << "path->bounceCount=" << path->bounceCount << "ray missed...";
                path->finalized = true; // if it doesn't intersect, then it is finished
                
                /*
                if (isDiffusion) {
                    qDebug() << "diffusion doesn't intersect!";
                }
                */
            }
        }
    }
    return activePaths;
}

SurfaceCharacteristics AudioReflector::getSurfaceCharacteristics(OctreeElement* elementHit) {
    float reflectiveRatio = (1.0f - (_absorptionRatio + _diffusionRatio));
    SurfaceCharacteristics result = { reflectiveRatio, _absorptionRatio, _diffusionRatio };
    return result;
}


