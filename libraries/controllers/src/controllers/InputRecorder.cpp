//
//  Created by Dante Ruiz 2017/04/16
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InputRecorder.h"

InputRecorder::InputRecorder() {}

InputRecorder::~InputRecorder() {}

InputRecorder& InputRecorder::getInstance() {
    static InputRecorder inputRecorder;
    return inputRecorder;
}
