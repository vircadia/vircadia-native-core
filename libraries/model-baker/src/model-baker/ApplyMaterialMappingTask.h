//
//  Created by Sam Gondelman on 2/7/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ApplyMaterialMappingTask_h
#define hifi_ApplyMaterialMappingTask_h

#include <QHash>

#include <hfm/HFM.h>

#include "Engine.h"

class ApplyMaterialMappingTask {
public:
    using Input = baker::VaryingSet2<QHash<QString, hfm::Material>, QVariantHash>;
    using Output = QHash<QString, hfm::Material>;
    using JobModel = baker::Job::ModelIO<ApplyMaterialMappingTask, Input, Output>;

    void run(const baker::BakeContextPointer& context, const Input& input, Output& output);
};

#endif // hifi_ApplyMaterialMappingTask_h