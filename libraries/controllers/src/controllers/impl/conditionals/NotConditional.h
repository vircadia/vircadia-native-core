//
//  Created by Bradley Austin Davis 2015/10/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_NotConditional_h
#define hifi_Controllers_NotConditional_h

#include "../Conditional.h"

namespace controller {

    class NotConditional : public Conditional {
    public:
        using Pointer = std::shared_ptr<NotConditional>;

        NotConditional(Conditional::Pointer operand) : _operand(operand) { }

        virtual bool satisfied() override;

    private:
        Conditional::Pointer _operand;
    };
}

#endif
