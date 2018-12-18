//
//  Baker.cpp
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2018/12/04.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Baker.h"

namespace baker {

    class BakerEngineBuilder {
    public:
        using Unused = int;

        using Input = hfm::Model::Pointer;
        using Output = hfm::Model::Pointer;
        using JobModel = Task::ModelIO<BakerEngineBuilder, Input, Output>;
        void build(JobModel& model, const Varying& in, Varying& out) {
            out = in;
        }
    };

    Baker::Baker(const hfm::Model::Pointer& hfmModel) :
        _engine(std::make_shared<Engine>(BakerEngineBuilder::JobModel::create("Baker"), std::make_shared<BakeContext>())) {
        _engine->feedInput<BakerEngineBuilder::Input>(hfmModel);
    }

    void Baker::run() {
        _engine->run();
        hfmModel = _engine->getOutput().get<BakerEngineBuilder::Output>();
    }

};
