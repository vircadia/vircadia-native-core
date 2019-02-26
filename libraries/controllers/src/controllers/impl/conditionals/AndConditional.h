//
//  Created by Bradley Austin Davis 2015/10/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_AndConditional_h
#define hifi_Controllers_AndConditional_h

#include "../Conditional.h"

namespace controller {

class AndConditional : public Conditional {
public:
    using Pointer = std::shared_ptr<AndConditional>;

    AndConditional(Conditional::List children)
        : _children(children) {}

    AndConditional(Conditional::Pointer& first, Conditional::Pointer& second)
        : _children({ first, second }) {}

    virtual bool satisfied() override;

private:
    Conditional::List _children;
};

}

#endif
