//
//  Created by HifiExperiments on 3/14/2021
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include "Procedural.h"

class ReferenceMaterial : public graphics::ProceduralMaterial {
public:
    using Parent = graphics::ProceduralMaterial;

    ReferenceMaterial() {}
    ReferenceMaterial(QUuid uuid);

    // Material
    graphics::MaterialKey getKey() const override;
    glm::vec3 getEmissive(bool SRGB = true) const override;
    float getOpacity() const override;
    graphics::MaterialKey::OpacityMapMode getOpacityMapMode() const override;
    float getOpacityCutoff() const override;
    graphics::MaterialKey::CullFaceMode getCullFaceMode() const override;
    bool isUnlit() const override;
    glm::vec3 getAlbedo(bool SRGB = true) const override;
    float getMetallic() const override;
    float getRoughness() const override;
    float getScattering() const override;
    bool resetOpacityMap() const override;
    graphics::Material::TextureMaps getTextureMaps() const override;
    glm::vec2 getLightmapParams() const override;
    bool getDefaultFallthrough() const override;

    // NetworkMaterial
    bool isMissingTexture() override;
    bool checkResetOpacityMap() override;

    // ProceduralMaterial
    bool isProcedural() const override;
    bool isEnabled() const override;
    bool isReady() const override;
    QString getProceduralString() const override;

    glm::vec4 getColor(const glm::vec4& color) const override;
    bool isFading() const override;
    uint64_t getFadeStartTime() const override;
    bool hasVertexShader() const override;
    void prepare(gpu::Batch& batch, const glm::vec3& position, const glm::vec3& size, const glm::quat& orientation,
                 const uint64_t& created, const ProceduralProgramKey key = ProceduralProgramKey()) override;
    void initializeProcedural() override;

    bool isReference() const override { return true; }
    std::function<graphics::MaterialPointer()> getReferenceOperator() const { return _materialForUUIDOperator; }

    static void setMaterialForUUIDOperator(std::function<graphics::MaterialPointer(QUuid)> materialForUUIDOperator);

private:
    static std::function<graphics::MaterialPointer(QUuid)> _unboundMaterialForUUIDOperator;
    std::function<graphics::MaterialPointer()> _materialForUUIDOperator;
    mutable bool _locked { false };

    graphics::MaterialPointer getMaterial() const;
    std::shared_ptr<NetworkMaterial> getNetworkMaterial() const;
    graphics::ProceduralMaterialPointer getProceduralMaterial() const;

    template <typename T, typename F>
    T resultWithLock(F&& f) const;

    template <typename F>
    void withLock(F&& f) const;
};
