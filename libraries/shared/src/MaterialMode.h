//
//  Created by Sam Gondelman on 1/12/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MaterialMode_h
#define hifi_MaterialMode_h

#include "QString"

enum MaterialMode {
    UV = 0,
    PROJECTED
};

class MaterialModeHelpers {
public:
    static QString getNameForMaterialMode(MaterialMode mode);
};

#endif // hifi_MaterialMode_h

