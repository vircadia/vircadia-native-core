//
//  ScriptableModel.cpp
//  libraries/graphics-scripting
//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GraphicsScriptingUtil.h"
#include "ScriptableModel.h"
#include "ScriptableMesh.h"

#include <QtScript/QScriptEngine>

#include "graphics/Material.h"

#include "image/Image.h"

// #define SCRIPTABLE_MESH_DEBUG 1

scriptable::ScriptableMaterial& scriptable::ScriptableMaterial::operator=(const scriptable::ScriptableMaterial& material) {
    name = material.name;
    model = material.model;
    opacity = material.opacity;
    roughness = material.roughness;
    metallic = material.metallic;
    scattering = material.scattering;
    unlit = material.unlit;
    emissive = material.emissive;
    albedo = material.albedo;
    emissiveMap = material.emissiveMap;
    albedoMap = material.albedoMap;
    opacityMap = material.opacityMap;
    metallicMap = material.metallicMap;
    specularMap = material.specularMap;
    roughnessMap = material.roughnessMap;
    glossMap = material.glossMap;
    normalMap = material.normalMap;
    bumpMap = material.bumpMap;
    occlusionMap = material.occlusionMap;
    lightmapMap = material.lightmapMap;
    scatteringMap = material.scatteringMap;
    priority = material.priority;

    return *this;
}

scriptable::ScriptableMaterial::ScriptableMaterial(const graphics::MaterialLayer& materialLayer) :
    name(materialLayer.material->getName().c_str()),
    model(materialLayer.material->getModel().c_str()),
    opacity(materialLayer.material->getOpacity()),
    roughness(materialLayer.material->getRoughness()),
    metallic(materialLayer.material->getMetallic()),
    scattering(materialLayer.material->getScattering()),
    unlit(materialLayer.material->isUnlit()),
    emissive(materialLayer.material->getEmissive()),
    albedo(materialLayer.material->getAlbedo()),
    priority(materialLayer.priority)
{
    auto map = materialLayer.material->getTextureMap(graphics::Material::MapChannel::EMISSIVE_MAP);
    if (map && map->getTextureSource()) {
        emissiveMap = map->getTextureSource()->getUrl().toString();
    }

    map = materialLayer.material->getTextureMap(graphics::Material::MapChannel::ALBEDO_MAP);
    if (map && map->getTextureSource()) {
        albedoMap = map->getTextureSource()->getUrl().toString();
        if (map->useAlphaChannel()) {
            opacityMap = albedoMap;
        }
    }

    map = materialLayer.material->getTextureMap(graphics::Material::MapChannel::METALLIC_MAP);
    if (map && map->getTextureSource()) {
        if (map->getTextureSource()->getType() == image::TextureUsage::Type::METALLIC_TEXTURE) {
            metallicMap = map->getTextureSource()->getUrl().toString();
        } else if (map->getTextureSource()->getType() == image::TextureUsage::Type::SPECULAR_TEXTURE) {
            specularMap = map->getTextureSource()->getUrl().toString();
        }
    }

    map = materialLayer.material->getTextureMap(graphics::Material::MapChannel::ROUGHNESS_MAP);
    if (map && map->getTextureSource()) {
        if (map->getTextureSource()->getType() == image::TextureUsage::Type::ROUGHNESS_TEXTURE) {
            roughnessMap = map->getTextureSource()->getUrl().toString();
        } else if (map->getTextureSource()->getType() == image::TextureUsage::Type::GLOSS_TEXTURE) {
            glossMap = map->getTextureSource()->getUrl().toString();
        }
    }

    map = materialLayer.material->getTextureMap(graphics::Material::MapChannel::NORMAL_MAP);
    if (map && map->getTextureSource()) {
        if (map->getTextureSource()->getType() == image::TextureUsage::Type::NORMAL_TEXTURE) {
            normalMap = map->getTextureSource()->getUrl().toString();
        } else if (map->getTextureSource()->getType() == image::TextureUsage::Type::BUMP_TEXTURE) {
            bumpMap = map->getTextureSource()->getUrl().toString();
        }
    }

    map = materialLayer.material->getTextureMap(graphics::Material::MapChannel::OCCLUSION_MAP);
    if (map && map->getTextureSource()) {
        occlusionMap = map->getTextureSource()->getUrl().toString();
    }

    map = materialLayer.material->getTextureMap(graphics::Material::MapChannel::LIGHTMAP_MAP);
    if (map && map->getTextureSource()) {
        lightmapMap = map->getTextureSource()->getUrl().toString();
    }

    map = materialLayer.material->getTextureMap(graphics::Material::MapChannel::SCATTERING_MAP);
    if (map && map->getTextureSource()) {
        scatteringMap = map->getTextureSource()->getUrl().toString();
    }
}

scriptable::ScriptableModelBase& scriptable::ScriptableModelBase::operator=(const scriptable::ScriptableModelBase& other) {
    provider = other.provider;
    objectID = other.objectID;
    for (const auto& mesh : other.meshes) {
        append(mesh);
    }
    materials = other.materials;
    materialNames = other.materialNames;
    return *this;
}

scriptable::ScriptableModelBase::~ScriptableModelBase() {
#ifdef SCRIPTABLE_MESH_DEBUG
    qCDebug(graphics_scripting) << "~ScriptableModelBase" << this;
#endif
    // makes cleanup order more deterministic to help with debugging
    for (auto& m : meshes) {
        m.strongMesh.reset();
    }
    meshes.clear();
    materials.clear();
    materialNames.clear();
}

void scriptable::ScriptableModelBase::append(scriptable::WeakMeshPointer mesh) {
    meshes << ScriptableMeshBase{ provider, this, mesh, this /*parent*/ };
}

void scriptable::ScriptableModelBase::append(const ScriptableMeshBase& mesh) {
    if (mesh.provider.lock().get() != provider.lock().get()) {
        qCDebug(graphics_scripting) << "warning: appending mesh from different provider..." << mesh.provider.lock().get() << " != " << provider.lock().get();
    }
    meshes << mesh;
}

void scriptable::ScriptableModelBase::appendMaterial(const graphics::MaterialLayer& material, int shapeID, std::string materialName) {
    materials[QString::number(shapeID)].push_back(ScriptableMaterial(material));
    materials["mat::" + QString::fromStdString(materialName)].push_back(ScriptableMaterial(material));
}

void scriptable::ScriptableModelBase::appendMaterials(const std::unordered_map<std::string, graphics::MultiMaterial>& materialsToAppend) {
    auto materialsToAppendCopy = materialsToAppend;
    for (auto& multiMaterial : materialsToAppendCopy) {
        while (!multiMaterial.second.empty()) {
            materials[QString(multiMaterial.first.c_str())].push_back(ScriptableMaterial(multiMaterial.second.top()));
            multiMaterial.second.pop();
        }
    }
}

void scriptable::ScriptableModelBase::appendMaterialNames(const std::vector<std::string>& names) {
    for (auto& name : names) {
        materialNames.append(QString::fromStdString(name));
    }
}

QString scriptable::ScriptableModel::toString() const {
    return QString("[ScriptableModel%1%2 numMeshes=%3]")
        .arg(objectID.isNull() ? "" : " objectID="+objectID.toString())
        .arg(objectName().isEmpty() ? "" : " name=" +objectName())
        .arg(meshes.size());
}

scriptable::ScriptableModelPointer scriptable::ScriptableModel::cloneModel(const QVariantMap& options) {
    scriptable::ScriptableModelPointer clone = scriptable::ScriptableModelPointer(new scriptable::ScriptableModel(*this));
    clone->meshes.clear();
    for (const auto &mesh : getConstMeshes()) {
        auto cloned = mesh->cloneMesh(options.value("recalculateNormals").toBool());
        if (auto tmp = qobject_cast<scriptable::ScriptableMeshBase*>(cloned)) {
            clone->meshes << *tmp;
            tmp->deleteLater(); // schedule our copy for cleanup
        } else {
            qCDebug(graphics_scripting) << "error cloning mesh" << cloned;
        }
    }
    return clone;
}


const QVector<scriptable::ScriptableMeshPointer> scriptable::ScriptableModel::getConstMeshes() const {
    QVector<scriptable::ScriptableMeshPointer> out;
    for (const auto& mesh : meshes) {
        const scriptable::ScriptableMesh* m = qobject_cast<const scriptable::ScriptableMesh*>(&mesh);
        if (!m) {
            m = scriptable::make_scriptowned<scriptable::ScriptableMesh>(mesh);
        } else {
            qCDebug(graphics_scripting) << "reusing scriptable mesh" << m;
        }
        const scriptable::ScriptableMeshPointer mp = scriptable::ScriptableMeshPointer(const_cast<scriptable::ScriptableMesh*>(m));
        out << mp;
    }
    return out;
}

QVector<scriptable::ScriptableMeshPointer> scriptable::ScriptableModel::getMeshes() {
    QVector<scriptable::ScriptableMeshPointer> out;
    for (auto& mesh : meshes) {
        scriptable::ScriptableMesh* m = qobject_cast<scriptable::ScriptableMesh*>(&mesh);
        if (!m) {
            m = scriptable::make_scriptowned<scriptable::ScriptableMesh>(mesh);
        } else {
            qCDebug(graphics_scripting) << "reusing scriptable mesh" << m;
        }
        scriptable::ScriptableMeshPointer mp = scriptable::ScriptableMeshPointer(m);
        out << mp;
    }
    return out;
}

quint32 scriptable::ScriptableModel::mapAttributeValues(QScriptValue callback) {
    quint32 result = 0;
    QVector<scriptable::ScriptableMeshPointer> in = getMeshes();
    if (in.size()) {
        foreach (scriptable::ScriptableMeshPointer meshProxy, in) {
            result += meshProxy->mapAttributeValues(callback);
        }
    }
    return result;
}

#include "ScriptableModel.moc"
