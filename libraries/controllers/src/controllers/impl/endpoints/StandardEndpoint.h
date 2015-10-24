//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_StandardEndpoint_h
#define hifi_Controllers_StandardEndpoint_h

#include "../Endpoint.h"

namespace controller {

class StandardEndpoint : public VirtualEndpoint {
public:
    StandardEndpoint(const Input& input) : VirtualEndpoint(input) {}
    virtual bool writeable() const override { return !_written; }
    virtual bool readable() const override { return !_read; }
    virtual void reset() override {
        apply(0.0f, 0.0f, Endpoint::Pointer());
        apply(Pose(), Pose(), Endpoint::Pointer());
        _written = _read = false;
    }

    virtual float value() override {
        _read = true;
        return VirtualEndpoint::value();
    }

    virtual void apply(float newValue, float oldValue, const Pointer& source) override {
        // For standard endpoints, the first NON-ZERO write counts.
        if (newValue != 0.0) {
            _written = true;
        }
        VirtualEndpoint::apply(newValue, oldValue, source);
    }

    virtual Pose pose() override {
        _read = true;
        return VirtualEndpoint::pose();
    }

    virtual void apply(const Pose& newValue, const Pose& oldValue, const Pointer& source) override {
        if (newValue != Pose()) {
            _written = true;
        }
        VirtualEndpoint::apply(newValue, oldValue, source);
    }

private:
    bool _written { false };
    bool _read { false };
};

}

#endif
