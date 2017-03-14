//
//  FBXReader_Material.cpp
//  interface/src/fbx
//
//  Created by Sam Gateau on 8/27/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FBXReader.h"

#include <iostream>
#include <memory>

#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QStringList>
#include <QTextStream>
#include <QtDebug>
#include <QtEndian>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include "ModelFormatLogging.h"

void FBXMaterial::getTextureNames(QSet<QString>& textureList) const {
    if (!normalTexture.isNull()) {
        textureList.insert(normalTexture.name);
    }
    if (!albedoTexture.isNull()) {
        textureList.insert(albedoTexture.name);
    }
    if (!opacityTexture.isNull()) {
        textureList.insert(opacityTexture.name);
    }
    if (!glossTexture.isNull()) {
        textureList.insert(glossTexture.name);
    }
    if (!roughnessTexture.isNull()) {
        textureList.insert(roughnessTexture.name);
    }
    if (!specularTexture.isNull()) {
        textureList.insert(specularTexture.name);
    }
    if (!metallicTexture.isNull()) {
        textureList.insert(metallicTexture.name);
    }
    if (!emissiveTexture.isNull()) {
        textureList.insert(emissiveTexture.name);
    }
    if (!occlusionTexture.isNull()) {
        textureList.insert(occlusionTexture.name);
    }
    if (!scatteringTexture.isNull()) {
        textureList.insert(scatteringTexture.name);
    }
    if (!lightmapTexture.isNull()) {
        textureList.insert(lightmapTexture.name);
    }
}

void FBXMaterial::setMaxNumPixelsPerTexture(int maxNumPixels) {
    normalTexture.maxNumPixels = maxNumPixels;
    albedoTexture.maxNumPixels = maxNumPixels;
    opacityTexture.maxNumPixels = maxNumPixels;
    glossTexture.maxNumPixels = maxNumPixels;
    roughnessTexture.maxNumPixels = maxNumPixels;
    specularTexture.maxNumPixels = maxNumPixels;
    metallicTexture.maxNumPixels = maxNumPixels;
    emissiveTexture.maxNumPixels = maxNumPixels;
    occlusionTexture.maxNumPixels = maxNumPixels;
    scatteringTexture.maxNumPixels = maxNumPixels;
    lightmapTexture.maxNumPixels = maxNumPixels;
}

bool FBXMaterial::needTangentSpace() const {
    return !normalTexture.isNull();
}

FBXTexture FBXReader::getTexture(const QString& textureID) {
    FBXTexture texture;
    const QByteArray& filepath = _textureFilepaths.value(textureID);
    texture.content = _textureContent.value(filepath);

    if (texture.content.isEmpty()) { // the content is not inlined
        texture.filename = _textureFilenames.value(textureID);
    } else { // use supplied filepath for inlined content
        texture.filename = filepath;
    }

    texture.name = _textureNames.value(textureID);
    texture.transform.setIdentity();
    texture.texcoordSet = 0;
    if (_textureParams.contains(textureID)) {
        auto p = _textureParams.value(textureID);

        texture.transform.setTranslation(p.translation);
        texture.transform.setRotation(glm::quat(glm::radians(p.rotation)));

        auto scaling = p.scaling;
        // Protect from bad scaling which should never happen
        if (scaling.x == 0.0f) {
            scaling.x = 1.0f;
        }
        if (scaling.y == 0.0f) {
            scaling.y = 1.0f;
        }
        if (scaling.z == 0.0f) {
            scaling.z = 1.0f;
        }
        texture.transform.setScale(scaling);

        if ((p.UVSet != "map1") && (p.UVSet != "UVSet0")) {
            texture.texcoordSet = 1;
        }
        texture.texcoordSetName = p.UVSet;
    }
    return texture;
}

void FBXReader::consolidateFBXMaterials(const QVariantHash& mapping) {

    QString materialMapString = mapping.value("materialMap").toString();
    QJsonDocument materialMapDocument = QJsonDocument::fromJson(materialMapString.toUtf8());
    QJsonObject materialMap = materialMapDocument.object();

    for (QHash<QString, FBXMaterial>::iterator it = _fbxMaterials.begin(); it != _fbxMaterials.end(); it++) {
        FBXMaterial& material = (*it);

        // Maya is the exporting the shading model and we are trying to use it
        bool isMaterialLambert = (material.shadingModel.toLower() == "lambert");

        // the pure material associated with this part
        bool detectDifferentUVs = false;
        FBXTexture diffuseTexture;
        FBXTexture diffuseFactorTexture;
        QString diffuseTextureID = diffuseTextures.value(material.materialID);
        QString diffuseFactorTextureID = diffuseFactorTextures.value(material.materialID);

        // If both factor and color textures are specified, the texture bound to DiffuseColor wins
        if (!diffuseFactorTextureID.isNull() || !diffuseTextureID.isNull()) {
            if (!diffuseFactorTextureID.isNull() && diffuseTextureID.isNull()) {
                diffuseTextureID = diffuseFactorTextureID;
                // If the diffuseTextureID comes from the Texture bound to DiffuseFactor, we know it s exported from maya
                // And the DiffuseFactor is forced to 0.5 by Maya which is bad
                // So we need to force it to 1.0
                material.diffuseFactor = 1.0;
            }

            diffuseTexture = getTexture(diffuseTextureID);

            // FBX files generated by 3DSMax have an intermediate texture parent, apparently
            foreach(const QString& childTextureID, _connectionChildMap.values(diffuseTextureID)) {
                if (_textureFilenames.contains(childTextureID)) {
                    diffuseTexture = getTexture(diffuseTextureID);
                }
            }

            material.albedoTexture = diffuseTexture;
            detectDifferentUVs = (diffuseTexture.texcoordSet != 0) || (!diffuseTexture.transform.isIdentity());
        }

        FBXTexture transparentTexture;
        QString transparentTextureID = transparentTextures.value(material.materialID);
        // If PBS Material, systematically bind the albedo texture as transparency texture and check for the alpha channel
        if (material.isPBSMaterial) {
            transparentTextureID = diffuseTextureID;
        }
        if (!transparentTextureID.isNull()) {
            transparentTexture = getTexture(transparentTextureID);
            material.opacityTexture = transparentTexture;
            detectDifferentUVs |= (transparentTexture.texcoordSet != 0) || (!transparentTexture.transform.isIdentity());
        }

        FBXTexture normalTexture;
        QString bumpTextureID = bumpTextures.value(material.materialID);
        QString normalTextureID = normalTextures.value(material.materialID);
        if (!normalTextureID.isNull()) {
            normalTexture = getTexture(normalTextureID);
            normalTexture.isBumpmap = false;

            material.normalTexture = normalTexture;
            detectDifferentUVs |= (normalTexture.texcoordSet != 0) || (!normalTexture.transform.isIdentity());
        } else if (!bumpTextureID.isNull()) {
            normalTexture = getTexture(bumpTextureID);
            normalTexture.isBumpmap = true;

            material.normalTexture = normalTexture;
            detectDifferentUVs |= (normalTexture.texcoordSet != 0) || (!normalTexture.transform.isIdentity());
        }

        FBXTexture specularTexture;
        QString specularTextureID = specularTextures.value(material.materialID);
        if (!specularTextureID.isNull()) {
            specularTexture = getTexture(specularTextureID);
            detectDifferentUVs |= (specularTexture.texcoordSet != 0) || (!specularTexture.transform.isIdentity());
            material.specularTexture = specularTexture;
        }

        FBXTexture metallicTexture;
        QString metallicTextureID = metallicTextures.value(material.materialID);
        if (!metallicTextureID.isNull()) {
            metallicTexture = getTexture(metallicTextureID);
            detectDifferentUVs |= (metallicTexture.texcoordSet != 0) || (!metallicTexture.transform.isIdentity());
            material.metallicTexture = metallicTexture;
        }

        FBXTexture roughnessTexture;
        QString roughnessTextureID = roughnessTextures.value(material.materialID);
        if (!roughnessTextureID.isNull()) {
            roughnessTexture = getTexture(roughnessTextureID);
            material.roughnessTexture = roughnessTexture;
            detectDifferentUVs |= (roughnessTexture.texcoordSet != 0) || (!roughnessTexture.transform.isIdentity());
        }

        FBXTexture shininessTexture;
        QString shininessTextureID = shininessTextures.value(material.materialID);
        if (!shininessTextureID.isNull()) {
            shininessTexture = getTexture(shininessTextureID);
            material.glossTexture = shininessTexture;
            detectDifferentUVs |= (shininessTexture.texcoordSet != 0) || (!shininessTexture.transform.isIdentity());
        }

        FBXTexture emissiveTexture;
        QString emissiveTextureID = emissiveTextures.value(material.materialID);
        if (!emissiveTextureID.isNull()) {
            emissiveTexture = getTexture(emissiveTextureID);
            detectDifferentUVs |= (emissiveTexture.texcoordSet != 0) || (!emissiveTexture.transform.isIdentity());
            material.emissiveTexture = emissiveTexture;

            if (isMaterialLambert) {
                // If the emissiveTextureID comes from the Texture bound to Emissive when material is lambert, we know it s exported from maya
                // And the EMissiveColor is forced to 0.5 by Maya which is bad
                // So we need to force it to 1.0
                material.emissiveColor = vec3(1.0);
            }
        }

        FBXTexture occlusionTexture;
        QString occlusionTextureID = occlusionTextures.value(material.materialID);
        if (occlusionTextureID.isNull()) {
            // 2nd chance
            // For blender we use the ambient factor texture as AOMap ONLY if the ambientFactor value is > 0.0
            if (material.ambientFactor > 0.0f) {
                occlusionTextureID = ambientFactorTextures.value(material.materialID);
            }
        }

        if (!occlusionTextureID.isNull()) {
            occlusionTexture = getTexture(occlusionTextureID);
            detectDifferentUVs |= (occlusionTexture.texcoordSet != 0) || (!emissiveTexture.transform.isIdentity());
            material.occlusionTexture = occlusionTexture;
        }

        glm::vec2 lightmapParams(0.f, 1.f);
        lightmapParams.x = _lightmapOffset;
        lightmapParams.y = _lightmapLevel;

        FBXTexture ambientTexture;
        QString ambientTextureID = ambientTextures.value(material.materialID);
        if (ambientTextureID.isNull()) {
            // 2nd chance
            // For blender we use the ambient factor texture as Lightmap ONLY if the ambientFactor value is set to 0
            if (material.ambientFactor == 0.0f) {
                ambientTextureID = ambientFactorTextures.value(material.materialID);
            }
        }

        if (_loadLightmaps && !ambientTextureID.isNull()) {
            ambientTexture = getTexture(ambientTextureID);
            detectDifferentUVs |= (ambientTexture.texcoordSet != 0) || (!ambientTexture.transform.isIdentity());
            material.lightmapTexture = ambientTexture;
            material.lightmapParams = lightmapParams;
        }

        // Finally create the true material representation
        material._material = std::make_shared<model::Material>();

        // Emissive color is the mix of emissiveColor with emissiveFactor
        auto emissive = material.emissiveColor * (isMaterialLambert ? 1.0f : material.emissiveFactor); // In lambert there is not emissiveFactor
        material._material->setEmissive(emissive);

        // Final diffuse color is the mix of diffuseColor with diffuseFactor
        auto diffuse = material.diffuseColor * material.diffuseFactor;
        material._material->setAlbedo(diffuse);

        if (material.isPBSMaterial) {
            material._material->setRoughness(material.roughness);
            material._material->setMetallic(material.metallic);
        } else {
            material._material->setRoughness(model::Material::shininessToRoughness(material.shininess));
            float metallic = std::max(material.specularColor.x, std::max(material.specularColor.y, material.specularColor.z));
            material._material->setMetallic(metallic);

            if (isMaterialLambert) {
                if (!material._material->getKey().isAlbedo()) {
                    // switch emissive to material albedo as we tag the material to unlit
                    material._material->setUnlit(true);
                    material._material->setAlbedo(emissive);

                    if (!material.emissiveTexture.isNull()) {
                        material.albedoTexture = material.emissiveTexture;
                    }
                }
            }
        }
        qCDebug(modelformat) << " fbx material Name:" << material.name;

        if (materialMap.contains(material.name)) {
            QJsonObject materialOptions = materialMap.value(material.name).toObject();
            qCDebug(modelformat) << "Mapping fbx material:" << material.name << " with HifiMaterial: " << materialOptions;

            if (materialOptions.contains("scattering")) {
                float scattering = (float) materialOptions.value("scattering").toDouble();
                material._material->setScattering(scattering);
            }

            if (materialOptions.contains("scatteringMap")) {
                QByteArray scatteringMap = materialOptions.value("scatteringMap").toVariant().toByteArray();
                material.scatteringTexture = FBXTexture();
                material.scatteringTexture.name = material.name + ".scatteringMap";
                material.scatteringTexture.filename = scatteringMap;
            }
        }

        if (material.opacity <= 0.0f) {
            material._material->setOpacity(1.0f);
        } else {
            material._material->setOpacity(material.opacity);
        }
    }
}
