//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filter_h
#define hifi_Controllers_Filter_h

#include <list>
#include <memory>
#include <numeric>
#include <functional>
#include <map>

#include <GLMHelpers.h>

#include <QtCore/QEasingCurve>

class QJsonObject;
class QJsonArray;


namespace controller {

    // Encapsulates part of a filter chain
    class Filter {
    public:
        virtual float apply(float value) const = 0;

        using Pointer = std::shared_ptr<Filter>;
        using List = std::list<Pointer>;
        using Lambda = std::function<float(float)>;

        // Factory features
        virtual bool parseParameters(const QJsonArray& parameters) { return true; }

        class Factory {
        public:

            class Entry {
            public:
                virtual Filter* create() const = 0;
                virtual const char* getName() const = 0;

                Entry() {};
                virtual ~Entry() {};
            };

            template <class T> class ClassEntry : public Entry {
            public:
                virtual Filter* create() const { return (Filter*) new T(); }
                virtual const char* getName() const {  return T::getName(); };

                ClassEntry() {};
                virtual ~ClassEntry() = default;

                class Builder {
                public:
                    Builder() {
                        std::shared_ptr<Entry> classEntry(new ClassEntry<T>());
                        Filter::getFactory().registerEntry(classEntry);
                    }
                };
            };

            using EntryMap = std::map<std::string, std::shared_ptr<Entry>>;

            void registerEntry(std::shared_ptr<Entry>& entry) {
                if (entry) {
                    _entries.insert(EntryMap::value_type(entry->getName(), entry));
                }
            }

            Filter* create(const std::string& name) const {
                const auto& entryIt = _entries.find(name);
                if (entryIt != _entries.end()) {
                    return (*entryIt).second->create();
                }
                return nullptr;
            }

        protected:
            EntryMap _entries;
        };

        static Filter::Pointer parse(const QJsonObject& json);
        static Factory& getFactory() { return _factory; }
    protected:
        static Factory _factory;
    };
}

#define REGISTER_FILTER_CLASS(classEntry, className) \
    static const char* getName() { return className; } \
    using FactoryEntryBuilder = Filter::Factory::ClassEntry<classEntry>::Builder;\
    static FactoryEntryBuilder _factoryEntryBuilder;

namespace controller {

    class LambdaFilter : public Filter {
    public:
    //    LambdaFilter() {}
        LambdaFilter(Lambda f) : _function(f) {};

        virtual float apply(float value) const {
            return _function(value);
        }

        virtual bool parseParameters(const QJsonArray& parameters) { return true; }

//        REGISTER_FILTER_CLASS(LambdaFilter);
    private:
        Lambda _function;
    };

    class ScriptFilter : public Filter {
    public:

    };
   
    class ScaleFilter : public Filter {
    public:
        REGISTER_FILTER_CLASS(ScaleFilter, "scale");
        ScaleFilter() {}
        ScaleFilter(float scale): _scale(scale) {}

        virtual float apply(float value) const override {
            return value * _scale;
        }
        virtual bool parseParameters(const QJsonArray& parameters);

    private:
        float _scale = 1.0f;
    };

    class InvertFilter : public ScaleFilter {
    public:
        REGISTER_FILTER_CLASS(InvertFilter, "invert");
        InvertFilter() : ScaleFilter(-1.0f) {}
        
        virtual bool parseParameters(const QJsonArray& parameters) { return true; }

    private:
    };

    class ClampFilter : public Filter {
    public:
        REGISTER_FILTER_CLASS(ClampFilter, "clamp");
        ClampFilter(float min = 0.0, float max = 1.0) : _min(min), _max(max) {};

        virtual float apply(float value) const override {
            return glm::clamp(value, _min, _max);
        }
        virtual bool parseParameters(const QJsonArray& parameters) override;
    protected:
        float _min = 0.0f;
        float _max = 1.0f;
    };

    class DeadZoneFilter : public Filter {
    public:
        REGISTER_FILTER_CLASS(DeadZoneFilter, "deadZone");
        DeadZoneFilter(float min = 0.0) : _min(min) {};

        virtual float apply(float value) const override;
        virtual bool parseParameters(const QJsonArray& parameters) override;
    protected:
        float _min = 0.0f;
    };

    class PulseFilter : public Filter {
    public:
        REGISTER_FILTER_CLASS(PulseFilter, "pulse");
        PulseFilter() {}
        PulseFilter(float interval) : _interval(interval) {}


        virtual float apply(float value) const override;

        virtual bool parseParameters(const QJsonArray& parameters);

    private:
        mutable float _lastEmitTime{ -::std::numeric_limits<float>::max() };
        float _interval = 1.0f;
    };

    class ConstrainToIntegerFilter : public Filter {
    public:
        REGISTER_FILTER_CLASS(ConstrainToIntegerFilter, "constrainToInteger");
        ConstrainToIntegerFilter() {};

        virtual float apply(float value) const override {
            return glm::sign(value);
        }
    protected:
    };

    class ConstrainToPositiveIntegerFilter : public Filter {
    public:
        REGISTER_FILTER_CLASS(ConstrainToPositiveIntegerFilter, "constrainToPositiveInteger");
        ConstrainToPositiveIntegerFilter() {};

        virtual float apply(float value) const override {
            return (value <= 0.0f) ? 0.0f : 1.0f;
        }
    protected:
    };

    //class EasingFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override;

    //private:
    //    QEasingCurve _curve;
    //};

    //// GLSL style filters
    //class StepFilter : public Filter {
    //public:
    //    StepFilter(float edge) : _edge(edge) {};
    //    virtual float apply(float value) const override;

    //private:
    //    const float _edge;
    //};

    //class PowFilter : public Filter {
    //public:
    //    PowFilter(float exponent) : _exponent(exponent) {};
    //    virtual float apply(float value) const override;

    //private:
    //    const float _exponent;
    //};

    //class AbsFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override;
    //};

    //class SignFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override;
    //};

    //class FloorFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override {
    //        return floor(newValue);
    //    }
    //};

    //class CeilFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override {
    //        return ceil(newValue);
    //    }
    //};

    //class FractFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override {
    //        return fract(newValue);
    //    }
    //};

    //class MinFilter : public Filter {
    //public:
    //    MinFilter(float mine) : _min(min) {};

    //    virtual float apply(float value) const override {
    //        return glm::min(_min, newValue);
    //    }

    //private:
    //    const float _min;
    //};

    //class MaxFilter : public Filter {
    //public:
    //    MaxFilter(float max) : _max(max) {};
    //    virtual float apply(float newValue, float oldValue) override;
    //private:
    //    const float _max;
    //};
}

#endif
