//
//  Created by Sam Gondelman on 2/7/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParseMaterialMappingTask_h
#define hifi_ParseMaterialMappingTask_h

#include <QHash>

#include <hfm/HFM.h>

#include <shared/HifiTypes.h>

#include "Engine.h"
#include "BakerTypes.h"

#include <procedural/ProceduralMaterialCache.h>

class ParseMaterialMappingTask {
public:
    using Input = baker::VaryingSet2<hifi::VariantHash, hifi::URL>;
    using Output = MaterialMapping;
    using JobModel = baker::Job::ModelIO<ParseMaterialMappingTask, Input, Output>;

    void run(const baker::BakeContextPointer& context, const Input& input, Output& output);
};

#endif // hifi_ParseMaterialMappingTask_h