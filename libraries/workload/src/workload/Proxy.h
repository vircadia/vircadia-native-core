//
//  Proxy.h
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.01.30
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_workload_Proxy_h
#define hifi_workload_Proxy_h

#include "View.h"

namespace workload {

class Owner {
public:
    Owner() = default;
    Owner(const Owner& other) = default;
    Owner& operator=(const Owner& other) = default;
    template <class T> Owner(const T& data) : _concept(std::make_shared<Model<T>>(data)) {}
    ~Owner() {}
    template <class T> const T get() const { return std::static_pointer_cast<const Model<T>>(_concept)->_data; }
protected:
    class Concept {
    public:
        virtual ~Concept() = default;
    };
    template <class T> class Model : public Concept {
    public:
        using Data = T;
        Data _data;
        Model(const Data& data) : _data(data) {}
        virtual ~Model() = default;
    };
private:
    std::shared_ptr<Concept> _concept;
};

class Proxy {
public:
    Proxy() : sphere(0.0f) {}
    Proxy(const Sphere& s) : sphere(s) {}

    Sphere sphere;
    uint8_t region{ Region::INVALID };
    uint8_t prevRegion{ Region::INVALID };
    uint16_t _padding;
    uint32_t _paddings[3];

    using Vector = std::vector<Proxy>;
};


} // namespace workload

#endif // hifi_workload_Proxy_h
