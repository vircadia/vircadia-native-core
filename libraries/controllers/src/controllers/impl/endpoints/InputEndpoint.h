//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_InputEndpoint_h
#define hifi_Controllers_InputEndpoint_h

#include "../Endpoint.h"

namespace controller {

class InputEndpoint : public Endpoint {
public:
    InputEndpoint(const Input& id = Input::INVALID_INPUT)
        : Endpoint(id) {
    }

    virtual float peek() const override;
    virtual float value() override;
    // FIXME need support for writing back to vibration / force feedback effects
    virtual void apply(float newValue, const Pointer& source) override {}
    virtual Pose peekPose() const override;
    virtual Pose pose() override;
    virtual void apply(const Pose& value, const Pointer& source) override { }

    virtual bool writeable() const override { return false; }
    virtual bool readable() const override { return !_read; }
    virtual void reset() override { _read = false; }

private:
    bool _read { false };
};

}

#endif
