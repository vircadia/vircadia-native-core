//
//  Created by Dante Ruiz on 2017/04/16
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InputRecorder_h
#define hifi_InputRecorder_h

#include <mutex>
#include <atomic>
#include <vector>

#include <QString>

#include "Pose.h"
#include "Actions.h"

namespace controller {
    class InputRecorder {
    public:
        using PoseStates = std::vector<Pose>;
        using ActionStates = std::vector<float>;
    
        InputRecorder();
        ~InputRecorder();

        static InputRecorder* getInstance();

        void saveRecording();
        void loadRecording(const QString& path);
        void startRecording();
        void startPlayback();
        void stopPlayback();
        void stopRecording();
        void toggleRecording() { _recording = !_recording; }
        void togglePlayback() { _playback = !_playback; }
        void resetFrame();
        bool isRecording() { return _recording; }
        bool isPlayingback() { return (_playback && !_loading); }
        void setActionState(controller::Action action, float value);
        void setActionState(controller::Action action, const controller::Pose pose);
        float getActionState(controller::Action action);
        controller::Pose getPoseState(controller::Action action);
        QString getSaveDirectory();
        void frameTick();
    private:
        bool _recording { false };
        bool _playback { false };
        bool _loading { false };
        std::vector<PoseStates> _poseStateList = std::vector<PoseStates>();
        std::vector<ActionStates> _actionStateList = std::vector<ActionStates>();
        PoseStates _currentFramePoses = PoseStates(toInt(Action::NUM_ACTIONS));
        ActionStates _currentFrameActions = ActionStates(toInt(Action::NUM_ACTIONS));
        
        int _framesRecorded { 0 };
        int _playCount { 0 };
    };
}    
#endif
