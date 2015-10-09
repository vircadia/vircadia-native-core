#include <map>
#include <list>

#include <QtCore/QObject>
#include <QtScript/QScriptValue>

extern float currentTime();

namespace Controllers {

    /*
    * Encapsulates a particular input / output,
    * i.e. Hydra.Button0, Standard.X, Action.Yaw
    */
    class Endpoint {
    public:
        virtual float value() = 0;
        virtual void apply(float newValue, float oldValue, const Endpoint& source) = 0;
    };

    using EndpointList = std::list<Endpoint*>;

    const EndpointList& getHardwareEndpoints();

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

    /*
    * A function which provides input
    */
    class FunctionEndpoint : public Endpoint {
    public:

        virtual float value() override {
            float now = currentTime();
            float delta = now - _lastCalled;
            float result = _inputFunction.call(_object, QScriptValue(delta)).toNumber();
            _lastCalled = now;
            return result;
        }

        virtual void apply(float newValue, float oldValue, const Endpoint& source) override {
            if (newValue != oldValue) {
                //_outputFunction.call(newValue, oldValue, source);
            }
        }

        float _lastValue{ NAN };
        float _lastCalled{ 0 };
        QScriptValue _outputFunction;
        QScriptValue _inputFunction;
        QScriptValue _object;
    };


    // Encapsulates part of a filter chain
    class Filter {
    public:
        virtual float apply(float newValue, float oldValue) = 0;
    };

    class ScaleFilter : public Filter {
    public:
        virtual float apply(float newValue, float oldValue) {
            return newValue * _scale;
        }

        float _scale{ 1.0 };
    };

    class PulseFilter : public Filter {
    public:
        virtual float apply(float newValue, float oldValue) {
            // ???
        }

        float _lastEmitValue{ 0 };
        float _lastEmitTime{ 0 };
        float _interval{ -1.0f };
    };

    using FilterList = std::list<Filter*>;

    /*
    * encapsulates a source, destination and filters to apply
    */
    class Route {
    public:
        Endpoint* _source;
        Endpoint* _destination;
        FilterList _filters;
    };

    using ValueMap = std::map<Endpoint*, float>;

    class Mapping {
    public:
        // List of routes
        using List = std::list<Route>;
        // Map of source channels to route lists
        using Map = std::map<Endpoint*, List>;

        Map _channelMappings;
        ValueMap _lastValues;
    };

    class MappingsStack {
        std::list<Mapping> _stack;
        ValueMap _lastValues;

        void update() {
            EndpointList hardwareInputs = getHardwareEndpoints();
            ValueMap currentValues;

            for (auto input : hardwareInputs) {
                currentValues[input] = input->value();
            }

            // Now process the current values for each level of the stack
            for (auto& mapping : _stack) {
                update(mapping, currentValues);
            }

            _lastValues = currentValues;
        }

        void update(Mapping& mapping, ValueMap& values) {
            ValueMap updates;
            EndpointList consumedEndpoints;
            for (const auto& entry : values) {
                Endpoint* endpoint = entry.first;
                if (!mapping._channelMappings.count(endpoint)) {
                    continue;
                }

                const Mapping::List& routes = mapping._channelMappings[endpoint];
                consumedEndpoints.push_back(endpoint);
                for (const auto& route : routes) {
                    float lastValue = 0;
                    if (mapping._lastValues.count(endpoint)) {
                        lastValue = mapping._lastValues[endpoint];
                    }
                    float value = entry.second;
                    for (const auto& filter : route._filters) {
                        value = filter->apply(value, lastValue);
                    }
                    updates[route._destination] = value;
                }
            }

            // Update the last seen values
            mapping._lastValues = values;

            // Remove all the consumed inputs
            for (auto endpoint : consumedEndpoints) {
                values.erase(endpoint); 
            }

            // Add all the updates (may restore some of the consumed data if a passthrough was created (i.e. source == dest)
            for (const auto& entry : updates) {
                values[entry.first] = entry.second;
            }
        }
    };
}
