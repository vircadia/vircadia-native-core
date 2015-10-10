#include <map>

#include "Endpoint.h"
#include "Filter.h"
#include "Route.h"

class QString;

namespace Controllers {

    using ValueMap = std::map<Endpoint::Pointer, float>;

    class Mapping {
    public:
        // Map of source channels to route lists
        using Map = std::map<Endpoint::Pointer, Route::List>;

        Map _channelMappings;
        ValueMap _lastValues;

        void parse(const QString& json);
        QString serialize();
    };

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
}
