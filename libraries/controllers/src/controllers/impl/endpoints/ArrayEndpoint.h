//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_ArrayEndpoint_h
#define hifi_Controllers_ArrayEndpoint_h

#include "../Endpoint.h"

namespace controller {

class ArrayEndpoint : public Endpoint {
    friend class UserInputMapper;
public:
    using Pointer = std::shared_ptr<ArrayEndpoint>;
    ArrayEndpoint() : Endpoint(Input::INVALID_INPUT) { }

    virtual float value() override {
        return 0.0;
    }

    virtual void apply(float newValue, float oldValue, const Endpoint::Pointer& source) override {
        for (auto& child : _children) {
            if (child->writeable()) {
                child->apply(newValue, oldValue, source);
            }
        }
    }

    virtual bool readable() const override { return false; }

private:
    Endpoint::List _children;
};

}

#endif
