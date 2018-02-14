//
//  Created by Sam Gondelman on 1/12/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MaterialMappingMode_h
#define hifi_MaterialMappingMode_h

#include "QString"

enum MaterialMappingMode {
    UV = 0,
    PROJECTED
};

class MaterialMappingModeHelpers {
public:
    static QString getNameForMaterialMappingMode(MaterialMappingMode mode);
};

#endif // hifi_MaterialMappingMode_h

