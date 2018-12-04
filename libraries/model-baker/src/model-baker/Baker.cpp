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

        using Input = VaryingSet1<hfm::Model::Pointer>;
        using Output = VaryingSet1<hfm::Model::Pointer>;
        using JobModel = Task::ModelIO<BakerEngineBuilder, Input, Output>;
        void build(JobModel& model, const Varying& in, Varying& out) {
            out = Output(in.getN<Input>(0));
        }
    };

    Baker::Baker(const hfm::Model::Pointer& hfmModel) :
        _engine(std::make_shared<Engine>(BakerEngineBuilder::JobModel::create("Baker"), std::make_shared<ImportContext>())) {
        _engine->feedInput<BakerEngineBuilder::Input>(0, hfmModel);
    }

    void Baker::run() {
        _engine->run();
        auto& output = _engine->getOutput().get<BakerEngineBuilder::Output>();
        hfmModel = output.get0();
    }

};
