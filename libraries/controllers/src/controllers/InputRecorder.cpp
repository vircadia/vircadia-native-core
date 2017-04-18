//
//  Created by Dante Ruiz 2017/04/16
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InputRecorder.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <GLMHelpers.h>
namespace controller {

    void poseToJsonObject(const Pose pose) {
    } 
 
    InputRecorder::InputRecorder() {}

    InputRecorder::~InputRecorder() {}

    InputRecorder* InputRecorder::getInstance() {
        static InputRecorder inputRecorder;
        return &inputRecorder;
    }

    void InputRecorder::startRecording() {
        _recording = true;
        _playback = false;
        _framesRecorded = 0;
        _poseStateList.clear();
        _actionStateList.clear();
    }

    void InputRecorder::saveRecording() {
        QJsonObject data;
        data["frameCount"] = _framesRecorded;

        QJsonArray actionArrayList;
        QJsonArray poseArrayList;
        for(const ActionStates actionState _actionStateList) {
            QJsonArray actionArray;
            for (const float value, actionState) {
                actionArray.append(value);
            }
            actionArrayList.append(actionArray);
        }

        for (const PoseStates poseState, _poseStateList) {
            QJsonArray poseArray;
            for (const Pose pose, poseState) {
                
            }
            poseArrayList.append(poseArray);
        }
            
    }

    void InputRecorder::loadRecording() {
    }

    void InputRecorder::stopRecording() {
        _recording = false;
    }

    void InputRecorder::startPlayback() {
        _playback = true;
        _recording = false;
    }

    void InputRecorder::stopPlayback() {
        _playback = false;
    }

    void InputRecorder::setActionState(controller::Action action, float value) {
        if (_recording) {
            _currentFrameActions[toInt(action)] += value;
        }
    }

    void InputRecorder::setActionState(controller::Action action, const controller::Pose pose) {
        if (_recording) {
            _currentFramePoses[toInt(action)] = pose;
        }
    }

    void InputRecorder::resetFrame() {
        if (_recording) {
            for(auto& channel : _currentFramePoses) {
                channel = Pose();
            }
            
            for(auto& channel : _currentFrameActions) {
                channel = 0.0f;
            }
        }
    }
    
    float InputRecorder::getActionState(controller::Action action) {
        if (_actionStateList.size() > 0 ) {
            return _actionStateList[_playCount][toInt(action)];
        }

        return 0.0f;
    }

    controller::Pose InputRecorder::getPoseState(controller::Action action) {
        if (_poseStateList.size() > 0) {
            return _poseStateList[_playCount][toInt(action)];
        }

        return Pose();
    }

    void InputRecorder::frameTick() {
        if (_recording) {
            _framesRecorded++;
            _poseStateList.push_back(_currentFramePoses);
            _actionStateList.push_back(_currentFrameActions);
        }

        if (_playback) {
            _playCount++;
            if (_playCount == _framesRecorded) {
                _playCount = 0;
            }
        }
    }
}
