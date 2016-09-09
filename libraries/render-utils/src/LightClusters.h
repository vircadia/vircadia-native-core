//
//  LightClusters.h
//
//  Created by Sam Gateau on 9/7/2016.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_LightClusters_h
#define hifi_render_utils_LightClusters_h

#include <gpu/Buffer.h>
#include <render/Engine.h>
#include "LightStage.h"

#include <ViewFrustum.h>

class FrustumGrid {
public:
    float _near { 0.1f };
    float _nearPrime { 1.0f };
    float _farPrime { 400.0f };
    float _far { 10000.0f };

    glm::uvec3 _dims { 16, 16, 16 };
    float spare;

    glm::mat4 _eyeToGridProj;
    glm::mat4 _eyeToGridProjInv;
    glm::mat4 _worldToEyeMat;
    glm::mat4 _eyeToWorldMat;

    float viewToLinearDepth(float depth) const {
        float nDepth = -depth;
        float ldepth = (nDepth - _nearPrime) / (_farPrime - _nearPrime);

        if (ldepth < 0.0f) {
            return (nDepth - _near) / (_nearPrime - _near) - 1.0f;
        }
        if (ldepth > 1.0f) {
            return (nDepth - _farPrime) / (_far - _farPrime) + 1.0f;
        }
        return ldepth;
    }

    float linearToGridDepth(float depth) const {
        return depth / (float) _dims.z;
    }

    int gridDepthToLayer(float gridDepth) const {
        return (int) gridDepth;
    }

    glm::vec2 ndcToGridXY(const glm::vec3& ncpos) const {
        return 0.5f * glm::vec2((ncpos.x + 1.0f) / (float)_dims.x, (ncpos.y + 1.0f) / (float)_dims.y);
    }

    glm::ivec3 viewToGridPos(const glm::vec3& pos) const {
        float z = linearToGridDepth(viewToLinearDepth(pos.z));

        auto cpos = _eyeToGridProj * glm::vec4(pos, 1.0f);

        glm::vec3 ncpos(cpos);
        ncpos /= cpos.w;


        return glm::ivec3(ndcToGridXY(ncpos), (int) linearToGridDepth(z));
    }

    void updateFrustrum(const ViewFrustum& frustum) {
        _eyeToGridProj = frustum.evalProjectionMatrixRange(_nearPrime, _farPrime);
        _eyeToGridProjInv = glm::inverse(_eyeToGridProj);

        Transform view;
        frustum.evalViewTransform(view);
        _eyeToWorldMat = view.getMatrix();
        _worldToEyeMat = view.getInverseMatrix();
    }

};

class LightClusters {
public:
    
    LightClusters();

    void updateFrustum(const ViewFrustum& frustum);

    void updateLightStage(const LightStagePointer& lightStage);

    void updateVisibleLights(const LightStage::LightIndices& visibleLights);


   // FrustumGrid _grid;

    ViewFrustum _frustum;

    LightStagePointer _lightStage;



    gpu::StructBuffer<FrustumGrid> _frustrumGridBuffer;


    gpu::BufferPointer _lightIndicesBuffer;
};

using LightClustersPointer = std::shared_ptr<LightClusters>;



class DebugLightClustersConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(int numDrawn READ getNumDrawn NOTIFY numDrawnChanged)
        Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
public:
    DebugLightClustersConfig() : render::Job::Config(true){}

    int getNumDrawn() { return numDrawn; }
    void setNumDrawn(int num) { numDrawn = num; emit numDrawnChanged(); }

    int maxDrawn { -1 };

signals:
    void numDrawnChanged();
    void dirty();

protected:
    int numDrawn { 0 };
};

class DebugLightClusters {
public:
    // using Inputs = render::VaryingSet6 < DeferredFrameTransformPointer, DeferredFramebufferPointer, LightingModelPointer, SurfaceGeometryFramebufferPointer, AmbientOcclusionFramebufferPointer, SubsurfaceScatteringResourcePointer>;
    using Config = DebugLightClustersConfig;
    using JobModel = render::Job::Model<DebugLightClusters, Config>;

    DebugLightClusters();

    void configure(const Config& config);

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);

protected:
    gpu::BufferPointer _gridBuffer;
    gpu::PipelinePointer _drawClusterGrid;
    const gpu::PipelinePointer getDrawClusterGridPipeline();
};

#endif
