//
//  Created by Bradley Austin Davis on 2015/06/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

namespace Cursor {
    enum class Source {
        MOUSE,
        LEFT_HAND,
        RIGHT_HAND,
        UNKNOWN,
    };

    class Instance {
        Source type;
    };

    class Manager {
    public:
        static Manager& instance();

        uint8_t getCount();
        Instance
    };
}


