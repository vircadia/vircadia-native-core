//
//  AnimationLoop.h
//  libraries/animation/src/
//
//  Created by Brad Hefta-Gaub on 11/12/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimationLoop_h
#define hifi_AnimationLoop_h

class AnimationDetails;

class AnimationLoop {
public:
    static const float MAXIMUM_POSSIBLE_FRAME;

    AnimationLoop();
    explicit AnimationLoop(const AnimationDetails& animationDetails);
    AnimationLoop(float fps, bool loop, bool hold, bool startAutomatically, float firstFrame, 
                  float lastFrame, bool running, float currentFrame);

    void setFPS(float fps) { _fps = fps; }
    float getFPS() const { return _fps; }

    void setLoop(bool loop) { _loop = loop; }
    bool getLoop() const { return _loop; }
    
    void setHold(bool hold) { _hold = hold; }
    bool getHold() const { return _hold; }
    
    void setStartAutomatically(bool startAutomatically);
    bool getStartAutomatically() const { return _startAutomatically; }
    
    void setFirstFrame(float firstFrame) { _firstFrame = glm::clamp(firstFrame, 0.0f, MAXIMUM_POSSIBLE_FRAME); }
    float getFirstFrame() const { return _firstFrame; }
    
    void setLastFrame(float lastFrame) { _lastFrame = glm::clamp(lastFrame, 0.0f, MAXIMUM_POSSIBLE_FRAME); }
    float getLastFrame() const { return _lastFrame; }
    
    /// by default the AnimationLoop will always reset to the first frame on any call to setRunning
    /// this is not desirable in the case of entities with animations, if you don't want the reset
    /// to happen then call this method with false;
    void setResetOnRunning(bool resetOnRunning) { _resetOnRunning = resetOnRunning; }
    void setRunning(bool running);
    bool getRunning() const { return _running; }

    void setCurrentFrame(float currentFrame) { _currentFrame = glm::clamp(currentFrame, _firstFrame, _lastFrame); }
    float getCurrentFrame() const { return _currentFrame; }

    void setMaxFrameIndexHint(float value) { _maxFrameIndexHint = glm::clamp(value, 0.0f, MAXIMUM_POSSIBLE_FRAME); }
    float getMaxFrameIndexHint() const { return _maxFrameIndexHint; }
    
    void start() { setRunning(true); }
    void stop() { setRunning(false); }
    void simulate(float deltaTime); /// call this with deltaTime if you as the caller are managing the delta time between calls

    void simulateAtTime(quint64 now); /// call this with "now" if you want the animationLoop to handle delta times

private:
    float _fps;
    bool _loop;
    bool _hold;
    bool _startAutomatically;
    float _firstFrame;
    float _lastFrame;
    bool _running;
    float _currentFrame;
    float _maxFrameIndexHint;
    bool _resetOnRunning;
    quint64 _lastSimulated;
};

#endif // hifi_AnimationLoop_h
