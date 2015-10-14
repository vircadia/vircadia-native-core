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

#include <QtCore/QEasingCurve>

class QJsonObject;
class QJsonArray;


namespace controller {
    /*
    template <class T> class Factory {
    public:

    template <class T> class Entry {
    public:
    virtual T* create() = 0;
    };

    template <class T, class S> class DefaultEntry{
    public:
    T* create() { return new S(); }
    };

    using EntryMap = std::map<std::string, std::unique_ptr<Entry<T>>>;

    void registerEntry(const std::string& name, std::unique_ptr<Entry<T>>& entry) {
    if (entry) {
    _entries[name] = entry;
    }
    }

    T* create(const std::string& name) const {
    auto& entryIt = _entries.find(name);
    if (entryIt != _entries.end()) {
    return (*entryIt).second->create();
    }
    return nullptr;
    }

    protected:
    EntryMap _entries;
    };
    */
    // Encapsulates part of a filter chain
    class Filter {
    public:
        virtual float apply(float value) const = 0;

        using Pointer = std::shared_ptr<Filter>;
        using List = std::list<Pointer>;
        using Lambda = std::function<float(float)>;

        // Factory features
        virtual bool parseParameters(const QJsonArray& parameters) = 0;

        class Factory {
        public:

            class Entry {
            public:
                virtual Filter* create() = 0;
                
                Entry() = default;
                virtual ~Entry() = default;
            };

            template <class T> class ClassEntry {
            public:
                virtual Filter* create() { return (Filter*) new T(); }

                ClassEntry() = default;
                virtual ~ClassEntry() = default;
            };

            using EntryMap = std::map<std::string, std::shared_ptr<Entry>>;

            void registerEntry(const std::string& name, const std::shared_ptr<Entry>& entry) {
                if (entry) {
                    _entries.insert(EntryMap::value_type(name, entry));
                }
            }

            Filter* create(const std::string& name) const {
                auto& entryIt = _entries.find(name);
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

#define REGISTER_FILTER_CLASS(classEntry) static Filter::Factory::ClassEntry<classEntry> _factoryEntry;

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
        ScaleFilter() {}
        ScaleFilter(float scale): _scale(scale) {}

        virtual float apply(float value) const override {
            return value * _scale;
        }
        virtual bool parseParameters(const QJsonArray& parameters);

        REGISTER_FILTER_CLASS(ScaleFilter);
    private:
        float _scale = 1.0f;
    };

    //class AbstractRangeFilter : public Filter {
    //public:
    //    RangeFilter(float min, float max) : _min(min), _max(max) {}

    //protected:
    //    const float _min;
    //    const float _max;
    //};

    ///*
    //* Constrains will emit the input value on the first call, and every *interval* seconds, otherwise returns 0
    //*/
    //class PulseFilter : public Filter {
    //public:
    //    PulseFilter(float interval);
    //    virtual float apply(float value) const override;

    //private:
    //    float _lastEmitTime{ -std::numeric_limits<float>::max() };
    //    const float _interval;
    //};


    class PulseFilter : public Filter {
    public:
        REGISTER_FILTER_CLASS(PulseFilter);
        PulseFilter() {}
        PulseFilter(float interval) : _interval(interval) {}


        virtual float apply(float value) const override;

        virtual bool parseParameters(const QJsonArray& parameters);

    private:
        mutable float _lastEmitTime{ -std::numeric_limits<float>::max() };
        float _interval = 1.0f;
    };

    ////class DeadzoneFilter : public AbstractRangeFilter {
    ////public:
    ////    DeadzoneFilter(float min, float max = 1.0f);
    ////    virtual float apply(float newValue, float oldValue) override;
    ////};

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

    //class ClampFilter : public RangeFilter {
    //public:
    //    ClampFilter(float min = 0.0, float max = 1.0) : RangeFilter(min, max) {};
    //    virtual float apply(float value) const override;
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
