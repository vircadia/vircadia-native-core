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
    AnimationLoop(const AnimationDetails& animationDetails);
    AnimationLoop(float fps, bool loop, bool hold, bool startAutomatically, float firstFrame, 
                    float lastFrame, bool running, float frameIndex);

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
    bool isRunning() const { return _running; }

    void setFrameIndex(float frameIndex) { _frameIndex = glm::clamp(frameIndex, _firstFrame, _lastFrame); }
    float getFrameIndex() const { return _frameIndex; }

    void setMaxFrameIndexHint(float value) { _maxFrameIndexHint = glm::clamp(value, 0.0f, MAXIMUM_POSSIBLE_FRAME); }
    float getMaxFrameIndexHint() const { return _maxFrameIndexHint; }
    
    void start() { setRunning(true); }
    void stop() { setRunning(false); }
    void simulate(float deltaTime);
    
private:
    float _fps = 30.0f;
    bool _loop = false;
    bool _hold = false;
    bool _startAutomatically = false;
    float _firstFrame = 0.0f;
    float _lastFrame = MAXIMUM_POSSIBLE_FRAME;
    bool _running = false;
    float _frameIndex = 0.0f;
    float _maxFrameIndexHint = MAXIMUM_POSSIBLE_FRAME;
    bool _resetOnRunning = false;
};

#endif // hifi_AnimationLoop_h
