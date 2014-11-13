//
//  AnimationLoop.h
//  libraries/script-engine/src/
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
    
    void setFirstFrame(float firstFrame) { _firstFrame = firstFrame; }
    float getFirstFrame() const { return _firstFrame; }
    
    void setLastFrame(float lastFrame) { _lastFrame = lastFrame; }
    float getLastFrame() const { return _lastFrame; }
    
    void setRunning(bool running);
    bool isRunning() const { return _running; }

    void setFrameIndex(float frameIndex) { _frameIndex = glm::clamp(frameIndex, _firstFrame, _lastFrame); }
    float getFrameIndex() const { return _frameIndex; }
    
    void start() { setRunning(true); }
    void stop() { setRunning(false); }
    void simulate(float deltaTime);
    
private:
    float _fps;
    bool _loop;
    bool _hold;
    bool _startAutomatically;
    float _firstFrame;
    float _lastFrame;
    bool _running;
    float _frameIndex;
};

#endif // hifi_AnimationLoop_h
