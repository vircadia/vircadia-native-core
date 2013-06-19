//
//  Audio.cpp
//  interface
//
//  Created by Stephen Birarda on 1/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#ifndef _WIN32

#include <iostream>
#include <fstream>
#include <pthread.h>
#include <sys/stat.h>
#include <cstring>

#include <StdDev.h>
#include <UDPSocket.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <AgentList.h>
#include <AgentTypes.h>
#include <AngleUtil.h>

#include "Application.h"
#include "Audio.h"
#include "Util.h"
#include "Log.h"

#define DEBUG_ECHO_CANCELLATION

static const int NUM_AUDIO_CHANNELS = 2;

static const int PACKET_LENGTH_BYTES = 1024;
static const int PACKET_LENGTH_BYTES_PER_CHANNEL = PACKET_LENGTH_BYTES / 2;
static const int PACKET_LENGTH_SAMPLES = PACKET_LENGTH_BYTES / sizeof(int16_t);
static const int PACKET_LENGTH_SAMPLES_PER_CHANNEL = PACKET_LENGTH_SAMPLES / 2;

static const int PHASE_DELAY_AT_90 = 20;
static const float AMPLITUDE_RATIO_AT_90 = 0.5;

static const int MIN_FLANGE_EFFECT_THRESHOLD = 600;
static const int MAX_FLANGE_EFFECT_THRESHOLD = 1500;
static const float FLANGE_BASE_RATE = 4;
static const float MAX_FLANGE_SAMPLE_WEIGHT = 0.50;
static const float MIN_FLANGE_INTENSITY = 0.25;

static const float JITTER_BUFFER_LENGTH_MSECS = 12;
static const short JITTER_BUFFER_SAMPLES = JITTER_BUFFER_LENGTH_MSECS *
                                           NUM_AUDIO_CHANNELS * (SAMPLE_RATE / 1000.0);

static const float AUDIO_CALLBACK_MSECS = (float)BUFFER_LENGTH_SAMPLES_PER_CHANNEL / (float)SAMPLE_RATE * 1000.0;

static const int AGENT_LOOPBACK_MODIFIER = 307;

// Speex preprocessor and echo canceller adaption
static const int   AEC_N_CHANNELS_MIC = 1;                                      // Number of microphone channels
static const int   AEC_N_CHANNELS_PLAY = 2;                                     // Number of speaker channels
static const int   AEC_FILTER_LENGTH = BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 8;   // Width of the filter
static const int   AEC_BUFFERED_FRAMES = 6;                                     // Maximum number of frames to buffer
static const int   AEC_BUFFERED_SAMPLES_PER_CHANNEL = BUFFER_LENGTH_SAMPLES_PER_CHANNEL * AEC_BUFFERED_FRAMES;
static const int   AEC_BUFFERED_SAMPLES = AEC_BUFFERED_SAMPLES_PER_CHANNEL * AEC_N_CHANNELS_PLAY;
static const int   AEC_TMP_BUFFER_SIZE = (AEC_N_CHANNELS_MIC +                  // Temporary space for processing a
        AEC_N_CHANNELS_PLAY) * BUFFER_LENGTH_SAMPLES_PER_CHANNEL;               //  single frame

// Speex preprocessor and echo canceller configuration
static const int   AEC_NOISE_REDUCTION = -400;                                  // Noise reduction (important)
static const int   AEC_RESIDUAL_ECHO_REDUCTION = -60;                           // Residual echo reduction
static const int   AEC_RESIDUAL_ECHO_REDUCTION_ACTIVE = -40;                    //  ~on active side
static const bool  AEC_USE_AGC = true;                                          // Automatic gain control
static const int   AEC_AGC_MAX_GAIN = -12;                                      // Gain in db
static const int   AEC_AGC_TARGET_LEVEL = 20000;                                // Target reference level
static const int   AEC_AGC_MAX_INC = 6;                                         // Max increase in db/s
static const int   AEC_AGC_MAX_DEC = 40;                                        // Max decrease in db/s
static const bool  AEC_USE_VAD = false;                                         // Voice activity determination

// Delay test (performed before using speex)
static const float AEC_PING_PITCH = 16.f;                                       // Ping wavelength, # samples / radian
static const float AEC_PING_VOLUME = 32000.f;                                   // Ping peak amplitude
static const int   AEC_PING_RETRY = 3;                                          // Number of retries for EC calibration
static const int   AEC_PING_MIN_AMPLI = 225;                                    // Minimum amplitude for EC calibration
static const int   AEC_PING_MAX_PERIOD_DIFFERENCE = 15;                         // Maximum # samples from expected period
static const int   AEC_PING_PERIOD = int(Radians::twicePi() * AEC_PING_PITCH);  // Sine period based on the given pitch
static const int   AEC_PING_HALF_PERIOD = int(Radians::pi() * AEC_PING_PITCH);  // Distance between extrema
static const int   AEC_PING_BUFFER_OFFSET = BUFFER_LENGTH_SAMPLES_PER_CHANNEL - AEC_PING_PERIOD * 2.0f; // Signal start


inline void Audio::performIO(int16_t* inputLeft, int16_t* outputLeft, int16_t* outputRight) {

    AgentList* agentList = AgentList::getInstance();
    Application* interface = Application::getInstance();
    Avatar* interfaceAvatar = interface->getAvatar();

    eventuallyCancelEcho(inputLeft);
 
    // Add Procedural effects to input samples
    addProceduralSounds(inputLeft, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
    
    if (agentList && inputLeft) {
        
        //  Measure the loudness of the signal from the microphone and store in audio object
        float loudness = 0;
        for (int i = 0; i < BUFFER_LENGTH_SAMPLES_PER_CHANNEL; i++) {
            loudness += abs(inputLeft[i]);
        }
        
        loudness /= BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
        _lastInputLoudness = loudness;
        
        // add input (@microphone) data to the scope
#ifndef DEBUG_ECHO_CANCELLATION
        _scope->addSamples(0, inputLeft, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
#endif
        Agent* audioMixer = agentList->soloAgentOfType(AGENT_TYPE_AUDIO_MIXER);
        
        if (audioMixer) {
            glm::vec3 headPosition = interfaceAvatar->getHeadJointPosition();
            glm::quat headOrientation = interfaceAvatar->getHead().getOrientation();
            
            int leadingBytes = sizeof(PACKET_HEADER_MICROPHONE_AUDIO_NO_ECHO) + sizeof(headPosition) + sizeof(headOrientation);
            
            // we need the amount of bytes in the buffer + 1 for type
            // + 12 for 3 floats for position + float for bearing + 1 attenuation byte
            unsigned char dataPacket[BUFFER_LENGTH_BYTES_PER_CHANNEL + leadingBytes];
            
            dataPacket[0] = (Application::getInstance()->shouldEchoAudio())
                ? PACKET_HEADER_MICROPHONE_AUDIO_WITH_ECHO
                : PACKET_HEADER_MICROPHONE_AUDIO_NO_ECHO;
            unsigned char *currentPacketPtr = dataPacket + 1;
            
            // memcpy the three float positions
            memcpy(currentPacketPtr, &headPosition, sizeof(headPosition));
            currentPacketPtr += (sizeof(headPosition));
            
            // memcpy our orientation
            memcpy(currentPacketPtr, &headOrientation, sizeof(headOrientation));
            currentPacketPtr += sizeof(headOrientation);
            
            // copy the audio data to the last BUFFER_LENGTH_BYTES bytes of the data packet
            memcpy(currentPacketPtr, inputLeft, BUFFER_LENGTH_BYTES_PER_CHANNEL);
            
            agentList->getAgentSocket()->send(audioMixer->getActiveSocket(),
                                              dataPacket,
                                              BUFFER_LENGTH_BYTES_PER_CHANNEL + leadingBytes);
        }
        
    }
    
    memset(outputLeft, 0, PACKET_LENGTH_BYTES_PER_CHANNEL);
    memset(outputRight, 0, PACKET_LENGTH_BYTES_PER_CHANNEL);
    
    AudioRingBuffer* ringBuffer = &_ringBuffer;
    
    // if we've been reset, and there isn't any new packets yet
    // just play some silence
    
    if (ringBuffer->getEndOfLastWrite()) {
        
        if (!ringBuffer->isStarted() && ringBuffer->diffLastWriteNextOutput() < PACKET_LENGTH_SAMPLES + JITTER_BUFFER_SAMPLES) {
//            printLog("Held back, buffer has %d of %d samples required.\n",
//                     ringBuffer->diffLastWriteNextOutput(), PACKET_LENGTH_SAMPLES + JITTER_BUFFER_SAMPLES);
        } else if (ringBuffer->diffLastWriteNextOutput() < PACKET_LENGTH_SAMPLES) {
            ringBuffer->setStarted(false);
            
            _numStarves++;
            _packetsReceivedThisPlayback = 0;
            
            // printLog("Starved #%d\n", starve_counter);
            _wasStarved = 10;      //   Frames to render the indication that the system was starved.
        } else {
            if (!ringBuffer->isStarted()) {
                ringBuffer->setStarted(true);
                // printLog("starting playback %3.1f msecs delayed \n", (usecTimestampNow() - usecTimestamp(&firstPlaybackTimer))/1000.0);
            } else {
                // printLog("pushing buffer\n");
            }
            // play whatever we have in the audio buffer
            
            // if we haven't fired off the flange effect, check if we should
            // TODO: lastMeasuredHeadYaw is now relative to body - check if this still works.
            
            int lastYawMeasured = fabsf(interfaceAvatar->getHeadYawRate());
            
            if (!_samplesLeftForFlange && lastYawMeasured > MIN_FLANGE_EFFECT_THRESHOLD) {
                // we should flange for one second
                if ((_lastYawMeasuredMaximum = std::max(_lastYawMeasuredMaximum, lastYawMeasured)) != lastYawMeasured) {
                    _lastYawMeasuredMaximum = std::min(_lastYawMeasuredMaximum, MIN_FLANGE_EFFECT_THRESHOLD);
                    
                    _samplesLeftForFlange = SAMPLE_RATE;
                    
                    _flangeIntensity = MIN_FLANGE_INTENSITY +
                        ((_lastYawMeasuredMaximum - MIN_FLANGE_EFFECT_THRESHOLD) /
                         (float)(MAX_FLANGE_EFFECT_THRESHOLD - MIN_FLANGE_EFFECT_THRESHOLD)) *
                        (1 - MIN_FLANGE_INTENSITY);
                    
                    _flangeRate = FLANGE_BASE_RATE * _flangeIntensity;
                    _flangeWeight = MAX_FLANGE_SAMPLE_WEIGHT * _flangeIntensity;
                }
            }
            
            for (int s = 0; s < PACKET_LENGTH_SAMPLES_PER_CHANNEL; s++) {
                
                int leftSample = ringBuffer->getNextOutput()[s];
                int rightSample = ringBuffer->getNextOutput()[s + PACKET_LENGTH_SAMPLES_PER_CHANNEL];
                
                if (_samplesLeftForFlange > 0) {
                    float exponent = (SAMPLE_RATE - _samplesLeftForFlange - (SAMPLE_RATE / _flangeRate)) /
                        (SAMPLE_RATE / _flangeRate);
                    int sampleFlangeDelay = (SAMPLE_RATE / (1000 * _flangeIntensity)) * powf(2, exponent);
                    
                    if (_samplesLeftForFlange != SAMPLE_RATE || s >= (SAMPLE_RATE / 2000)) {
                        // we have a delayed sample to add to this sample
                        
                        int16_t *flangeFrame = ringBuffer->getNextOutput();
                        int flangeIndex = s - sampleFlangeDelay;
                        
                        if (flangeIndex < 0) {
                            // we need to grab the flange sample from earlier in the buffer
                            flangeFrame = ringBuffer->getNextOutput() != ringBuffer->getBuffer()
                            ? ringBuffer->getNextOutput() - PACKET_LENGTH_SAMPLES
                            : ringBuffer->getNextOutput() + RING_BUFFER_LENGTH_SAMPLES - PACKET_LENGTH_SAMPLES;
                            
                            flangeIndex = PACKET_LENGTH_SAMPLES_PER_CHANNEL + (s - sampleFlangeDelay);
                        }
                        
                        int16_t leftFlangeSample = flangeFrame[flangeIndex];
                        int16_t rightFlangeSample = flangeFrame[flangeIndex + PACKET_LENGTH_SAMPLES_PER_CHANNEL];
                        
                        leftSample = (1 - _flangeWeight) * leftSample + (_flangeWeight * leftFlangeSample);
                        rightSample = (1 - _flangeWeight) * rightSample + (_flangeWeight * rightFlangeSample);
                        
                        _samplesLeftForFlange--;
                        
                        if (_samplesLeftForFlange == 0) {
                            _lastYawMeasuredMaximum = 0;
                        }
                    }
                }
                
                outputLeft[s] = leftSample;
                outputRight[s] = rightSample;
            }
            ringBuffer->setNextOutput(ringBuffer->getNextOutput() + PACKET_LENGTH_SAMPLES);
            
            if (ringBuffer->getNextOutput() == ringBuffer->getBuffer() + RING_BUFFER_LENGTH_SAMPLES) {
                ringBuffer->setNextOutput(ringBuffer->getBuffer());
            }
        }
    }

    eventuallySendRecvPing(inputLeft, outputLeft, outputRight);
    eventuallyRecordEcho(outputLeft, outputRight);


    // add output (@speakers) data just written to the scope
#ifndef DEBUG_ECHO_CANCELLATION
    _scope->addSamples(1, outputLeft, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
    _scope->addSamples(2, outputRight, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
#endif

    gettimeofday(&_lastCallbackTime, NULL);
}

// inputBuffer  A pointer to an internal portaudio data buffer containing data read by portaudio.
// outputBuffer A pointer to an internal portaudio data buffer to be read by the configured output device.
// frames       Number of frames that portaudio requests to be read/written.
// timeInfo     Portaudio time info. Currently unused.
// statusFlags  Portaudio status flags. Currently unused.
// userData     Pointer to supplied user data (in this case, a pointer to the parent Audio object
int Audio::audioCallback (const void* inputBuffer,
                          void* outputBuffer,
                          unsigned long frames,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData) {
    
    int16_t* inputLeft = static_cast<int16_t*const*>(inputBuffer)[0];
    int16_t* outputLeft = static_cast<int16_t**>(outputBuffer)[0];
    int16_t* outputRight = static_cast<int16_t**>(outputBuffer)[1];

    static_cast<Audio*>(userData)->performIO(inputLeft, outputLeft, outputRight);
    return paContinue;
}


static void outputPortAudioError(PaError error) {
    if (error != paNoError) {
        printLog("-- portaudio termination error --\n");
        printLog("PortAudio error (%d): %s\n", error, Pa_GetErrorText(error));
    }
}

Audio::Audio(Oscilloscope* scope) :
    _stream(NULL),
    _speexEchoState(NULL),
    _speexPreprocessState(NULL),
    _ringBuffer(true),
    _scope(scope),
    _averagedLatency(0.0),
    _measuredJitter(0),
    _jitterBufferLengthMsecs(12.0),
//    _jitterBufferSamples(_jitterBufferLengthMsecs *
//                     NUM_AUDIO_CHANNELS * (SAMPLE_RATE / 1000.0)),
    _wasStarved(0),
    _numStarves(0),
    _lastInputLoudness(0),
    _lastVelocity(0),
    _lastAcceleration(0),
    _totalPacketsReceived(0),
    _firstPlaybackTime(),
    _packetsReceivedThisPlayback(0),
    _isCancellingEcho(false),
    _isSendingEchoPing(false),
    _echoAnalysisPending(false),
    _echoInputFramesToRecord(0),
    _samplesLeftForFlange(0),
    _lastYawMeasuredMaximum(0),
    _flangeIntensity(0.0f),
    _flangeRate(0.0f),
    _flangeWeight(0.0f)
{
    outputPortAudioError(Pa_Initialize());
    outputPortAudioError(Pa_OpenDefaultStream(&_stream,
                                              2,
                                              2,
                                              (paInt16 | paNonInterleaved),
                                              SAMPLE_RATE,
                                              BUFFER_LENGTH_SAMPLES_PER_CHANNEL,
                                              audioCallback,
                                              (void*) this));

    if (! _stream) {
        return;
    }
 
    _echoSamplesLeft = new int16_t[AEC_BUFFERED_SAMPLES + AEC_TMP_BUFFER_SIZE];
    if (! _echoSamplesLeft) {
        return;
    }
    memset(_echoSamplesLeft, 0, AEC_BUFFERED_SAMPLES * sizeof(int16_t));
    _echoSamplesRight = _echoSamplesLeft + AEC_BUFFERED_SAMPLES_PER_CHANNEL;
    _speexTmpBuf = _echoSamplesRight + AEC_BUFFERED_SAMPLES_PER_CHANNEL;

    _speexPreprocessState = speex_preprocess_state_init(BUFFER_LENGTH_SAMPLES_PER_CHANNEL, SAMPLE_RATE);
    if (_speexPreprocessState) {
        _speexEchoState = speex_echo_state_init_mc(BUFFER_LENGTH_SAMPLES_PER_CHANNEL, 
                                                   AEC_FILTER_LENGTH, AEC_N_CHANNELS_MIC, AEC_N_CHANNELS_PLAY);
        if (_speexEchoState) {
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_ECHO_STATE, _speexEchoState);
            int tmp;
            speex_echo_ctl(_speexEchoState, SPEEX_ECHO_SET_SAMPLING_RATE, &(tmp = SAMPLE_RATE));
            tmp = AEC_NOISE_REDUCTION;
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &tmp);
            tmp = AEC_RESIDUAL_ECHO_REDUCTION;
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &tmp);
            tmp = AEC_RESIDUAL_ECHO_REDUCTION_ACTIVE;
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &tmp);
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_AGC, &(tmp = int(AEC_USE_AGC)));
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &(tmp = AEC_AGC_MAX_GAIN));
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_AGC_TARGET, &(tmp = AEC_AGC_TARGET_LEVEL));
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &(tmp = AEC_AGC_MAX_INC));
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &(tmp = AEC_AGC_MAX_DEC));
            speex_preprocess_ctl(_speexPreprocessState, SPEEX_PREPROCESS_SET_VAD, &(tmp = int(AEC_USE_VAD)));
        } else {
            speex_preprocess_state_destroy(_speexPreprocessState);
            _speexPreprocessState = NULL;
        }
    }

    // start the stream now that sources are good to go
    outputPortAudioError(Pa_StartStream(_stream));

    gettimeofday(&_lastReceiveTime, NULL);
}

Audio::~Audio() {    
    if (_stream) {
        outputPortAudioError(Pa_CloseStream(_stream));
        outputPortAudioError(Pa_Terminate());
    }
    if (_speexEchoState) {
        speex_preprocess_state_destroy(_speexPreprocessState);
        speex_echo_state_destroy(_speexEchoState);
    }
    delete[] _echoSamplesLeft;
}

void Audio::addReceivedAudioToBuffer(unsigned char* receivedData, int receivedBytes) {
    const int NUM_INITIAL_PACKETS_DISCARD = 3;
    
    timeval currentReceiveTime;
    gettimeofday(&currentReceiveTime, NULL);
    _totalPacketsReceived++;
    
    double timeDiff = diffclock(&_lastReceiveTime, &currentReceiveTime);
    
    //  Discard first few received packets for computing jitter (often they pile up on start)
    if (_totalPacketsReceived > NUM_INITIAL_PACKETS_DISCARD) {
        _stdev.addValue(timeDiff);
    }
    
    if (_stdev.getSamples() > 500) {
        _measuredJitter = _stdev.getStDev();
        _stdev.reset();
    }
    
    if (!_ringBuffer.isStarted()) {
        _packetsReceivedThisPlayback++;
    }
    
    if (_packetsReceivedThisPlayback == 1) {
        gettimeofday(&_firstPlaybackTime, NULL);
    }
    
    _ringBuffer.parseData((unsigned char*) receivedData, PACKET_LENGTH_BYTES + sizeof(PACKET_HEADER));
    
    _lastReceiveTime = currentReceiveTime;
}

void Audio::render(int screenWidth, int screenHeight) {
    if (_stream) {
        glLineWidth(2.0);
        glBegin(GL_LINES);
        glColor3f(1,1,1);
        
        int startX = 20.0;
        int currentX = startX;
        int topY = screenHeight - 40;
        int bottomY = screenHeight - 20;
        float frameWidth = 20.0;
        float halfY = topY + ((bottomY - topY) / 2.0);
        
        // draw the lines for the base of the ring buffer
        
        glVertex2f(currentX, topY);
        glVertex2f(currentX, bottomY);
        
        for (int i = 0; i < RING_BUFFER_LENGTH_FRAMES; i++) {
            glVertex2f(currentX, halfY);
            glVertex2f(currentX + frameWidth, halfY);
            currentX += frameWidth;
            
            glVertex2f(currentX, topY);
            glVertex2f(currentX, bottomY);
        }
        glEnd();
        
        //  Show a bar with the amount of audio remaining in ring buffer beyond current playback
        float remainingBuffer = 0;
        timeval currentTime;
        gettimeofday(&currentTime, NULL);
        float timeLeftInCurrentBuffer = 0;
        if (_lastCallbackTime.tv_usec > 0) {
            timeLeftInCurrentBuffer = AUDIO_CALLBACK_MSECS - diffclock(&_lastCallbackTime, &currentTime);
        }
        
        if (_ringBuffer.getEndOfLastWrite() != NULL)
            remainingBuffer = _ringBuffer.diffLastWriteNextOutput() / PACKET_LENGTH_SAMPLES * AUDIO_CALLBACK_MSECS;
        
        if (_wasStarved == 0) {
            glColor3f(0, 1, 0);
        } else {
            glColor3f(0.5 + (_wasStarved / 20.0f), 0, 0);
            _wasStarved--;
        }
        
        glBegin(GL_QUADS);
        glVertex2f(startX, topY + 2);
        glVertex2f(startX + (remainingBuffer + timeLeftInCurrentBuffer)/AUDIO_CALLBACK_MSECS*frameWidth, topY + 2);
        glVertex2f(startX + (remainingBuffer + timeLeftInCurrentBuffer)/AUDIO_CALLBACK_MSECS*frameWidth, bottomY - 2);
        glVertex2f(startX, bottomY - 2);
        glEnd();
        
        if (_averagedLatency == 0.0) {
            _averagedLatency = remainingBuffer + timeLeftInCurrentBuffer;
        } else {
            _averagedLatency = 0.99f * _averagedLatency + 0.01f * (remainingBuffer + timeLeftInCurrentBuffer);
        }
        
        //  Show a yellow bar with the averaged msecs latency you are hearing (from time of packet receipt)
        glColor3f(1,1,0);
        glBegin(GL_QUADS);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth - 2, topY - 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth + 2, topY - 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth + 2, bottomY + 2);
        glVertex2f(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth - 2, bottomY + 2);
        glEnd();
        
        char out[40];
        sprintf(out, "%3.0f\n", _averagedLatency);
        drawtext(startX + _averagedLatency / AUDIO_CALLBACK_MSECS * frameWidth - 10, topY - 10, 0.10, 0, 1, 0, out, 1,1,0);
        //drawtext(startX + 0, topY-10, 0.08, 0, 1, 0, out, 1,1,0);
        
        //  Show a Cyan bar with the most recently measured jitter stdev
        
        int jitterPels = _measuredJitter / ((1000.0f * PACKET_LENGTH_SAMPLES / SAMPLE_RATE)) * frameWidth;
        
        glColor3f(0,1,1);
        glBegin(GL_QUADS);
        glVertex2f(startX + jitterPels - 2, topY - 2);
        glVertex2f(startX + jitterPels + 2, topY - 2);
        glVertex2f(startX + jitterPels + 2, bottomY + 2);
        glVertex2f(startX + jitterPels - 2, bottomY + 2);
        glEnd();
        
        sprintf(out,"%3.1f\n", _measuredJitter);
        drawtext(startX + jitterPels - 5, topY-10, 0.10, 0, 1, 0, out, 0,1,1);
        
        sprintf(out, "%3.1fms\n", JITTER_BUFFER_LENGTH_MSECS);
        drawtext(startX - 10, bottomY + 15, 0.1, 0, 1, 0, out, 1, 0, 0);
    }
}

//  Take a pointer to the acquired microphone input samples and add procedural sounds
void Audio::addProceduralSounds(int16_t* inputBuffer, int numSamples) {
    const float MAX_AUDIBLE_VELOCITY = 6.0;
    const float MIN_AUDIBLE_VELOCITY = 0.1;
    const int VOLUME_BASELINE = 400;
    const float SOUND_PITCH = 8.f;
    
    float speed = glm::length(_lastVelocity);
    float volume = VOLUME_BASELINE * (1.f - speed / MAX_AUDIBLE_VELOCITY);
    
    //  Add a noise-modulated sinewave with volume that tapers off with speed increasing
    if ((speed > MIN_AUDIBLE_VELOCITY) && (speed < MAX_AUDIBLE_VELOCITY)) {
        for (int i = 0; i < numSamples; i++) {
            inputBuffer[i] += (int16_t)((sinf((float) i / SOUND_PITCH * speed) * randFloat()) * volume * speed);
        }
    }
}

static inline void subScaled(int16_t* dst, const int16_t* src, unsigned n, int scale16fixpt) {

    for (int16_t* dstEnd = dst + n; dst != dstEnd; ++src, ++dst) {
        *dst -= int16_t((*src * scale16fixpt) >> 16);
    } 
}

inline void Audio::eventuallyCancelEcho(int16_t* inputLeft) {
    if (! _isCancellingEcho) {
        return;
    }

    // Construct an artificial frame from the captured playback
    // that contains the appropriately delayed output to cancel
    unsigned n = BUFFER_LENGTH_SAMPLES_PER_CHANNEL, n2 = 0;
    unsigned readPos = (_echoWritePos + AEC_BUFFERED_SAMPLES_PER_CHANNEL - _echoDelay) % AEC_BUFFERED_SAMPLES_PER_CHANNEL;
    unsigned readEnd = readPos + n;
    if (readEnd >= AEC_BUFFERED_SAMPLES_PER_CHANNEL) {
        n2 = (readEnd -= AEC_BUFFERED_SAMPLES_PER_CHANNEL);
        n -= n2;
    }
    // Use two subsequent buffers for the two stereo channels
    int16_t* playBufferLeft = _speexTmpBuf + BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
    memcpy(playBufferLeft, _echoSamplesLeft + readPos, n * sizeof(int16_t));
    memcpy(playBufferLeft + n, _echoSamplesLeft, n2 * sizeof(int16_t));
    int16_t* playBufferRight = playBufferLeft + BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
    memcpy(playBufferRight, _echoSamplesRight + readPos, n * sizeof(int16_t));
    memcpy(playBufferRight + n, _echoSamplesLeft, n2 * sizeof(int16_t));

#ifdef DEBUG_ECHO_CANCELLATION
    // Visualize the input
    _scope->addSamples(0, inputLeft, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
    _scope->addSamples(1, playBufferLeft, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
#endif

    // Have Speex perform echo cancellation
    speex_echo_cancellation(_speexEchoState, inputLeft, playBufferLeft, _speexTmpBuf);
    memcpy(inputLeft, _speexTmpBuf, BUFFER_LENGTH_BYTES_PER_CHANNEL);
    speex_preprocess_run(_speexPreprocessState, inputLeft); 

#ifdef DEBUG_ECHO_CANCELLATION
    // Visualize the result
    _scope->addSamples(2, inputLeft, BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
#endif
}

inline void Audio::eventuallyRecordEcho(int16_t* outputLeft, int16_t* outputRight) {
    if (! _isCancellingEcho) {
        return;
    }

    // Copy playback data to circular buffers
    unsigned n = BUFFER_LENGTH_SAMPLES_PER_CHANNEL, n2 = 0;
    unsigned writeEnd = _echoWritePos + n;
    if (writeEnd >= AEC_BUFFERED_SAMPLES_PER_CHANNEL) {
        n2 = (writeEnd -= AEC_BUFFERED_SAMPLES_PER_CHANNEL);
        n -= n2;
    }
    memcpy(_echoSamplesLeft + _echoWritePos, outputLeft, n * sizeof(int16_t));
    memcpy(_echoSamplesLeft, outputLeft + n, n2 * sizeof(int16_t));
    memcpy(_echoSamplesRight + _echoWritePos, outputRight, n * sizeof(int16_t));
    memcpy(_echoSamplesRight, outputRight + n, n2 * sizeof(int16_t));

    _echoWritePos = writeEnd;
}

void Audio::setIsCancellingEcho(bool enabled) {

    _isCancellingEcho = false;

    if (enabled) {

        // Request recalibration
        _echoPingRetries = AEC_PING_RETRY;
        _echoInputFramesToRecord = AEC_BUFFERED_FRAMES;
        _isSendingEchoPing = true;

        // _scope->setDownsampleRatio(8); // DEBUG
        // _scope->inputPaused = false; // DEBUG
    }
}

void Audio::testPing() {

    _echoInputFramesToRecord = 0;
    _isSendingEchoPing = true;
}

inline void Audio::eventuallySendRecvPing(int16_t* inputLeft, int16_t* outputLeft, int16_t* outputRight) {
/*
    // Artificial, local echo hack
    if (Application::getInstance()->shouldEchoAudio()) {

        enum { bufs = 32 };
        static int16_t buf[bufs][BUFFER_LENGTH_SAMPLES_PER_CHANNEL];
        static int bufIdx = 0;

        int wBuf = bufIdx;
        bufIdx = (bufIdx + 1) % bufs;
        memcpy(buf[wBuf], inputLeft, BUFFER_LENGTH_BYTES_PER_CHANNEL);
        subScaled(outputLeft, buf[bufIdx], BUFFER_LENGTH_SAMPLES_PER_CHANNEL, -0x7000);
    }
*/
    // Calibration of echo cancellation
    if (_isSendingEchoPing) {

        // Overwrite output with ping signal
        memset(outputLeft, 0, AEC_PING_BUFFER_OFFSET * sizeof(int16_t));
        outputLeft += AEC_PING_BUFFER_OFFSET;
        memset(outputRight, 0, AEC_PING_BUFFER_OFFSET * sizeof(int16_t));
        outputRight += AEC_PING_BUFFER_OFFSET;
        for (int s = -AEC_PING_PERIOD; s < AEC_PING_PERIOD; ++s) {
            float t = float(s) / AEC_PING_PITCH;
            // Use signed variant of sinc 
            // speaker-reproducible with a unique characteristic point in time
            *outputLeft++ = *outputRight++ = int16_t(AEC_PING_VOLUME * 
                    sinf(t) / fmaxf(1.0f, pow((abs(t)-1.5f) / 1.5f, 1.2f)));
        }

        // As of the next frame, we'll be recoding _echoInputFramesToRecord from the mic
        _isSendingEchoPing = false;
        printLog("Send audio ping\n");

    } else if (_echoInputFramesToRecord > 0) {

        // Store input samples
        int offset = BUFFER_LENGTH_SAMPLES_PER_CHANNEL * (
                AEC_BUFFERED_FRAMES - _echoInputFramesToRecord);
        memcpy(_echoSamplesLeft + offset, 
               inputLeft, BUFFER_LENGTH_SAMPLES_PER_CHANNEL * sizeof(int16_t));

        --_echoInputFramesToRecord;

        if (_echoInputFramesToRecord == 0) {
            _echoAnalysisPending = true;
            printLog("Received ping echo\n");
        }
    }
}

static int findExtremum(int16_t const* samples, int length, int sign) {

    int x0 = -1;
    int y0 = -AEC_PING_VOLUME;
    for (int x = 0; x < length; ++samples, ++x) {
        int y = *samples * sign;
        if (y > y0) {
            x0 = x;
            y0 = y;
        }
    }
    return x0;
}

bool Audio::calibrateEchoCancellation() {

    // Analyze received signal
    int botAt = findExtremum(_echoSamplesLeft, AEC_BUFFERED_SAMPLES_PER_CHANNEL, -1);
    if (botAt == -1) {
        printLog("AEC: Minimum not found.\n");
        return false;
    }
    int topAt = findExtremum(_echoSamplesLeft, AEC_BUFFERED_SAMPLES_PER_CHANNEL, 1);
    if (topAt == -1) {
        printLog("AEC: Maximum not found.\n");
        return false;
    }

    // Determine peak amplitude
    int ampli = (_echoSamplesLeft[topAt] - _echoSamplesLeft[botAt]) / 2;
    if (ampli < AEC_PING_MIN_AMPLI) {
        // We can't reliably calibrate and probably won't hear it, anyways.
        printLog("AEC: Amplitude too low %d.\n", ampli);
        return false;
    }

    // Determine period
    int halfPeriod = topAt - botAt;
    if (halfPeriod < 0) {
        printLog("AEC: Min/max inverted.\n");
        halfPeriod = -halfPeriod;
        topAt -= AEC_PING_PERIOD;
        ampli = -ampli;
    }
    if (abs(halfPeriod-AEC_PING_HALF_PERIOD) > AEC_PING_MAX_PERIOD_DIFFERENCE) {
        // Probably not our signal
        printLog("AEC: Unexpected period %d vs. %d\n", halfPeriod, AEC_PING_HALF_PERIOD);
        return false;
    }

    // Determine delay based on the characteristic center of the signal we found
    // (this value is too small by one packet minus ping length and it's good that
    // way as the initial movement will be before the peak)
    _echoDelay = (botAt + topAt) / 2;

    printLog("AEC:\ndelay = %d\namp = %d\ntopAt = %d\nbotAt = %d\n", _echoDelay, ampli, topAt, botAt); 
    return true;
}

bool Audio::eventuallyCalibrateEchoCancellation() {

    // Pending request -> process it
    if (! _echoAnalysisPending) {
        return false;
    }
    _echoAnalysisPending = false;

    if (calibrateEchoCancellation()) {
        // Success! Enable echo cancellation.
        _echoWritePos = 0;
        memset(_echoSamplesLeft, 0, AEC_BUFFERED_SAMPLES * sizeof(int16_t));
        _isCancellingEcho = true;
    }
    else if (--_echoPingRetries >= 0) {
        // Retry - better luck next time.
        _isSendingEchoPing = true;
        _echoInputFramesToRecord = AEC_BUFFERED_FRAMES;
        // _scope->inputPaused = false; // DEBUG
        return false;
    }
    // _scope->inputPaused = true; // DEBUG
    return true;
}

#endif
