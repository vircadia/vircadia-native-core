//
//  Created by Luis Cuenca on 3/7/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParseFlowDataTask_h
#define hifi_ParseFlowDataTask_h

#include <hfm/HFM.h>
#include "Engine.h"

class ParseFlowDataTask {
public:
    using Input = QVariantHash;
    using Output = FlowData;
    using JobModel = baker::Job::ModelIO<ParseFlowDataTask, Input, Output>;

    void run(const baker::BakeContextPointer& context, const Input& input, Output& output);
};

#endif // hifi_ParseFlowDataTask_h
