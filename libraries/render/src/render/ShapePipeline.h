//
//  ShapePipeline.h
//  render/src/render
//
//  Created by Zach Pomerantz on 12/31/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_ShapePipeline_h
#define hifi_render_ShapePipeline_h

#include <unordered_set>

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
        UNLIT,
        SKINNED,
        STEREO,
        DEPTH_ONLY,
        DEPTH_BIAS,
        WIREFRAME,
        NO_CULL_FACE,

        OWN_PIPELINE,
        INVALID,

        NUM_FLAGS, // Not a valid flag
    };
    using Flags = std::bitset<NUM_FLAGS>;

    Flags _flags;

    ShapeKey() : _flags{ 0 } {}
    ShapeKey(const Flags& flags) : _flags{flags} {}

    class Builder {
    public:
        Builder() {}
        Builder(ShapeKey key) : _flags{key._flags} {}

        ShapeKey build() const { return ShapeKey{_flags}; }

        Builder& withTranslucent() { _flags.set(TRANSLUCENT); return (*this); }
        Builder& withLightmap() { _flags.set(LIGHTMAP); return (*this); }
        Builder& withTangents() { _flags.set(TANGENTS); return (*this); }
        Builder& withSpecular() { _flags.set(SPECULAR); return (*this); }
        Builder& withUnlit() { _flags.set(UNLIT); return (*this); }
        Builder& withSkinned() { _flags.set(SKINNED); return (*this); }
        Builder& withStereo() { _flags.set(STEREO); return (*this); }
        Builder& withDepthOnly() { _flags.set(DEPTH_ONLY); return (*this); }
        Builder& withDepthBias() { _flags.set(DEPTH_BIAS); return (*this); }
        Builder& withWireframe() { _flags.set(WIREFRAME); return (*this); }
        Builder& withoutCullFace() { _flags.set(NO_CULL_FACE); return (*this); }

        Builder& withOwnPipeline() { _flags.set(OWN_PIPELINE); return (*this); }
        Builder& invalidate() { _flags.set(INVALID); return (*this); }

        static const ShapeKey ownPipeline() { return Builder().withOwnPipeline(); }
        static const ShapeKey invalid() { return Builder().invalidate(); }

    protected:
        friend class ShapeKey;
        Flags _flags{0};
    };
    ShapeKey(const Builder& builder) : ShapeKey{builder._flags} {}

    class Filter {
    public:
        Filter(Flags flags, Flags mask) : _flags{flags}, _mask{mask} {}
        Filter(const ShapeKey& key) : _flags{ key._flags } { _mask.set(); }

        // Build a standard filter (will always exclude OWN_PIPELINE, INVALID)
        class Builder {
        public:
            Builder();

            Filter build() const { return Filter(_flags, _mask); }

            Builder& withTranslucent() { _flags.set(TRANSLUCENT); _mask.set(TRANSLUCENT); return (*this); }
            Builder& withOpaque() { _flags.reset(TRANSLUCENT); _mask.set(TRANSLUCENT); return (*this); }

            Builder& withLightmap() { _flags.set(LIGHTMAP); _mask.set(LIGHTMAP); return (*this); }
            Builder& withoutLightmap() { _flags.reset(LIGHTMAP); _mask.set(LIGHTMAP); return (*this); }

            Builder& withTangents() { _flags.set(TANGENTS); _mask.set(TANGENTS); return (*this); }
            Builder& withoutTangents() { _flags.reset(TANGENTS); _mask.set(TANGENTS); return (*this); }

            Builder& withSpecular() { _flags.set(SPECULAR); _mask.set(SPECULAR); return (*this); }
            Builder& withoutSpecular() { _flags.reset(SPECULAR); _mask.set(SPECULAR); return (*this); }

            Builder& withUnlit() { _flags.set(UNLIT); _mask.set(UNLIT); return (*this); }
            Builder& withoutUnlit() { _flags.reset(UNLIT); _mask.set(UNLIT); return (*this); }

            Builder& withSkinned() { _flags.set(SKINNED); _mask.set(SKINNED); return (*this); }
            Builder& withoutSkinned() { _flags.reset(SKINNED); _mask.set(SKINNED); return (*this); }

            Builder& withStereo() { _flags.set(STEREO); _mask.set(STEREO); return (*this); }
            Builder& withoutStereo() { _flags.reset(STEREO); _mask.set(STEREO); return (*this); }

            Builder& withDepthOnly() { _flags.set(DEPTH_ONLY); _mask.set(DEPTH_ONLY); return (*this); }
            Builder& withoutDepthOnly() { _flags.reset(DEPTH_ONLY); _mask.set(DEPTH_ONLY); return (*this); }

            Builder& withDepthBias() { _flags.set(DEPTH_BIAS); _mask.set(DEPTH_BIAS); return (*this); }
            Builder& withoutDepthBias() { _flags.reset(DEPTH_BIAS); _mask.set(DEPTH_BIAS); return (*this); }

            Builder& withWireframe() { _flags.set(WIREFRAME); _mask.set(WIREFRAME); return (*this); }
            Builder& withoutWireframe() { _flags.reset(WIREFRAME); _mask.set(WIREFRAME); return (*this); }

            Builder& withCullFace() { _flags.reset(NO_CULL_FACE); _mask.set(NO_CULL_FACE); return (*this); }
            Builder& withoutCullFace() { _flags.set(NO_CULL_FACE); _mask.set(NO_CULL_FACE); return (*this); }

        protected:
            friend class Filter;
            Flags _flags{0};
            Flags _mask{0};
        };
        Filter(const Filter::Builder& builder) : Filter(builder._flags, builder._mask) {}
    protected:
        friend class ShapePlumber;
        Flags _flags{0};
        Flags _mask{0};
    };

    bool hasLightmap() const { return _flags[LIGHTMAP]; }
    bool hasTangents() const { return _flags[TANGENTS]; }
    bool hasSpecular() const { return _flags[SPECULAR]; }
    bool isUnlit() const { return _flags[UNLIT]; }
    bool isTranslucent() const { return _flags[TRANSLUCENT]; }
    bool isSkinned() const { return _flags[SKINNED]; }
    bool isStereo() const { return _flags[STEREO]; }
    bool isDepthOnly() const { return _flags[DEPTH_ONLY]; }
    bool isDepthBiased() const { return _flags[DEPTH_BIAS]; }
    bool isWireFrame() const { return _flags[WIREFRAME]; }
    bool isCullFace() const { return !_flags[NO_CULL_FACE]; }

    bool hasOwnPipeline() const { return _flags[OWN_PIPELINE]; }
    bool isValid() const { return !_flags[INVALID]; }

    // Comparator for use in stl containers
    class Hash {
    public:
        size_t operator() (const ShapeKey& key) const {
            return std::hash<ShapeKey::Flags>()(key._flags);
        }
    };

    // Comparator for use in stl containers
    class KeyEqual {
    public:
        bool operator()(const ShapeKey& lhs, const ShapeKey& rhs) const { return lhs._flags == rhs._flags; }
    };
};

inline QDebug operator<<(QDebug debug, const ShapeKey& key) {
    if (key.isValid()) {
        if (key.hasOwnPipeline()) {
            debug << "[ShapeKey: OWN_PIPELINE]";
        } else {
            debug << "[ShapeKey:"
                << "hasLightmap:" << key.hasLightmap()
                << "hasTangents:" << key.hasTangents()
                << "hasSpecular:" << key.hasSpecular()
                << "isUnlit:" << key.isUnlit()
                << "isTranslucent:" << key.isTranslucent()
                << "isSkinned:" << key.isSkinned()
                << "isStereo:" << key.isStereo()
                << "isDepthOnly:" << key.isDepthOnly()
                << "isDepthBiased:" << key.isDepthBiased()
                << "isWireFrame:" << key.isWireFrame()
                << "isCullFace:" << key.isCullFace()
                << "]";
        }
    } else {
        debug << "[ShapeKey: INVALID]";
    }
    return debug;
}

// Rendering abstraction over gpu::Pipeline and map locations
// Meta-information (pipeline and locations) to render a shape
class ShapePipeline {
public:
    class Slot {
    public:
        enum BUFFER {
            SKINNING = 0,
            MATERIAL,
            TEXMAPARRAY,
            LIGHTING_MODEL,
            LIGHT,
            LIGHT_AMBIENT_BUFFER,
        };

        enum MAP {
            ALBEDO = 0,
            NORMAL,
            METALLIC,
            EMISSIVE_LIGHTMAP,
            ROUGHNESS,
            OCCLUSION,
            SCATTERING,
            LIGHT_AMBIENT,

            NORMAL_FITTING = 10,
        };
    };

    class Locations {
    public:
        int albedoTextureUnit;
        int normalTextureUnit;
        int roughnessTextureUnit;
        int metallicTextureUnit;
        int emissiveTextureUnit;
        int occlusionTextureUnit;
        int normalFittingMapUnit;
        int lightingModelBufferUnit;
        int skinClusterBufferUnit;
        int materialBufferUnit;
        int texMapArrayBufferUnit;
        int lightBufferUnit;
        int lightAmbientBufferUnit;
        int lightAmbientMapUnit;
    };
    using LocationsPointer = std::shared_ptr<Locations>;

    using BatchSetter = std::function<void(const ShapePipeline&, gpu::Batch&)>;

    ShapePipeline(gpu::PipelinePointer pipeline, LocationsPointer locations, BatchSetter batchSetter) :
        pipeline(pipeline), locations(locations), batchSetter(batchSetter) {}

    // Normally, a pipeline is accessed thorugh pickPipeline. If it needs to be set manually,
    // after calling setPipeline this method should be called to prepare the pipeline with default buffers.
    void prepare(gpu::Batch& batch);

    gpu::PipelinePointer pipeline;
    std::shared_ptr<Locations> locations;

protected:
    friend class ShapePlumber;

    BatchSetter batchSetter;
};
using ShapePipelinePointer = std::shared_ptr<ShapePipeline>;

class ShapePlumber {
public:
    using Key = ShapeKey;
    using Filter = Key::Filter;
    using Pipeline = ShapePipeline;
    using PipelinePointer = ShapePipelinePointer;
    using PipelineMap = std::unordered_map<ShapeKey, PipelinePointer, ShapeKey::Hash, ShapeKey::KeyEqual>;
    using Slot = Pipeline::Slot;
    using Locations = Pipeline::Locations;
    using LocationsPointer = Pipeline::LocationsPointer;
    using BatchSetter = Pipeline::BatchSetter;

    void addPipeline(const Key& key, const gpu::ShaderPointer& program, const gpu::StatePointer& state,
        BatchSetter batchSetter = nullptr);
    void addPipeline(const Filter& filter, const gpu::ShaderPointer& program, const gpu::StatePointer& state,
        BatchSetter batchSetter = nullptr);

    const PipelinePointer pickPipeline(RenderArgs* args, const Key& key) const;

protected:
    void addPipelineHelper(const Filter& filter, Key key, int bit, const PipelinePointer& pipeline);
    PipelineMap _pipelineMap;

private:
    mutable std::unordered_set<Key, Key::Hash, Key::KeyEqual> _missingKeys;
};

using ShapePlumberPointer = std::shared_ptr<ShapePlumber>;

}

#endif // hifi_render_ShapePipeline_h
