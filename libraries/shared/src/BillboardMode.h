//
//  Created by Sam Gondelman on 11/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BillboardMode_h
#define hifi_BillboardMode_h

#include "QString"

enum class BillboardMode {
    NONE = 0,
    YAW,
    FULL
};

class BillboardModeHelpers {
public:
    static QString getNameForBillboardMode(BillboardMode mode);
};

#endif // hifi_BillboardMode_h

