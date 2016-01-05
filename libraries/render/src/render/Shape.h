//
//  Shape.h
//  render/src/render
//
//  Created by Zach Pomerantz on 12/31/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Shape_h
#define hifi_render_Shape_h

#include <gpu/Batch.h>
#include <RenderArgs.h>

namespace render {

class ShapeKey {
public:
    enum FlagBit {
        TRANSLUCENT = 0,
        LIGHTMAP,
        TANGENTS,
        SPECULAR,
        EMISSIVE,
        SKINNED,
        STEREO,
        DEPTH_ONLY,
        SHADOW,
        WIREFRAME,

        NO_PIPELINE,
        INVALID,

        NUM_FLAGS,        // Not a valid flag
    };
    using Flags = std::bitset<NUM_FLAGS>;

    Flags _flags;

    ShapeKey() : _flags{ 0 } {}
    ShapeKey(const Flags& flags) : _flags(flags) {}

    class Builder {
        friend class ShapeKey;
        Flags _flags{ 0 };
    public:
        Builder() {}
        Builder(ShapeKey key) : _flags{ key._flags } {}

        ShapeKey build() const { return ShapeKey(_flags); }

        Builder& withTranslucent() { _flags.set(TRANSLUCENT); return (*this); }
        Builder& withLightmap() { _flags.set(LIGHTMAP); return (*this); }
        Builder& withTangents() { _flags.set(TANGENTS); return (*this); }
        Builder& withSpecular() { _flags.set(SPECULAR); return (*this); }
        Builder& withEmissive() { _flags.set(EMISSIVE); return (*this); }
        Builder& withSkinned() { _flags.set(SKINNED); return (*this); }
        Builder& withStereo() { _flags.set(STEREO); return (*this); }
        Builder& withDepthOnly() { _flags.set(DEPTH_ONLY); return (*this); }
        Builder& withShadow() { _flags.set(SHADOW); return (*this); }
        Builder& withWireframe() { _flags.set(WIREFRAME); return (*this); }
        Builder& withoutPipeline() { _flags.set(NO_PIPELINE); return (*this); }
        Builder& invalidate() { _flags.set(INVALID); return (*this); }

        static const ShapeKey noPipeline() { return Builder().withoutPipeline(); }
        static const ShapeKey invalid() { return Builder().invalidate(); }
    };
    ShapeKey(const Builder& builder) : ShapeKey(builder._flags) {}

    bool hasLightmap() const { return _flags[LIGHTMAP]; }
    bool hasTangents() const { return _flags[TANGENTS]; }
    bool hasSpecular() const { return _flags[SPECULAR]; }
    bool hasEmissive() const { return _flags[EMISSIVE]; }
    bool isTranslucent() const { return _flags[TRANSLUCENT]; }
    bool isSkinned() const { return _flags[SKINNED]; }
    bool isStereo() const { return _flags[STEREO]; }
    bool isDepthOnly() const { return _flags[DEPTH_ONLY]; }
    bool isShadow() const { return _flags[SHADOW]; }
    bool isWireFrame() const { return _flags[WIREFRAME]; }

    bool hasPipeline() const { return !_flags[NO_PIPELINE]; }
    bool isValid() const { return !_flags[INVALID]; }

    // Hasher for use in unordered_maps
    class Hash {
    public:
        size_t operator() (const ShapeKey& key) {
            return std::hash<ShapeKey::Flags>()(key._flags);
        }
    };

    // Comparator for use in unordered_maps
    class KeyEqual {
    public:
        bool operator()(const ShapeKey& lhs, const ShapeKey& rhs) const { return lhs._flags == rhs._flags; }
    };
};

inline QDebug operator<<(QDebug debug, const ShapeKey& renderKey) {
    if (renderKey.isValid()) {
        debug << "[ShapeKey:"
            << "hasLightmap:" << renderKey.hasLightmap()
            << "hasTangents:" << renderKey.hasTangents()
            << "hasSpecular:" << renderKey.hasSpecular()
            << "hasEmissive:" << renderKey.hasEmissive()
            << "isTranslucent:" << renderKey.isTranslucent()
            << "isSkinned:" << renderKey.isSkinned()
            << "isStereo:" << renderKey.isStereo()
            << "isDepthOnly:" << renderKey.isDepthOnly()
            << "isShadow:" << renderKey.isShadow()
            << "isWireFrame:" << renderKey.isWireFrame()
            << "]";
    } else {
        debug << "[ShapeKey: INVALID]";
    }
    return debug;
}

// Rendering abstraction over gpu::Pipeline and map locations
class ShapePipeline {
public:
    class Slots {
    public:
        static const int SKINNING_GPU = 2;
        static const int MATERIAL_GPU = 3;
        static const int DIFFUSE_MAP = 0;
        static const int NORMAL_MAP = 1;
        static const int SPECULAR_MAP = 2;
        static const int LIGHTMAP_MAP = 3;
        static const int LIGHT_BUFFER = 4;
        static const int NORMAL_FITTING_MAP = 10;
    };

    class Locations {
    public:
        int texcoordMatrices;
        int diffuseTextureUnit;
        int normalTextureUnit;
        int specularTextureUnit;
        int emissiveTextureUnit;
        int emissiveParams;
        int normalFittingMapUnit;
        int skinClusterBufferUnit;
        int materialBufferUnit;
        int lightBufferUnit;
    };
    using LocationsPointer = std::shared_ptr<Locations>;

    gpu::PipelinePointer pipeline;
    std::shared_ptr<Locations> locations;

    ShapePipeline() : ShapePipeline(nullptr, nullptr) {}
    ShapePipeline(gpu::PipelinePointer pipeline, std::shared_ptr<Locations> locations) :
        pipeline(pipeline), locations(locations) {}
};

// Meta-information (pipeline and locations) to render a shape
class Shape {
public:
    using Key = ShapeKey;
    using Pipeline = ShapePipeline;
    using PipelinePointer = std::shared_ptr<Pipeline>;
    using Slots = ShapePipeline::Slots;
    using Locations = ShapePipeline::Locations;

    using PipelineMap = std::unordered_map<ShapeKey, PipelinePointer, ShapeKey::Hash, ShapeKey::KeyEqual>;
    class PipelineLib : public PipelineMap {
    public:
        void addPipeline(Key key, gpu::ShaderPointer& vertexShader, gpu::ShaderPointer& pixelShader);
    };

    static void addPipeline(Key key, gpu::ShaderPointer& vertexShader, gpu::ShaderPointer& pixelShader) {
        _pipelineLib.addPipeline(key, vertexShader, pixelShader);
    }
    virtual const PipelinePointer pickPipeline(RenderArgs* args, const Key& key)  const {
        return Shape::_pickPipeline(args, key);
    }

    virtual ~Shape() {};

protected:
    static const PipelinePointer _pickPipeline(RenderArgs* args, const Key& key);
    static PipelineLib _pipelineLib;
};

}

#endif // hifi_render_Shape_h
