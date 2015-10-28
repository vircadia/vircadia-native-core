//
//  Created by Bradley Austin Davis on 2015/10/18
//  (based on UserInputMapper inner class created by Sam Gateau on 4/27/15)
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_controllers_DeviceProxy_h
#define hifi_controllers_DeviceProxy_h

#include <functional>

#include <QtCore/QString>
#include <QtCore/QVector>

#include "Input.h"
#include "Pose.h"

namespace controller {
    /*
    using Modifiers = std::vector<Input>;
    typedef QPair<Input, QString> InputPair;
    class Endpoint;
    using EndpointPtr = std::shared_ptr<Endpoint>;
    
    template<typename T>
    using InputGetter = std::function<T(const Input& input, int timestamp)>;
    using ButtonGetter = InputGetter<bool>;
    using AxisGetter = InputGetter<float>;
    using PoseGetter = InputGetter<Pose>;
    using ResetBindings = std::function<bool()>;
    using AvailableInputGetter = std::function<Input::NamedVector()>;
    using EndpointCreator = std::function<EndpointPtr(const Input&)>;

    class DeviceProxy {
    public:
        using Pointer = std::shared_ptr<DeviceProxy>;

        QString _name;
    };
    */
}

#endif
