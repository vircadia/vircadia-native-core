//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Endpoint.h"

#include <mutex>
#include <input-plugins/UserInputMapper.h>

namespace Controllers {

    // FIXME how do we handle dynamic changes in connected hardware?
    const Endpoint::List& Endpoint::getHardwareEndpoints() {
        static Endpoint::List ACTIVE_HARDWARE;
        static std::once_flag once;
        std::call_once(once, [&] {
            auto userInputMapper = DependencyManager::get<UserInputMapper>();
            // TODO populate ACTIVE_HARDWARE with all the connected devices
            // For each connected device
                // for each input channel
                    // build a HardwareEndpoint instance around the input channel and add it to the list
        });
    }

    // Ex: xbox.RY, xbox.A ....
    class HardwareEndpoint : public Endpoint {
    public:
        virtual float value() override {
            // ...
        }

        virtual void apply(float newValue, float oldValue, const Endpoint& source) override {
            // Default does nothing, but in theory this could be something like vibration
            // mapping.from(xbox.X).to(xbox.Vibrate)
        }
    };

    // Ex: Standard.RY, Action.Yaw
    class VirtualEndpoint : public Endpoint {
    public:
        virtual void apply(float newValue) {
            if (newValue != _lastValue) {
                _lastValue = newValue;
            }
        }

        virtual float value() {
            return _lastValue;
        }

        float _lastValue;
    };

}
