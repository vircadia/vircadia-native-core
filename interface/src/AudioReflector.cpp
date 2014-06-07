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
const unsigned int ABSOLUTE_MAXIMUM_BOUNCE_COUNT = 10;
const float DEFAULT_LOCAL_ATTENUATION_FACTOR = 0.125;
const float DEFAULT_COMB_FILTER_WINDOW = 0.05f; //ms delay differential to avoid

const float SLIGHTLY_SHORT = 0.999f; // slightly inside the distance so we're on the inside of the reflection point

const float DEFAULT_ABSORPTION_RATIO = 0.125; // 12.5% is absorbed
const float DEFAULT_DIFFUSION_RATIO = 0.125; // 12.5% is diffused
const float DEFAULT_ORIGINAL_ATTENUATION = 1.0f;
const float DEFAULT_ECHO_ATTENUATION = 1.0f;

AudioReflector::AudioReflector(QObject* parent) : 
    QObject(parent),
    _preDelay(DEFAULT_PRE_DELAY),
    _soundMsPerMeter(DEFAULT_MS_DELAY_PER_METER),
    _distanceAttenuationScalingFactor(DEFAULT_DISTANCE_SCALING_FACTOR),
    _localAudioAttenuationFactor(DEFAULT_LOCAL_ATTENUATION_FACTOR),
    _combFilterWindow(DEFAULT_COMB_FILTER_WINDOW),
    _diffusionFanout(DEFAULT_DIFFUSION_FANOUT),
    _absorptionRatio(DEFAULT_ABSORPTION_RATIO),
    _diffusionRatio(DEFAULT_DIFFUSION_RATIO),
    _originalSourceAttenuation(DEFAULT_ORIGINAL_ATTENUATION),
    _allEchoesAttenuation(DEFAULT_ECHO_ATTENUATION),
    _withDiffusion(false),
    _lastPreDelay(DEFAULT_PRE_DELAY),
    _lastSoundMsPerMeter(DEFAULT_MS_DELAY_PER_METER),
    _lastDistanceAttenuationScalingFactor(DEFAULT_DISTANCE_SCALING_FACTOR),
    _lastLocalAudioAttenuationFactor(DEFAULT_LOCAL_ATTENUATION_FACTOR),
    _lastDiffusionFanout(DEFAULT_DIFFUSION_FANOUT),
    _lastAbsorptionRatio(DEFAULT_ABSORPTION_RATIO),
    _lastDiffusionRatio(DEFAULT_DIFFUSION_RATIO),
    _lastDontDistanceAttenuate(false),
    _lastAlternateDistanceAttenuate(false)
{
    _reflections = 0;
    _diffusionPathCount = 0;
    _officialAverageAttenuation = _averageAttenuation = 0.0f;
    _officialMaxAttenuation = _maxAttenuation = 0.0f;
    _officialMinAttenuation = _minAttenuation = 0.0f;
    _officialAverageDelay = _averageDelay = 0;
    _officialMaxDelay = _maxDelay = 0;
    _officialMinDelay = _minDelay = 0;
    _inboundEchoesCount = 0;
    _inboundEchoesSuppressedCount = 0;
    _localEchoesCount = 0;
    _localEchoesSuppressedCount = 0;
}

bool AudioReflector::haveAttributesChanged() {
    bool withDiffusion = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingWithDiffusions);
    bool dontDistanceAttenuate = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingDontDistanceAttenuate);
    bool alternateDistanceAttenuate = Menu::getInstance()->isOptionChecked(
                                                MenuOption::AudioSpatialProcessingAlternateDistanceAttenuate);
    
    bool attributesChange = (_withDiffusion != withDiffusion
        || _lastPreDelay != _preDelay
        || _lastSoundMsPerMeter != _soundMsPerMeter
        || _lastDistanceAttenuationScalingFactor != _distanceAttenuationScalingFactor
        || _lastDiffusionFanout != _diffusionFanout
        || _lastAbsorptionRatio != _absorptionRatio
        || _lastDiffusionRatio != _diffusionRatio
        || _lastDontDistanceAttenuate != dontDistanceAttenuate
        || _lastAlternateDistanceAttenuate != alternateDistanceAttenuate);

    if (attributesChange) {
        _withDiffusion = withDiffusion;
        _lastPreDelay = _preDelay;
        _lastSoundMsPerMeter = _soundMsPerMeter;
        _lastDistanceAttenuationScalingFactor = _distanceAttenuationScalingFactor;
        _lastDiffusionFanout = _diffusionFanout;
        _lastAbsorptionRatio = _absorptionRatio;
        _lastDiffusionRatio = _diffusionRatio;
        _lastDontDistanceAttenuate = dontDistanceAttenuate;
        _lastAlternateDistanceAttenuate = alternateDistanceAttenuate;
    }
    
    return attributesChange;
}

void AudioReflector::render() {

    // if we're not set up yet, or we're not processing spatial audio, then exit early
    if (!_myAvatar || !_audio->getProcessSpatialAudio()) {
        return;
    }

    // use this oportunity to calculate our reflections
    calculateAllReflections();
    
    // only render if we've been asked to do so
    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingRenderPaths)) {
        drawRays();
    }
}

// delay = 1ms per foot
//       = 3ms per meter
float AudioReflector::getDelayFromDistance(float distance) {
    float delay = (_soundMsPerMeter * distance);
    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingPreDelay)) {
        delay += _preDelay;
    }
    return delay;
}

// attenuation = from the Audio Mixer
float AudioReflector::getDistanceAttenuationCoefficient(float distance) {


    bool doDistanceAttenuation = !Menu::getInstance()->isOptionChecked(
                                            MenuOption::AudioSpatialProcessingDontDistanceAttenuate);

    bool originalFormula = !Menu::getInstance()->isOptionChecked(
                                            MenuOption::AudioSpatialProcessingAlternateDistanceAttenuate);
    
    
    float distanceCoefficient = 1.0f;
    
    if (doDistanceAttenuation) {
    
        if (originalFormula) {
            const float DISTANCE_SCALE = 2.5f;
            const float GEOMETRIC_AMPLITUDE_SCALAR = 0.3f;
            const float DISTANCE_LOG_BASE = 2.5f;
            const float DISTANCE_SCALE_LOG = logf(DISTANCE_SCALE) / logf(DISTANCE_LOG_BASE);
    
            float distanceSquareToSource = distance * distance;

            // calculate the distance coefficient using the distance to this node
            distanceCoefficient = powf(GEOMETRIC_AMPLITUDE_SCALAR,
                                             DISTANCE_SCALE_LOG +
                                             (0.5f * logf(distanceSquareToSource) / logf(DISTANCE_LOG_BASE)) - 1);
            distanceCoefficient = std::min(1.0f, distanceCoefficient * getDistanceAttenuationScalingFactor());
        } else {
        
            // From Fred: If we wanted something that would produce a tail that could go up to 5 seconds in a 
            // really big room, that would suggest the sound still has to be in the audible after traveling about 
            // 1500 meters.  If it’s a sound of average volume, we probably have about 30 db, or 5 base2 orders 
            // of magnitude we can drop down before the sound becomes inaudible. (That’s approximate headroom 
            // based on a few sloppy assumptions.) So we could try a factor like 1 / (2^(D/300)) for starters.
            // 1 / (2^(D/300))
            const float DISTANCE_BASE = 2.0f;
            const float DISTANCE_DENOMINATOR = 300.0f;
            const float DISTANCE_NUMERATOR = 300.0f;
            distanceCoefficient = DISTANCE_NUMERATOR / powf(DISTANCE_BASE, (distance / DISTANCE_DENOMINATOR ));
            distanceCoefficient = std::min(1.0f, distanceCoefficient * getDistanceAttenuationScalingFactor());
        }
    }
    
    return distanceCoefficient;
}

glm::vec3 AudioReflector::getFaceNormal(BoxFace face) {
    bool wantSlightRandomness = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingSlightlyRandomSurfaces);
    glm::vec3 faceNormal;
    const float MIN_RANDOM_LENGTH = 0.99f;
    const float MAX_RANDOM_LENGTH = 1.0f;
    const float NON_RANDOM_LENGTH = 1.0f;
    float normalLength = wantSlightRandomness ? randFloatInRange(MIN_RANDOM_LENGTH, MAX_RANDOM_LENGTH) : NON_RANDOM_LENGTH;
    float remainder = (1.0f - normalLength)/2.0f;
    float remainderSignA = randomSign();
    float remainderSignB = randomSign();

    if (face == MIN_X_FACE) {
        faceNormal = glm::vec3(-normalLength, remainder * remainderSignA, remainder * remainderSignB);
    } else if (face == MAX_X_FACE) {
        faceNormal = glm::vec3(normalLength, remainder * remainderSignA, remainder * remainderSignB);
    } else if (face == MIN_Y_FACE) {
        faceNormal = glm::vec3(remainder * remainderSignA, -normalLength, remainder * remainderSignB);
    } else if (face == MAX_Y_FACE) {
        faceNormal = glm::vec3(remainder * remainderSignA, normalLength, remainder * remainderSignB);
    } else if (face == MIN_Z_FACE) {
        faceNormal = glm::vec3(remainder * remainderSignA, remainder * remainderSignB, -normalLength);
    } else if (face == MAX_Z_FACE) {
        faceNormal = glm::vec3(remainder * remainderSignA, remainder * remainderSignB, normalLength);
    }
    return faceNormal;
}

// set up our buffers for our attenuated and delayed samples
const int NUMBER_OF_CHANNELS = 2;

void AudioReflector::injectAudiblePoint(AudioSource source, const AudiblePoint& audiblePoint,
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
    float averageEarDelayMsecs = (leftEarDelayMsecs + rightEarDelayMsecs) / 2.0f;
    
    bool safeToInject = true; // assume the best
    
    // check to see if this new injection point would be within the comb filter 
    // suppression window for any of the existing known delays
    QMap<float, float>& knownDelays = (source == INBOUND_AUDIO) ? _inboundAudioDelays : _localAudioDelays;
    QMap<float, float>::const_iterator lowerBound = knownDelays.lowerBound(averageEarDelayMsecs - _combFilterWindow);
    if (lowerBound != knownDelays.end()) {
        float closestFound = lowerBound.value();
        float deltaToClosest = (averageEarDelayMsecs - closestFound);
        if (deltaToClosest > -_combFilterWindow && deltaToClosest < _combFilterWindow) {
            safeToInject = false;
        }
    }
    
    // keep track of any of our suppressed echoes so we can report them in our statistics
    if (!safeToInject) {
        QVector<float>& suppressedEchoes = (source == INBOUND_AUDIO) ? _inboundEchoesSuppressed : _localEchoesSuppressed;
        suppressedEchoes << averageEarDelayMsecs;
    } else {
        knownDelays[averageEarDelayMsecs] = averageEarDelayMsecs;

        _totalDelay += rightEarDelayMsecs + leftEarDelayMsecs;
        _delayCount += 2;
        _maxDelay = std::max(_maxDelay,rightEarDelayMsecs);
        _maxDelay = std::max(_maxDelay,leftEarDelayMsecs);
        _minDelay = std::min(_minDelay,rightEarDelayMsecs);
        _minDelay = std::min(_minDelay,leftEarDelayMsecs);
        
        int rightEarDelay = rightEarDelayMsecs * sampleRate / MSECS_PER_SECOND;
        int leftEarDelay = leftEarDelayMsecs * sampleRate / MSECS_PER_SECOND;

        float rightEarAttenuation = audiblePoint.attenuation * 
                                        getDistanceAttenuationCoefficient(rightEarDistance + audiblePoint.distance);

        float leftEarAttenuation = audiblePoint.attenuation * 
                                        getDistanceAttenuationCoefficient(leftEarDistance + audiblePoint.distance);

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

            attenuatedLeftSamplesData[sample * NUMBER_OF_CHANNELS] = 
                        leftSample * leftEarAttenuation * _allEchoesAttenuation;
            attenuatedLeftSamplesData[sample * NUMBER_OF_CHANNELS + 1] = 0;

            attenuatedRightSamplesData[sample * NUMBER_OF_CHANNELS] = 0;
            attenuatedRightSamplesData[sample * NUMBER_OF_CHANNELS + 1] = 
                        rightSample * rightEarAttenuation * _allEchoesAttenuation;
        }
        
        // now inject the attenuated array with the appropriate delay
        unsigned int sampleTimeLeft = sampleTime + leftEarDelay;
        unsigned int sampleTimeRight = sampleTime + rightEarDelay;
    
        _audio->addSpatialAudioToBuffer(sampleTimeLeft, attenuatedLeftSamples, totalNumberOfSamples);
        _audio->addSpatialAudioToBuffer(sampleTimeRight, attenuatedRightSamples, totalNumberOfSamples);
        
        _injectedEchoes++;
    }
}


void AudioReflector::preProcessOriginalInboundAudio(unsigned int sampleTime, 
                                QByteArray& samples, const QAudioFormat& format) {
                                
    if (_originalSourceAttenuation != 1.0f) {
        int numberOfSamples = (samples.size() / sizeof(int16_t));
        int16_t* sampleData = (int16_t*)samples.data();
        for (int i = 0; i < numberOfSamples; i++) {
            sampleData[i] = sampleData[i] * _originalSourceAttenuation;
        }
    }

}

void AudioReflector::processLocalAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format) {
    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingProcessLocalAudio)) {
        const int NUM_CHANNELS_INPUT = 1;
        const int NUM_CHANNELS_OUTPUT = 2;
        const int EXPECTED_SAMPLE_RATE = 24000;
        if (format.channelCount() == NUM_CHANNELS_INPUT && format.sampleRate() == EXPECTED_SAMPLE_RATE) {
            QAudioFormat outputFormat = format;
            outputFormat.setChannelCount(NUM_CHANNELS_OUTPUT);
            QByteArray stereoInputData(samples.size() * NUM_CHANNELS_OUTPUT, 0);
            int numberOfSamples = (samples.size() / sizeof(int16_t));
            int16_t* monoSamples = (int16_t*)samples.data();
            int16_t* stereoSamples = (int16_t*)stereoInputData.data();
            
            for (int i = 0; i < numberOfSamples; i++) {
                stereoSamples[i* NUM_CHANNELS_OUTPUT] = monoSamples[i] * _localAudioAttenuationFactor;
                stereoSamples[(i * NUM_CHANNELS_OUTPUT) + 1] = monoSamples[i] * _localAudioAttenuationFactor;
            }
            _localAudioDelays.clear();
            _localEchoesSuppressed.clear();
            echoAudio(LOCAL_AUDIO, sampleTime, stereoInputData, outputFormat);
            _localEchoesCount = _localAudioDelays.size();
            _localEchoesSuppressedCount = _localEchoesSuppressed.size();
        }
    }
}

void AudioReflector::processInboundAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format) {
    _inboundAudioDelays.clear();
    _inboundEchoesSuppressed.clear();
    echoAudio(INBOUND_AUDIO, sampleTime, samples, format);
    _inboundEchoesCount = _inboundAudioDelays.size();
    _inboundEchoesSuppressedCount = _inboundEchoesSuppressed.size();
}

void AudioReflector::echoAudio(AudioSource source, unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format) {
    QMutexLocker locker(&_mutex);

    _maxDelay = 0;
    _maxAttenuation = 0.0f;
    _minDelay = std::numeric_limits<int>::max();
    _minAttenuation = std::numeric_limits<float>::max();
    _totalDelay = 0.0f;
    _delayCount = 0;
    _totalAttenuation = 0.0f;
    _attenuationCount = 0;

    // depending on if we're processing local or external audio, pick the correct points vector
    QVector<AudiblePoint>& audiblePoints = source == INBOUND_AUDIO ? _inboundAudiblePoints : _localAudiblePoints;

    int injectCalls = 0;
    _injectedEchoes = 0;
    foreach(const AudiblePoint& audiblePoint, audiblePoints) {
        injectCalls++;
        injectAudiblePoint(source, audiblePoint, samples, sampleTime, format.sampleRate());
    }
    
    /*
    qDebug() << "injectCalls=" << injectCalls;
    qDebug() << "_injectedEchoes=" << _injectedEchoes;
    */

    _averageDelay = _delayCount == 0 ? 0 : _totalDelay / _delayCount;
    _averageAttenuation = _attenuationCount == 0 ? 0 : _totalAttenuation / _attenuationCount;
    
    if (_reflections == 0) {
        _minDelay = 0.0f;
        _minAttenuation = 0.0f;
    }
    
    _officialMaxDelay = _maxDelay;
    _officialMinDelay = _minDelay;
    _officialMaxAttenuation = _maxAttenuation;
    _officialMinAttenuation = _minAttenuation;
    _officialAverageDelay = _averageDelay;
    _officialAverageAttenuation = _averageAttenuation;

}

void AudioReflector::drawVector(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
    glDisable(GL_LIGHTING);
    glLineWidth(2.0);

    // Draw the vector itself
    glBegin(GL_LINES);
    glColor3f(color.x,color.y,color.z);
    glVertex3f(start.x, start.y, start.z);
    glVertex3f(end.x, end.y, end.z);
    glEnd();

    glEnable(GL_LIGHTING);
}



AudioPath::AudioPath(AudioSource source, const glm::vec3& origin, const glm::vec3& direction, 
                        float attenuation, float delay, float distance,bool isDiffusion, int bounceCount) :
                        
    source(source),
    isDiffusion(isDiffusion),
    startPoint(origin),
    startDirection(direction),
    startDelay(delay),
    startAttenuation(attenuation),

    lastPoint(origin),
    lastDirection(direction),
    lastDistance(distance),
    lastDelay(delay),
    lastAttenuation(attenuation),
    bounceCount(bounceCount),

    finalized(false),
    reflections()
{
}

void AudioReflector::addAudioPath(AudioSource source, const glm::vec3& origin, const glm::vec3& initialDirection, 
                                        float initialAttenuation, float initialDelay, float initialDistance, bool isDiffusion) {
                                        
    AudioPath* path = new AudioPath(source, origin, initialDirection, initialAttenuation, initialDelay,
                                        initialDistance, isDiffusion, 0);

    QVector<AudioPath*>& audioPaths = source == INBOUND_AUDIO ? _inboundAudioPaths : _localAudioPaths;

    audioPaths.push_back(path);
}

// NOTE: This is a prototype of an eventual utility that will identify the speaking sources for the inbound audio
// stream. It's not currently called but will be added soon.
void AudioReflector::identifyAudioSources() {
    // looking for audio sources....
    foreach (const AvatarSharedPointer& avatarPointer, _avatarManager->getAvatarHash()) {
        Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
        if (!avatar->isInitialized()) {
            continue;
        }
        qDebug() << "avatar["<< avatar <<"] loudness:" <<  avatar->getAudioLoudness();
    }
}

void AudioReflector::calculateAllReflections() {
    // only recalculate when we've moved, or if the attributes have changed
    // TODO: what about case where new voxels are added in front of us???
    bool wantHeadOrientation = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingHeadOriented);
    glm::quat orientation = wantHeadOrientation ? _myAvatar->getHead()->getFinalOrientationInWorldFrame() : _myAvatar->getOrientation();
    glm::vec3 origin = _myAvatar->getHead()->getPosition();
    glm::vec3 listenerPosition = _myAvatar->getHead()->getPosition();

    bool shouldRecalc = _reflections == 0
                            || !isSimilarPosition(origin, _origin) 
                            || !isSimilarOrientation(orientation, _orientation) 
                            || !isSimilarPosition(listenerPosition, _listenerPosition)
                            || haveAttributesChanged();

    if (shouldRecalc) {
        QMutexLocker locker(&_mutex);
        quint64 start = usecTimestampNow();        
        _origin = origin;
        _orientation = orientation;
        _listenerPosition = listenerPosition;
        analyzePaths(); // actually does the work
        quint64 end = usecTimestampNow();
        const bool wantDebugging = false;
        if (wantDebugging) {
            qDebug() << "newCalculateAllReflections() elapsed=" << (end - start);
        }
    }
}    

void AudioReflector::drawRays() {
    const glm::vec3 RED(1,0,0);
    const glm::vec3 GREEN(0,1,0);
    const glm::vec3 BLUE(0,0,1);
    const glm::vec3 CYAN(0,1,1);
    
    int diffusionNumber = 0;
    
    QMutexLocker locker(&_mutex);

    // draw the paths for inbound audio
    foreach(AudioPath* const& path, _inboundAudioPaths) {
        // if this is an original reflection, draw it in RED
        if (path->isDiffusion) {
            diffusionNumber++;
            drawPath(path, GREEN);
        } else {
            drawPath(path, RED);
        }
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingProcessLocalAudio)) {
        // draw the paths for local audio
        foreach(AudioPath* const& path, _localAudioPaths) {
            // if this is an original reflection, draw it in RED
            if (path->isDiffusion) {
                diffusionNumber++;
                drawPath(path, CYAN);
            } else {
                drawPath(path, BLUE);
            }
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

void AudioReflector::clearPaths() {
    // clear our inbound audio paths
    foreach(AudioPath* const& path, _inboundAudioPaths) {
        delete path;
    }
    _inboundAudioPaths.clear();
    _inboundAudiblePoints.clear(); // clear our inbound audible points

    // clear our local audio paths
    foreach(AudioPath* const& path, _localAudioPaths) {
        delete path;
    }
    _localAudioPaths.clear();
    _localAudiblePoints.clear(); // clear our local audible points
}

// Here's how this works: we have an array of AudioPaths, we loop on all of our currently calculating audio 
// paths, and calculate one ray per path. If that ray doesn't reflect, or reaches a max distance/attenuation, then it
// is considered finalized.
// If the ray hits a surface, then, based on the characteristics of that surface, it will calculate the new 
// attenuation, path length, and delay for the primary path. For surfaces that have diffusion, it will also create
// fanout number of new paths, those new paths will have an origin of the reflection point, and an initial attenuation
// of their diffusion ratio. Those new paths will be added to the active audio paths, and be analyzed for the next loop.
void AudioReflector::analyzePaths() {
    clearPaths();
    
    // add our initial paths
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

    // NOTE: we're still calculating our initial paths based on the listeners position. But the analysis code has been
    // updated to support individual sound sources (which is how we support diffusion), we can use this new paradigm to
    // add support for individual sound sources, and more directional sound sources    

    addAudioPath(INBOUND_AUDIO, _origin, front, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, right, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, up, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, down, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, back, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, left, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, frontRightUp, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, frontLeftUp, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, backRightUp, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, backLeftUp, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, frontRightDown, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, frontLeftDown, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, backRightDown, initialAttenuation, preDelay);
    addAudioPath(INBOUND_AUDIO, _origin, backLeftDown, initialAttenuation, preDelay);
    
    // the original paths for the local audio are directional to the front of the origin
    addAudioPath(LOCAL_AUDIO, _origin, front, initialAttenuation, preDelay);
    addAudioPath(LOCAL_AUDIO, _origin, frontRightUp, initialAttenuation, preDelay);
    addAudioPath(LOCAL_AUDIO, _origin, frontLeftUp, initialAttenuation, preDelay);
    addAudioPath(LOCAL_AUDIO, _origin, frontRightDown, initialAttenuation, preDelay);
    addAudioPath(LOCAL_AUDIO, _origin, frontLeftDown, initialAttenuation, preDelay);

    // loop through all our audio paths and keep analyzing them until they complete
    int steps = 0;
    int acitvePaths = _inboundAudioPaths.size() + _localAudioPaths.size(); // when we start, all paths are active
    while(acitvePaths > 0) {
        acitvePaths = analyzePathsSingleStep();
        steps++;
    }
    _reflections = _inboundAudiblePoints.size() + _localAudiblePoints.size();
    _diffusionPathCount = countDiffusionPaths();
}

int AudioReflector::countDiffusionPaths() {
    int diffusionCount = 0;
    
    foreach(AudioPath* const& path, _inboundAudioPaths) {
        if (path->isDiffusion) {
            diffusionCount++;
        }
    }
    foreach(AudioPath* const& path, _localAudioPaths) {
        if (path->isDiffusion) {
            diffusionCount++;
        }
    }
    return diffusionCount;
}

int AudioReflector::analyzePathsSingleStep() {
    // iterate all the active sound paths, calculate one step per active path
    int activePaths = 0;

    QVector<AudioPath*>* pathsLists[] = { &_inboundAudioPaths, &_localAudioPaths };

    for(unsigned int i = 0; i < sizeof(pathsLists) / sizeof(pathsLists[0]); i++) {

        QVector<AudioPath*>& pathList = *pathsLists[i];

        foreach(AudioPath* const& path, pathList) {
    
            glm::vec3 start = path->lastPoint;
            glm::vec3 direction = path->lastDirection;
            OctreeElement* elementHit; // output from findRayIntersection
            float distance; // output from findRayIntersection
            BoxFace face; // output from findRayIntersection

            if (!path->finalized) {
                activePaths++;
            
                if (path->bounceCount > ABSOLUTE_MAXIMUM_BOUNCE_COUNT) {
                    path->finalized = true;
                } else if (_voxels->findRayIntersection(start, direction, elementHit, distance, face)) {
                    // TODO: we need to decide how we want to handle locking on the ray intersection, if we force lock,
                    // we get an accurate picture, but it could prevent rendering of the voxels. If we trylock (default), 
                    // we might not get ray intersections where they may exist, but we can't really detect that case...
                    // add last parameter of Octree::Lock to force locking
                    handlePathPoint(path, distance, elementHit, face);

                } else {
                    // If we didn't intersect, but this was a diffusion ray, then we will go ahead and cast a short ray out
                    // from our last known point, in the last known direction, and leave that sound source hanging there
                    if (path->isDiffusion) {
                        const float MINIMUM_RANDOM_DISTANCE = 0.25f;
                        const float MAXIMUM_RANDOM_DISTANCE = 0.5f;
                        float distance = randFloatInRange(MINIMUM_RANDOM_DISTANCE, MAXIMUM_RANDOM_DISTANCE);
                        handlePathPoint(path, distance, NULL, UNKNOWN_FACE);
                    } else {
                        path->finalized = true; // if it doesn't intersect, then it is finished
                    }
                }
            }
        }
    }
    return activePaths;
}

void AudioReflector::handlePathPoint(AudioPath* path, float distance, OctreeElement* elementHit, BoxFace face) {
    glm::vec3 start = path->lastPoint;
    glm::vec3 direction = path->lastDirection;
    glm::vec3 end = start + (direction * (distance * SLIGHTLY_SHORT));

    float currentReflectiveAttenuation = path->lastAttenuation; // only the reflective components
    float currentDelay = path->lastDelay; // start with our delay so far
    float pathDistance = path->lastDistance;

    pathDistance += glm::distance(start, end);

    float toListenerDistance = glm::distance(end, _listenerPosition);

    // adjust our current delay by just the delay from the most recent ray
    currentDelay += getDelayFromDistance(distance);

    // now we know the current attenuation for the "perfect" reflection case, but we now incorporate
    // our surface materials to determine how much of this ray is absorbed, reflected, and diffused
    SurfaceCharacteristics material = getSurfaceCharacteristics(elementHit);

    float reflectiveAttenuation = currentReflectiveAttenuation * material.reflectiveRatio;
    float totalDiffusionAttenuation = currentReflectiveAttenuation * material.diffusionRatio;
    
    bool wantDiffusions = Menu::getInstance()->isOptionChecked(MenuOption::AudioSpatialProcessingWithDiffusions);
    int fanout = wantDiffusions ? _diffusionFanout : 0;

    float partialDiffusionAttenuation = fanout < 1 ? 0.0f : totalDiffusionAttenuation / (float)fanout;

    // total delay includes the bounce back to listener
    float totalDelay = currentDelay + getDelayFromDistance(toListenerDistance);
    float toListenerAttenuation = getDistanceAttenuationCoefficient(toListenerDistance + pathDistance);

    // if our resulting partial diffusion attenuation, is still above our minimum attenuation
    // then we add new paths for each diffusion point
    if ((partialDiffusionAttenuation * toListenerAttenuation) > MINIMUM_ATTENUATION_TO_REFLECT
            && totalDelay < MAXIMUM_DELAY_MS) {
            
        // diffusions fan out from random places on the semisphere of the collision point
        for(int i = 0; i < fanout; i++) {
            glm::vec3 diffusion;
            
            // We're creating a random normal here. But we want it to be relatively dramatic compared to how we handle
            // our slightly random surface normals.
            const float MINIMUM_RANDOM_LENGTH = 0.5f;
            const float MAXIMUM_RANDOM_LENGTH = 1.0f;
            float randomness = randFloatInRange(MINIMUM_RANDOM_LENGTH, MAXIMUM_RANDOM_LENGTH);
            float remainder = (1.0f - randomness)/2.0f;
            float remainderSignA = randomSign();
            float remainderSignB = randomSign();

            if (face == MIN_X_FACE) {
                diffusion = glm::vec3(-randomness, remainder * remainderSignA, remainder * remainderSignB);
            } else if (face == MAX_X_FACE) {
                diffusion = glm::vec3(randomness, remainder * remainderSignA, remainder * remainderSignB);
            } else if (face == MIN_Y_FACE) {
                diffusion = glm::vec3(remainder * remainderSignA, -randomness, remainder * remainderSignB);
            } else if (face == MAX_Y_FACE) {
                diffusion = glm::vec3(remainder * remainderSignA, randomness, remainder * remainderSignB);
            } else if (face == MIN_Z_FACE) {
                diffusion = glm::vec3(remainder * remainderSignA, remainder * remainderSignB, -randomness);
            } else if (face == MAX_Z_FACE) {
                diffusion = glm::vec3(remainder * remainderSignA, remainder * remainderSignB, randomness);
            } else if (face == UNKNOWN_FACE) {
                float randomnessX = randFloatInRange(MINIMUM_RANDOM_LENGTH, MAXIMUM_RANDOM_LENGTH);
                float randomnessY = randFloatInRange(MINIMUM_RANDOM_LENGTH, MAXIMUM_RANDOM_LENGTH);
                float randomnessZ = randFloatInRange(MINIMUM_RANDOM_LENGTH, MAXIMUM_RANDOM_LENGTH);
                diffusion = glm::vec3(direction.x * randomnessX, direction.y * randomnessY, direction.z * randomnessZ);
            }
            
            diffusion = glm::normalize(diffusion);

            // add new audio path for these diffusions, the new path's source is the same as the original source
            addAudioPath(path->source, end, diffusion, partialDiffusionAttenuation, currentDelay, pathDistance, true);
        }
    } else {
        const bool wantDebugging = false;
        if (wantDebugging) {
            if ((partialDiffusionAttenuation * toListenerAttenuation) <= MINIMUM_ATTENUATION_TO_REFLECT) {
                qDebug() << "too quiet to diffuse";
                qDebug() << "   partialDiffusionAttenuation=" << partialDiffusionAttenuation;
                qDebug() << "   toListenerAttenuation=" << toListenerAttenuation;
                qDebug() << "   result=" << (partialDiffusionAttenuation * toListenerAttenuation);
                qDebug() << "   MINIMUM_ATTENUATION_TO_REFLECT=" << MINIMUM_ATTENUATION_TO_REFLECT;
            }
            if (totalDelay > MAXIMUM_DELAY_MS) {
                qDebug() << "too delayed to diffuse";
                qDebug() << "   totalDelay=" << totalDelay;
                qDebug() << "   MAXIMUM_DELAY_MS=" << MAXIMUM_DELAY_MS;
            }
        }
    }

    // if our reflective attenuation is above our minimum, then add our reflection point and
    // allow our path to continue
    if (((reflectiveAttenuation + totalDiffusionAttenuation) * toListenerAttenuation) > MINIMUM_ATTENUATION_TO_REFLECT
            && totalDelay < MAXIMUM_DELAY_MS) {

        // add this location, as the reflective attenuation as well as the total diffusion attenuation
        // NOTE: we add the delay to the audible point, not back to the listener. The additional delay
        // and attenuation to the listener is recalculated at the point where we actually inject the
        // audio so that it can be adjusted to ear position
        AudiblePoint point = {end, currentDelay, (reflectiveAttenuation + totalDiffusionAttenuation), pathDistance};

        QVector<AudiblePoint>& audiblePoints = path->source == INBOUND_AUDIO ? _inboundAudiblePoints : _localAudiblePoints;

        audiblePoints.push_back(point);
    
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
        const bool wantDebugging = false;
        if (wantDebugging) {
            if (((reflectiveAttenuation + totalDiffusionAttenuation) * toListenerAttenuation) <= MINIMUM_ATTENUATION_TO_REFLECT) {
                qDebug() << "too quiet to add audible point";
                qDebug() << "   reflectiveAttenuation + totalDiffusionAttenuation=" << (reflectiveAttenuation + totalDiffusionAttenuation);
                qDebug() << "   toListenerAttenuation=" << toListenerAttenuation;
                qDebug() << "   result=" << ((reflectiveAttenuation + totalDiffusionAttenuation) * toListenerAttenuation);
                qDebug() << "   MINIMUM_ATTENUATION_TO_REFLECT=" << MINIMUM_ATTENUATION_TO_REFLECT;
            }
            if (totalDelay > MAXIMUM_DELAY_MS) {
                qDebug() << "too delayed to add audible point";
                qDebug() << "   totalDelay=" << totalDelay;
                qDebug() << "   MAXIMUM_DELAY_MS=" << MAXIMUM_DELAY_MS;
            }
        }
        path->finalized = true; // if we're too quiet, then we're done
    }
}

// TODO: eventually we will add support for different surface characteristics based on the element
// that is hit, which is why we pass in the elementHit to this helper function. But for now, all
// surfaces have the same characteristics
SurfaceCharacteristics AudioReflector::getSurfaceCharacteristics(OctreeElement* elementHit) {
    SurfaceCharacteristics result = { getReflectiveRatio(), _absorptionRatio, _diffusionRatio };
    return result;
}

void AudioReflector::setReflectiveRatio(float ratio) { 
    float safeRatio = std::max(0.0f, std::min(ratio, 1.0f));
    float currentReflectiveRatio = (1.0f - (_absorptionRatio + _diffusionRatio));
    float halfDifference = (safeRatio - currentReflectiveRatio) / 2.0f;

    // evenly distribute the difference between the two other ratios
    _absorptionRatio -= halfDifference;
    _diffusionRatio -= halfDifference;
}

void AudioReflector::setAbsorptionRatio(float ratio) { 
    float safeRatio = std::max(0.0f, std::min(ratio, 1.0f));
    _absorptionRatio = safeRatio;
    const float MAX_COMBINED_RATIO = 1.0f;
    if (_absorptionRatio + _diffusionRatio > MAX_COMBINED_RATIO) {
        _diffusionRatio = MAX_COMBINED_RATIO - _absorptionRatio;
    }
}

void AudioReflector::setDiffusionRatio(float ratio) { 
    float safeRatio = std::max(0.0f, std::min(ratio, 1.0f));
    _diffusionRatio = safeRatio;
    const float MAX_COMBINED_RATIO = 1.0f;
    if (_absorptionRatio + _diffusionRatio > MAX_COMBINED_RATIO) {
        _absorptionRatio = MAX_COMBINED_RATIO - _diffusionRatio;
    }
}

