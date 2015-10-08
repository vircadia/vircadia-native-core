//
//  ModelRender.h
//  interface/src/renderer
//
//  Created by Sam Gateau on 10/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelRender_h
#define hifi_ModelRender_h

#include <gpu/Batch.h>

#include <render/Scene.h>

class ModelRender {
public:

    static const int SKINNING_GPU_SLOT = 2;
    static const int MATERIAL_GPU_SLOT = 3;
    static const int DIFFUSE_MAP_SLOT = 0;
    static const int NORMAL_MAP_SLOT = 1;
    static const int SPECULAR_MAP_SLOT = 2;
    static const int LIGHTMAP_MAP_SLOT = 3;
    static const int LIGHT_BUFFER_SLOT = 4;

    class Locations {
    public:
        int alphaThreshold;
        int texcoordMatrices;
        int diffuseTextureUnit;
        int normalTextureUnit;
        int specularTextureUnit;
        int emissiveTextureUnit;
        int emissiveParams;
        int glowIntensity;
        int normalFittingMapUnit;
        int skinClusterBufferUnit;
        int materialBufferUnit;
        int lightBufferUnit;
    };

    static void pickPrograms(gpu::Batch& batch, RenderArgs::RenderMode mode, bool translucent, float alphaThreshold,
        bool hasLightmap, bool hasTangents, bool hasSpecular, bool isSkinned, bool isWireframe, RenderArgs* args,
        Locations*& locations);

    class RenderKey {
    public:
        enum FlagBit {
            IS_TRANSLUCENT_FLAG = 0,
            HAS_LIGHTMAP_FLAG,
            HAS_TANGENTS_FLAG,
            HAS_SPECULAR_FLAG,
            HAS_EMISSIVE_FLAG,
            IS_SKINNED_FLAG,
            IS_STEREO_FLAG,
            IS_DEPTH_ONLY_FLAG,
            IS_SHADOW_FLAG,
            IS_WIREFRAME_FLAG,

            NUM_FLAGS,
        };

        enum Flag {
            IS_TRANSLUCENT = (1 << IS_TRANSLUCENT_FLAG),
            HAS_LIGHTMAP = (1 << HAS_LIGHTMAP_FLAG),
            HAS_TANGENTS = (1 << HAS_TANGENTS_FLAG),
            HAS_SPECULAR = (1 << HAS_SPECULAR_FLAG),
            HAS_EMISSIVE = (1 << HAS_EMISSIVE_FLAG),
            IS_SKINNED = (1 << IS_SKINNED_FLAG),
            IS_STEREO = (1 << IS_STEREO_FLAG),
            IS_DEPTH_ONLY = (1 << IS_DEPTH_ONLY_FLAG),
            IS_SHADOW = (1 << IS_SHADOW_FLAG),
            IS_WIREFRAME = (1 << IS_WIREFRAME_FLAG),
        };
        typedef unsigned short Flags;



        bool isFlag(short flagNum) const { return bool((_flags & flagNum) != 0); }

        bool isTranslucent() const { return isFlag(IS_TRANSLUCENT); }
        bool hasLightmap() const { return isFlag(HAS_LIGHTMAP); }
        bool hasTangents() const { return isFlag(HAS_TANGENTS); }
        bool hasSpecular() const { return isFlag(HAS_SPECULAR); }
        bool hasEmissive() const { return isFlag(HAS_EMISSIVE); }
        bool isSkinned() const { return isFlag(IS_SKINNED); }
        bool isStereo() const { return isFlag(IS_STEREO); }
        bool isDepthOnly() const { return isFlag(IS_DEPTH_ONLY); }
        bool isShadow() const { return isFlag(IS_SHADOW); } // = depth only but with back facing
        bool isWireFrame() const { return isFlag(IS_WIREFRAME); }

        Flags _flags = 0;
        short _spare = 0;

        int getRaw() { return *reinterpret_cast<int*>(this); }


        RenderKey(
            bool translucent, bool hasLightmap,
            bool hasTangents, bool hasSpecular, bool isSkinned, bool isWireframe) :
            RenderKey((translucent ? IS_TRANSLUCENT : 0)
            | (hasLightmap ? HAS_LIGHTMAP : 0)
            | (hasTangents ? HAS_TANGENTS : 0)
            | (hasSpecular ? HAS_SPECULAR : 0)
            | (isSkinned ? IS_SKINNED : 0)
            | (isWireframe ? IS_WIREFRAME : 0)
            ) {}

        RenderKey(RenderArgs::RenderMode mode,
            bool translucent, float alphaThreshold, bool hasLightmap,
            bool hasTangents, bool hasSpecular, bool isSkinned, bool isWireframe) :
            RenderKey(((translucent && (alphaThreshold == 0.0f) && (mode != RenderArgs::SHADOW_RENDER_MODE)) ? IS_TRANSLUCENT : 0)
            | (hasLightmap && (mode != RenderArgs::SHADOW_RENDER_MODE) ? HAS_LIGHTMAP : 0) // Lightmap, tangents and specular don't matter for depthOnly
            | (hasTangents && (mode != RenderArgs::SHADOW_RENDER_MODE) ? HAS_TANGENTS : 0)
            | (hasSpecular && (mode != RenderArgs::SHADOW_RENDER_MODE) ? HAS_SPECULAR : 0)
            | (isSkinned ? IS_SKINNED : 0)
            | (isWireframe ? IS_WIREFRAME : 0)
            | ((mode == RenderArgs::SHADOW_RENDER_MODE) ? IS_DEPTH_ONLY : 0)
            | ((mode == RenderArgs::SHADOW_RENDER_MODE) ? IS_SHADOW : 0)
            ) {}

        RenderKey(int bitmask) : _flags(bitmask) {}
    };


    class RenderPipeline {
    public:
        gpu::PipelinePointer _pipeline;
        std::shared_ptr<Locations> _locations;
        RenderPipeline(gpu::PipelinePointer pipeline, std::shared_ptr<Locations> locations) :
            _pipeline(pipeline), _locations(locations) {}
    };

    typedef std::unordered_map<int, RenderPipeline> BaseRenderPipelineMap;
    class RenderPipelineLib : public BaseRenderPipelineMap {
    public:
        typedef RenderKey Key;


        void addRenderPipeline(Key key, gpu::ShaderPointer& vertexShader, gpu::ShaderPointer& pixelShader);

        void initLocations(gpu::ShaderPointer& program, Locations& locations);
    };
    static RenderPipelineLib _renderPipelineLib;

    static const RenderPipelineLib& getRenderPipelineLib();

};

#endif // hifi_ModelRender_h