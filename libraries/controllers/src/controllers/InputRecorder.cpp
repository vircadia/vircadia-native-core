//
//  Created by Dante Ruiz 2017/04/16
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InputRecorder.h"

namespace controller {
 
    InputRecorder::InputRecorder() {}

    InputRecorder::~InputRecorder() {}

    InputRecorder& InputRecorder::getInstance() {
        static InputRecorder inputRecorder;
        return inputRecorder;
    }

    void InputRecorder::startRecording() {
        _recording = true;
        _framesRecorded = 0;
        _poseStateList.clear();
        _actionStateList.clear();
        qDebug() << "-------------> input recording starting <---------------";
    }

    void InputRecorder::stopRecording() {
        _recording = false;
        qDebug() << "--------------> input recording stopping <-----------------";
    }

    void InputRecorder::startPlayback() {
        _playback = true;
        _recording = false;
        qDebug() << "-----------------> starting playback <---------------";
    }

    void InputRecorder::stopPlayback() {
        _playback = false;
        _recording = false;
    }

    void InputRecorder::setActionState(controller::Action action, float value) {
        if (_recording) {
            qDebug() << "-----------------> setiing action state <---------------";
            _actionStateList[_framesRecorded][toInt(action)] = value;
        }
    }

    void InputRecorder::setActionState(controller::Action action, const controller::Pose pose) {
        if (_recording) {
            qDebug() << "-----------------> setiing Pose state <---------------";
            _poseStateList[_framesRecorded][toInt(action)] = pose;
        }
    }

    float InputRecorder::getActionState(controller::Action action) {
        return _actionStateList[_playCount][toInt(action)];
    }

    controller::Pose InputRecorder::getPoseState(controller::Action action) {
        return _poseStateList[_playCount][toInt(action)];
    }

    void InputRecorder::frameTick() {
        if (_recording) {
            _framesRecorded++;
        }

        if (_playback) {
            if (_playCount < _framesRecorded) {
                _playCount++;
            } else {
                _playCount = 0;
            }
        }
    }
}
