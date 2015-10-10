#include "NewControllerScriptingInterface.h"

#include <mutex>
#include <set>

#include <GLMHelpers.h>
#include <DependencyManager.h>
#include <input-plugins/UserInputMapper.h>

#include "impl/MappingBuilderProxy.h"

namespace Controllers {
    void NewControllerScriptingInterface::update() {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        static float last = secTimestampNow();
        float now = secTimestampNow();
        userInputMapper->update(now - last);
        last = now;
    }

    QObject* NewControllerScriptingInterface::newMapping() {
        qDebug() << "Creating new Mapping proxy";
        return new MappingBuilderProxy(std::make_shared<Mapping>());
    }

    float NewControllerScriptingInterface::getValue(const int& source) {
        //UserInputMapper::Input input; input._id = source;
        //auto userInputMapper = DependencyManager::get<UserInputMapper>();
        //auto deviceProxy = userInputMapper->getDeviceProxy(input);
        //return deviceProxy->getButton(input, 0) ? 1.0 : 0.0;

        return (sin(secTimestampNow()) + 1.0f) / 2.0f;
    }

} // namespace controllers




//    class MappingsStack {
//        std::list<Mapping> _stack;
//        ValueMap _lastValues;
//
//        void update() {
//            EndpointList hardwareInputs = getHardwareEndpoints();
//            ValueMap currentValues;
//
//            for (auto input : hardwareInputs) {
//                currentValues[input] = input->value();
//            }
//
//            // Now process the current values for each level of the stack
//            for (auto& mapping : _stack) {
//                update(mapping, currentValues);
//            }
//
//            _lastValues = currentValues;
//        }
//
//        void update(Mapping& mapping, ValueMap& values) {
//            ValueMap updates;
//            EndpointList consumedEndpoints;
//            for (const auto& entry : values) {
//                Endpoint* endpoint = entry.first;
//                if (!mapping._channelMappings.count(endpoint)) {
//                    continue;
//                }
//
//                const Mapping::List& routes = mapping._channelMappings[endpoint];
//                consumedEndpoints.push_back(endpoint);
//                for (const auto& route : routes) {
//                    float lastValue = 0;
//                    if (mapping._lastValues.count(endpoint)) {
//                        lastValue = mapping._lastValues[endpoint];
//                    }
//                    float value = entry.second;
//                    for (const auto& filter : route._filters) {
//                        value = filter->apply(value, lastValue);
//                    }
//                    updates[route._destination] = value;
//                }
//            }
//
//            // Update the last seen values
//            mapping._lastValues = values;
//
//            // Remove all the consumed inputs
//            for (auto endpoint : consumedEndpoints) {
//                values.erase(endpoint);
//            }
//
//            // Add all the updates (may restore some of the consumed data if a passthrough was created (i.e. source == dest)
//            for (const auto& entry : updates) {
//                values[entry.first] = entry.second;
//            }
//        }
//    };
//var mapping = Controller.newMapping();
//mapping.map(hydra.LeftButton0, actions.ContextMenu);
//mapping.map(hydra.LeftButton0).to(xbox.RT);
//mapping.from(xbox.RT).constrainToBoolean().invert().to(actions.Foo)
//    mapping.from(xbox.RY).invert().deadZone(0.2).to(actions.Pitch)
//    mapping.from(xbox.RY).filter(function(newValue, oldValue) {
//    return newValue * 2.0
//}).to(actions.Pitch)

//mapping.from(function(time) {
//        return Math.cos(time);
//    }).to(actions.Pitch);

//    mapping.mapFromFunction(function() {
//        return x;
//    }, actions.ContextMenu);

//    mapping.from(xbox.LY).clamp(0, 1).to(actions.Forward);
//    mapping.from(xbox.LY).clamp(-1, 0).to(actions.Backward);
//    mapping.from(xbox.RY).clamp(0, 1).to(actions.Forward);
//    mapping.from(xbox.RS).to();
//    mapping.from(xbox.ALL).to();

//    mapping.from(xbox.RY).to(function(...) { ... });
//    mapping.from(xbox.RY).pass();

//    mapping.suppress() ≅ mapping.to(null)
//        mapping.pass() ≅ mapping.to(fromControl)

//        mapping.from(keyboard.RightParen).invert().to(actions.Yaw)
//        mapping.from(keyboard.LeftParen).to(actions.Yaw)

//        mapping.from(hydra.LX).pulse(MIN_SNAP_TIME, 3.0).to(Actions.Yaw)

//        mapping.from(keyboard.LeftParen).pulse(MIN_SNAP_TIME).to(Actions.Yaw)
//        // Enable and disable as above

//        mappingSnap.from(hydra.LX).to(function(newValue, oldValue) {
//        timeSinceLastYaw += deltaTime

#include "NewControllerScriptingInterface.moc"
