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

#include "BakerTypes.h"
#include "ModelMath.h"
#include "BuildGraphicsMeshTask.h"
#include "CalculateMeshNormalsTask.h"
#include "CalculateMeshTangentsTask.h"
#include "CalculateBlendshapeNormalsTask.h"
#include "CalculateBlendshapeTangentsTask.h"
#include "PrepareJointsTask.h"

namespace baker {

    class GetModelPartsTask {
    public:
        using Input = hfm::Model::Pointer;
        using Output = VaryingSet6<std::vector<hfm::Mesh>, hifi::URL, baker::MeshIndicesToModelNames, baker::BlendshapesPerMesh, QHash<QString, hfm::Material>, std::vector<hfm::Joint>>;
        using JobModel = Job::ModelIO<GetModelPartsTask, Input, Output>;

        void run(const BakeContextPointer& context, const Input& input, Output& output) {
            const auto& hfmModelIn = input;
            output.edit0() = hfmModelIn->meshes.toStdVector();
            output.edit1() = hfmModelIn->originalURL;
            output.edit2() = hfmModelIn->meshIndicesToModelNames;
            auto& blendshapesPerMesh = output.edit3();
            blendshapesPerMesh.reserve(hfmModelIn->meshes.size());
            for (int i = 0; i < hfmModelIn->meshes.size(); i++) {
                blendshapesPerMesh.push_back(hfmModelIn->meshes[i].blendshapes.toStdVector());
            }
            output.edit4() = hfmModelIn->materials;
            output.edit5() = hfmModelIn->joints.toStdVector();
        }
    };

    class BuildBlendshapesTask {
    public:
        using Input = VaryingSet3<BlendshapesPerMesh, std::vector<NormalsPerBlendshape>, std::vector<TangentsPerBlendshape>>;
        using Output = BlendshapesPerMesh;
        using JobModel = Job::ModelIO<BuildBlendshapesTask, Input, Output>;

        void run(const BakeContextPointer& context, const Input& input, Output& output) {
            const auto& blendshapesPerMeshIn = input.get0();
            const auto& normalsPerBlendshapePerMesh = input.get1();
            const auto& tangentsPerBlendshapePerMesh = input.get2();
            auto& blendshapesPerMeshOut = output;

            blendshapesPerMeshOut = blendshapesPerMeshIn;

            for (int i = 0; i < (int)blendshapesPerMeshOut.size(); i++) {
                const auto& normalsPerBlendshape = safeGet(normalsPerBlendshapePerMesh, i);
                const auto& tangentsPerBlendshape = safeGet(tangentsPerBlendshapePerMesh, i);
                auto& blendshapesOut = blendshapesPerMeshOut[i];
                for (int j = 0; j < (int)blendshapesOut.size(); j++) {
                    const auto& normals = safeGet(normalsPerBlendshape, j);
                    const auto& tangents = safeGet(tangentsPerBlendshape, j);
                    auto& blendshape = blendshapesOut[j];
                    blendshape.normals = QVector<glm::vec3>::fromStdVector(normals);
                    blendshape.tangents = QVector<glm::vec3>::fromStdVector(tangents);
                }
            }
        }
    };

    class BuildMeshesTask {
    public:
        using Input = VaryingSet5<std::vector<hfm::Mesh>, std::vector<graphics::MeshPointer>, NormalsPerMesh, TangentsPerMesh, BlendshapesPerMesh>;
        using Output = std::vector<hfm::Mesh>;
        using JobModel = Job::ModelIO<BuildMeshesTask, Input, Output>;

        void run(const BakeContextPointer& context, const Input& input, Output& output) {
            auto& meshesIn = input.get0();
            int numMeshes = (int)meshesIn.size();
            auto& graphicsMeshesIn = input.get1();
            auto& normalsPerMeshIn = input.get2();
            auto& tangentsPerMeshIn = input.get3();
            auto& blendshapesPerMeshIn = input.get4();

            auto meshesOut = meshesIn;
            for (int i = 0; i < numMeshes; i++) {
                auto& meshOut = meshesOut[i];
                meshOut._mesh = safeGet(graphicsMeshesIn, i);
                meshOut.normals = QVector<glm::vec3>::fromStdVector(safeGet(normalsPerMeshIn, i));
                meshOut.tangents = QVector<glm::vec3>::fromStdVector(safeGet(tangentsPerMeshIn, i));
                meshOut.blendshapes = QVector<hfm::Blendshape>::fromStdVector(safeGet(blendshapesPerMeshIn, i));
            }
            output = meshesOut;
        }
    };

    class BuildModelTask {
    public:
        using Input = VaryingSet5<hfm::Model::Pointer, std::vector<hfm::Mesh>, std::vector<hfm::Joint>, QMap<int, glm::quat>, QHash<QString, int>>;
        using Output = hfm::Model::Pointer;
        using JobModel = Job::ModelIO<BuildModelTask, Input, Output>;

        void run(const BakeContextPointer& context, const Input& input, Output& output) {
            auto hfmModelOut = input.get0();
            hfmModelOut->meshes = QVector<hfm::Mesh>::fromStdVector(input.get1());
            hfmModelOut->joints = QVector<hfm::Joint>::fromStdVector(input.get2());
            hfmModelOut->jointRotationOffsets = input.get3();
            hfmModelOut->jointIndices = input.get4();
            output = hfmModelOut;
        }
    };

    class BakerEngineBuilder {
    public:
        using Input = VaryingSet2<hfm::Model::Pointer, hifi::VariantHash>;
        using Output = VaryingSet2<hfm::Model::Pointer, MaterialMapping>;
        using JobModel = Task::ModelIO<BakerEngineBuilder, Input, Output>;
        void build(JobModel& model, const Varying& input, Varying& output) {
            const auto& hfmModelIn = input.getN<Input>(0);
            const auto& mapping = input.getN<Input>(1);

            // Split up the inputs from hfm::Model
            const auto modelPartsIn = model.addJob<GetModelPartsTask>("GetModelParts", hfmModelIn);
            const auto meshesIn = modelPartsIn.getN<GetModelPartsTask::Output>(0);
            const auto url = modelPartsIn.getN<GetModelPartsTask::Output>(1);
            const auto meshIndicesToModelNames = modelPartsIn.getN<GetModelPartsTask::Output>(2);
            const auto blendshapesPerMeshIn = modelPartsIn.getN<GetModelPartsTask::Output>(3);
            const auto materials = modelPartsIn.getN<GetModelPartsTask::Output>(4);
            const auto jointsIn = modelPartsIn.getN<GetModelPartsTask::Output>(5);

            // Calculate normals and tangents for meshes and blendshapes if they do not exist
            // Note: Normals are never calculated here for OBJ models. OBJ files optionally define normals on a per-face basis, so for consistency normals are calculated beforehand in OBJSerializer.
            const auto normalsPerMesh = model.addJob<CalculateMeshNormalsTask>("CalculateMeshNormals", meshesIn);
            const auto calculateMeshTangentsInputs = CalculateMeshTangentsTask::Input(normalsPerMesh, meshesIn, materials).asVarying();
            const auto tangentsPerMesh = model.addJob<CalculateMeshTangentsTask>("CalculateMeshTangents", calculateMeshTangentsInputs);
            const auto calculateBlendshapeNormalsInputs = CalculateBlendshapeNormalsTask::Input(blendshapesPerMeshIn, meshesIn).asVarying();
            const auto normalsPerBlendshapePerMesh = model.addJob<CalculateBlendshapeNormalsTask>("CalculateBlendshapeNormals", calculateBlendshapeNormalsInputs);
            const auto calculateBlendshapeTangentsInputs = CalculateBlendshapeTangentsTask::Input(normalsPerBlendshapePerMesh, blendshapesPerMeshIn, meshesIn, materials).asVarying();
            const auto tangentsPerBlendshapePerMesh = model.addJob<CalculateBlendshapeTangentsTask>("CalculateBlendshapeTangents", calculateBlendshapeTangentsInputs);

            // Build the graphics::MeshPointer for each hfm::Mesh
            const auto buildGraphicsMeshInputs = BuildGraphicsMeshTask::Input(meshesIn, url, meshIndicesToModelNames, normalsPerMesh, tangentsPerMesh).asVarying();
            const auto graphicsMeshes = model.addJob<BuildGraphicsMeshTask>("BuildGraphicsMesh", buildGraphicsMeshInputs);

            // Prepare joint information
            const auto prepareJointsInputs = PrepareJointsTask::Input(jointsIn, mapping).asVarying();
            const auto jointInfoOut = model.addJob<PrepareJointsTask>("PrepareJoints", prepareJointsInputs);
            const auto jointsOut = jointInfoOut.getN<PrepareJointsTask::Output>(0);
            const auto jointRotationOffsets = jointInfoOut.getN<PrepareJointsTask::Output>(1);
            const auto jointIndices = jointInfoOut.getN<PrepareJointsTask::Output>(2);

            // Parse material mapping
            const auto materialMapping = model.addJob<ParseMaterialMappingTask>("ParseMaterialMapping", mapping);

            // Combine the outputs into a new hfm::Model
            const auto buildBlendshapesInputs = BuildBlendshapesTask::Input(blendshapesPerMeshIn, normalsPerBlendshapePerMesh, tangentsPerBlendshapePerMesh).asVarying();
            const auto blendshapesPerMeshOut = model.addJob<BuildBlendshapesTask>("BuildBlendshapes", buildBlendshapesInputs);
            const auto buildMeshesInputs = BuildMeshesTask::Input(meshesIn, graphicsMeshes, normalsPerMesh, tangentsPerMesh, blendshapesPerMeshOut).asVarying();
            const auto meshesOut = model.addJob<BuildMeshesTask>("BuildMeshes", buildMeshesInputs);
            const auto buildModelInputs = BuildModelTask::Input(hfmModelIn, meshesOut, jointsOut, jointRotationOffsets, jointIndices).asVarying();
            const auto hfmModelOut = model.addJob<BuildModelTask>("BuildModel", buildModelInputs);

            output = Output(hfmModelOut, materialMapping);
        }
    };

    Baker::Baker(const hfm::Model::Pointer& hfmModel, const hifi::VariantHash& mapping) :
        _engine(std::make_shared<Engine>(BakerEngineBuilder::JobModel::create("Baker"), std::make_shared<BakeContext>())) {
        _engine->feedInput<BakerEngineBuilder::Input>(0, hfmModel);
        _engine->feedInput<BakerEngineBuilder::Input>(1, mapping);
    }

    void Baker::run() {
        _engine->run();
    }

    hfm::Model::Pointer Baker::getHFMModel() const {
        return _engine->getOutput().get<BakerEngineBuilder::Output>().get0();
    }
    
    MaterialMapping Baker::getMaterialMapping() const {
        return _engine->getOutput().get<BakerEngineBuilder::Output>().get1();
    }
};
