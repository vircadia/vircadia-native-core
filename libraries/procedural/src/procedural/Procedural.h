//
//  Created by Bradley Austin Davis on 2015/09/05
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <atomic>

#include <QtCore/qglobal.h>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <render/Args.h>
#include <gpu/Shader.h>
#include <gpu/Pipeline.h>
#include <gpu/Batch.h>
#include <material-networking/ShaderCache.h>
#include <material-networking/TextureCache.h>
#include "ProceduralMaterialCache.h"

using UniformLambdas = std::list<std::function<void(gpu::Batch& batch)>>;
const size_t MAX_PROCEDURAL_TEXTURE_CHANNELS{ 4 };

/*@jsdoc
 * An object containing user-defined uniforms for communicating data to shaders.
 * @typedef {object} ProceduralUniforms
 */

/*@jsdoc
 * The data used to define a Procedural shader material.
 * @typedef {object} ProceduralData
 * @property {number} version=1 - The version of the procedural shader.
 * @property {string} vertexShaderURL - A link to a vertex shader.  Currently, only GLSL shaders are supported.  The shader must implement a different method depending on the version.
 *     If a procedural material contains a vertex shader, the bounding box of the material entity is used to cull the object to which the material is applied.
 * @property {string} fragmentShaderURL - A link to a fragment shader.  Currently, only GLSL shaders are supported.  The shader must implement a different method depending on the version.
 *     <code>shaderUrl</code> is an alias.
 * @property {string[]} channels=[] - An array of input texture URLs or entity IDs.  Currently, up to 4 are supported.  An entity ID may be that of an Image or Web entity.
 * @property {ProceduralUniforms} uniforms={} - A {@link ProceduralUniforms} object containing all the custom uniforms to be passed to the shader.
 */

struct ProceduralData {
    static QJsonValue getProceduralData(const QString& proceduralJson);
    static ProceduralData parse(const QString& userDataJson);
    void parse(const QJsonObject&);

    // Rendering object descriptions, from userData
    uint8_t version { 0 };
    QUrl fragmentShaderUrl;
    QUrl vertexShaderUrl;
    QJsonObject uniforms;
    QJsonArray channels;
};

class ProceduralProgramKey {
public:
    enum FlagBit {
        IS_TRANSPARENT = 0,
        IS_SKINNED,
        IS_SKINNED_DQ,

        NUM_FLAGS
    };
    typedef std::bitset<NUM_FLAGS> Flags;

    Flags _flags;

    bool isTransparent() const { return _flags[IS_TRANSPARENT]; }
    bool isSkinned() const { return _flags[IS_SKINNED]; }
    bool isSkinnedDQ() const { return _flags[IS_SKINNED_DQ]; }

    ProceduralProgramKey(bool transparent = false, bool isSkinned = false, bool isSkinnedDQ = false) {
        _flags.set(IS_TRANSPARENT, transparent);
        _flags.set(IS_SKINNED, isSkinned);
        _flags.set(IS_SKINNED_DQ, isSkinnedDQ);
    }
};
namespace std {
    template <>
    struct hash<ProceduralProgramKey> {
        size_t operator()(const ProceduralProgramKey& key) const {
            return std::hash<std::bitset<ProceduralProgramKey::FlagBit::NUM_FLAGS>>()(key._flags);
        }
    };
}
inline bool operator==(const ProceduralProgramKey& a, const ProceduralProgramKey& b) {
    return a._flags == b._flags;
}
inline bool operator!=(const ProceduralProgramKey& a, const ProceduralProgramKey& b) {
    return a._flags != b._flags;
}

// FIXME better encapsulation
// FIXME better mechanism for extending to things rendered using shaders other than simple.slv
struct Procedural {
public:
    Procedural();
    void setProceduralData(const ProceduralData& proceduralData);

    bool isReady() const;
    bool isEnabled() const { return _enabled; }
    void prepare(gpu::Batch& batch, const glm::vec3& position, const glm::vec3& size, const glm::quat& orientation,
                 const uint64_t& created, const ProceduralProgramKey key = ProceduralProgramKey());

    glm::vec4 getColor(const glm::vec4& entityColor) const;
    uint64_t getFadeStartTime() const { return _fadeStartTime; }
    bool isFading() const { return _doesFade && _isFading; }
    void setIsFading(bool isFading) { _isFading = isFading; }
    void setDoesFade(bool doesFade) { _doesFade = doesFade; }

    bool hasVertexShader() const;
    void setBoundOperator(const std::function<AABox(RenderArgs*)>& boundOperator) { _boundOperator = boundOperator; }
    bool hasBoundOperator() const { return (bool)_boundOperator; }
    AABox getBound(RenderArgs* args) { return _boundOperator(args); }

    gpu::Shader::Source _vertexSource;
    gpu::Shader::Source _vertexSourceSkinned;
    gpu::Shader::Source _vertexSourceSkinnedDQ;
    gpu::Shader::Source _opaqueFragmentSource;
    gpu::Shader::Source _transparentFragmentSource;

    gpu::StatePointer _opaqueState { std::make_shared<gpu::State>() };
    gpu::StatePointer _transparentState { std::make_shared<gpu::State>() };

    static std::function<void(gpu::StatePointer)> opaqueStencil;
    static std::function<void(gpu::StatePointer)> transparentStencil;

protected:
    // DO NOT TOUCH
    // We have to pack these in a particular way to match the ProceduralCommon.slh
    // layout.  
    struct StandardInputs {
        vec4 date;
        vec4 position; 
        vec4 scale;
        float timeSinceLastCompile;
        float timeSinceFirstCompile;
        float timeSinceEntityCreation;
        int frameCount;
        vec4 resolution[4];
        mat4 orientation;
    };

    static_assert(0 == offsetof(StandardInputs, date), "ProceduralOffsets");
    static_assert(16 == offsetof(StandardInputs, position), "ProceduralOffsets");
    static_assert(32 == offsetof(StandardInputs, scale), "ProceduralOffsets");
    static_assert(48 == offsetof(StandardInputs, timeSinceLastCompile), "ProceduralOffsets");
    static_assert(52 == offsetof(StandardInputs, timeSinceFirstCompile), "ProceduralOffsets");
    static_assert(56 == offsetof(StandardInputs, timeSinceEntityCreation), "ProceduralOffsets");
    static_assert(60 == offsetof(StandardInputs, frameCount), "ProceduralOffsets");
    static_assert(64 == offsetof(StandardInputs, resolution), "ProceduralOffsets");
    static_assert(128 == offsetof(StandardInputs, orientation), "ProceduralOffsets");

    // Procedural metadata
    ProceduralData _data;

    bool _enabled { false };
    uint64_t _lastCompile { 0 };
    uint64_t _firstCompile { 0 };
    int32_t _frameCount { 0 };

    // Rendering object descriptions
    QString _vertexShaderSource;
    QString _vertexShaderPath;
    uint64_t _vertexShaderModified { 0 };
    NetworkShaderPointer _networkVertexShader;
    QString _fragmentShaderSource;
    QString _fragmentShaderPath;
    uint64_t _fragmentShaderModified { 0 };
    NetworkShaderPointer _networkFragmentShader;
    bool _shaderDirty { true };
    bool _uniformsDirty { true };

    // Rendering objects
    UniformLambdas _uniforms;
    NetworkTexturePointer _channels[MAX_PROCEDURAL_TEXTURE_CHANNELS];

    std::unordered_map<ProceduralProgramKey, gpu::PipelinePointer> _proceduralPipelines;

    StandardInputs _standardInputs;
    gpu::BufferPointer _standardInputsBuffer;

    // Entity metadata
    glm::vec3 _entityDimensions;
    glm::vec3 _entityPosition;
    glm::mat3 _entityOrientation;
    uint64_t _entityCreated;

private:
    void setupUniforms();

    mutable uint64_t _fadeStartTime { 0 };
    mutable bool _hasStartedFade { false };
    mutable bool _isFading { false };
    bool _doesFade { true };
    ProceduralProgramKey _prevKey;

    std::function<AABox(RenderArgs*)> _boundOperator { nullptr };

    mutable std::mutex _mutex;
};

namespace graphics {

class ProceduralMaterial : public NetworkMaterial {
public:
    ProceduralMaterial() : NetworkMaterial() { initializeProcedural(); }
    ProceduralMaterial(const NetworkMaterial& material) : NetworkMaterial(material) { initializeProcedural(); }

    virtual bool isProcedural() const override { return true; }
    virtual bool isEnabled() const override { return _procedural.isEnabled(); }
    virtual bool isReady() const override { return _procedural.isReady(); }
    virtual QString getProceduralString() const override { return _proceduralString; }

    void setProceduralData(const QString& data) {
        _proceduralString = data;
        _procedural.setProceduralData(ProceduralData::parse(data));
    }
    virtual glm::vec4 getColor(const glm::vec4& color) const { return _procedural.getColor(color); }
    virtual bool isFading() const { return _procedural.isFading(); }
    void setIsFading(bool isFading) { _procedural.setIsFading(isFading); }
    virtual uint64_t getFadeStartTime() const { return _procedural.getFadeStartTime(); }
    virtual bool hasVertexShader() const { return _procedural.hasVertexShader(); }
    virtual void prepare(gpu::Batch& batch, const glm::vec3& position, const glm::vec3& size, const glm::quat& orientation,
                 const uint64_t& created, const ProceduralProgramKey key = ProceduralProgramKey()) {
        _procedural.prepare(batch, position, size, orientation, created, key);
    }

    virtual void initializeProcedural();

    void setBoundOperator(const std::function<AABox(RenderArgs*)>& boundOperator) { _procedural.setBoundOperator(boundOperator); }
    bool hasBoundOperator() const { return _procedural.hasBoundOperator(); }
    AABox getBound(RenderArgs* args) { return _procedural.getBound(args); }

private:
    QString _proceduralString;
    Procedural _procedural;
};
typedef std::shared_ptr<ProceduralMaterial> ProceduralMaterialPointer;

}
