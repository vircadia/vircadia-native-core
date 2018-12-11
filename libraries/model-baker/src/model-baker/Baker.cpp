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

#include <shared/HifiTypes.h>

#include "BakerTypes.h"
#include "BuildGraphicsMeshTask.h"

namespace baker {

    class GetModelPartsTask {
    public:
        using Input = VaryingSet1<hfm::Model::Pointer>;
        using Output = VaryingSet3<std::vector<hfm::Mesh>, hifi::URL, MeshIndicesToModelNames>;
        using JobModel = Job::ModelIO<GetModelPartsTask, Input, Output>;

        void run(const BakeContextPointer& context, const Input& input, Output& output) {
            auto& hfmModelIn = input.get0();
            output.edit0() = hfmModelIn->meshes.toStdVector();
            output.edit1() = hfmModelIn->originalURL;
            output.edit2() = hfmModelIn->meshIndicesToModelNames;
        }
    };

    class BuildMeshesTask {
    public:
        using Input = VaryingSet4<std::vector<hfm::Mesh>, std::vector<graphics::MeshPointer>, TangentsPerMesh, BlendshapesPerMesh>;
        using Output = VaryingSet1<std::vector<hfm::Mesh>>;
        using JobModel = Job::ModelIO<BuildMeshesTask, Input, Output>;

        void run(const BakeContextPointer& context, const Input& input, Output& output) {
            auto& meshesIn = input.get0();
            int numMeshes = (int)meshesIn.size();
            auto& graphicsMeshesIn = input.get1();
            auto& tangentsPerMeshIn = input.get2();
            auto& blendshapesPerMeshIn = input.get3();

            auto meshesOut = meshesIn;
            for (int i = 0; i < numMeshes; i++) {
                auto& meshOut = meshesOut[i];
                meshOut._mesh = graphicsMeshesIn[i];
                meshOut.tangents = QVector<glm::vec3>::fromStdVector(tangentsPerMeshIn[i]);
                meshOut.blendshapes = QVector<hfm::Blendshape>::fromStdVector(blendshapesPerMeshIn[i]);
            }
            output.edit0() = meshesOut;
        }
    };

    class BuildModelTask {
    public:
        using Input = VaryingSet2<hfm::Model::Pointer, std::vector<hfm::Mesh>>;
        using Output = VaryingSet1<hfm::Model::Pointer>;
        using JobModel = Job::ModelIO<BuildModelTask, Input, Output>;

        void run(const BakeContextPointer& context, const Input& input, Output& output) {
            auto hfmModelOut = input.get0();
            hfmModelOut->meshes = QVector<hfm::Mesh>::fromStdVector(input.get1());
            output.edit0() = hfmModelOut;
        }
    };

    class BakerEngineBuilder {
    public:
        using Input = VaryingSet1<hfm::Model::Pointer>;
        using Output = VaryingSet1<hfm::Model::Pointer>;
        using JobModel = Task::ModelIO<BakerEngineBuilder, Input, Output>;
        void build(JobModel& model, const Varying& in, Varying& out) {
            const auto hfmModelIn = in.getN<Input>(0);
            // Split up the inputs from hfm::Model
            const auto getModelPartsInputs = GetModelPartsTask::Input(hfmModelIn).asVarying();
            const auto modelPartsIn = model.addJob<GetModelPartsTask>("GetModelParts", getModelPartsInputs);
            const auto meshesIn = modelPartsIn.getN<GetModelPartsTask::Output>(0);
            const auto url = modelPartsIn.getN<GetModelPartsTask::Output>(1);
            const auto meshIndicesToModelNames = modelPartsIn.getN<GetModelPartsTask::Output>(2);

            // Build the graphics::MeshPointer for each hfm::Mesh
            const auto buildGraphicsMeshInputs = BuildGraphicsMeshTask::Input(meshesIn, url, meshIndicesToModelNames).asVarying();
            const auto buildGraphicsMeshOutputs = model.addJob<BuildGraphicsMeshTask>("BuildGraphicsMesh", buildGraphicsMeshInputs);
            const auto graphicsMeshes = buildGraphicsMeshOutputs.getN<BuildGraphicsMeshTask::Output>(0);
            // TODO: Move tangent/blendshape validation/calculation to an earlier step
            const auto tangentsPerMesh = buildGraphicsMeshOutputs.getN<BuildGraphicsMeshTask::Output>(1);
            const auto blendshapesPerMesh = buildGraphicsMeshOutputs.getN<BuildGraphicsMeshTask::Output>(2);

            // Combine the outputs into a new hfm::Model
            const auto buildMeshesInputs = BuildMeshesTask::Input(meshesIn, graphicsMeshes, tangentsPerMesh, blendshapesPerMesh).asVarying();
            const auto buildMeshesOutputs = model.addJob<BuildMeshesTask>("BuildMeshes", buildMeshesInputs);
            const auto meshesOut = buildMeshesOutputs.getN<BuildMeshesTask::Output>(0);
            const auto buildModelInputs = BuildModelTask::Input(hfmModelIn, meshesOut).asVarying();
            const auto buildModelOutputs = model.addJob<BuildModelTask>("BuildModel", buildModelInputs);
            const auto hfmModelOut = buildModelOutputs.getN<BuildModelTask::Output>(0);
            out = Output(hfmModelOut);
        }
    };

    Baker::Baker(const hfm::Model::Pointer& hfmModel) :
        _engine(std::make_shared<Engine>(BakerEngineBuilder::JobModel::create("Baker"), std::make_shared<BakeContext>())) {
        _engine->feedInput<BakerEngineBuilder::Input>(0, hfmModel);
    }

    void Baker::run() {
        _engine->run();
        auto& output = _engine->getOutput().get<BakerEngineBuilder::Output>();
        hfmModel = output.get0();
    }

};
