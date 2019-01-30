//
//  Created by Sam Gondelman on 2/9/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MaterialCache.h"

#include "QJsonObject"
#include "QJsonDocument"
#include "QJsonArray"

#include "RegisteredMetaTypes.h"

NetworkMaterialResource::NetworkMaterialResource(const QUrl& url) :
    Resource(url) {}

void NetworkMaterialResource::downloadFinished(const QByteArray& data) {
    parsedMaterials.reset();

    if (_url.toString().contains(".json")) {
        parsedMaterials = parseJSONMaterials(QJsonDocument::fromJson(data), _url);
    }

    // TODO: parse other material types

    finishedLoading(true);
}

/**jsdoc
 * <p>An RGB or SRGB color value.</p>
 * <table>
 *   <thead>
 *     <tr><th>Index</th><th>Type</th><th>Attributes</th><th>Default</th><th>Value</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>0</code></td><td>number</td><td></td><td></td>
 *       <td>Red component value. Number in the range <code>0.0</code> &ndash; <code>1.0</code>.</td></tr>
 *     <tr><td><code>1</code></td><td>number</td><td></td><td></td>
 *       <td>Green component value. Number in the range <code>0.0</code> &ndash; <code>1.0</code>.</td></tr>
 *     <tr><td><code>2</code></td><td>number</td><td></td><td></td>
 *       <td>Blue component value. Number in the range <code>0.0</code> &ndash; <code>1.0</code>.</td></tr>
 *     <tr><td><code>3</code></td><td>boolean</td><td>&lt;optional&gt;</td><td>false</td>
 *       <td>If <code>true</code> then the color is an SRGB color.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {array} RGBS
 */
bool NetworkMaterialResource::parseJSONColor(const QJsonValue& array, glm::vec3& color, bool& isSRGB) {
    if (array.isArray()) {
        QJsonArray colorArray = array.toArray();
        if (colorArray.size() >= 3 && colorArray[0].isDouble() && colorArray[1].isDouble() && colorArray[2].isDouble()) {
            isSRGB = true;
            if (colorArray.size() >= 4) {
                if (colorArray[3].isBool()) {
                    isSRGB = colorArray[3].toBool();
                }
            }
            color = glm::vec3(colorArray[0].toDouble(), colorArray[1].toDouble(), colorArray[2].toDouble());
            return true;
        }
    } else if (array.isObject()) {
        bool toReturn;
        isSRGB = true;
        color = vec3FromVariant(array.toObject(), toReturn);
        return toReturn;
    }
    return false;
}

/**jsdoc
 * A material or set of materials such as may be used by a {@link Entities.EntityType|Material} entity.
 * @typedef {object} MaterialResource
 * @property {number} materialVersion=1 - The version of the material. <em>Currently not used.</em>
 * @property {Material|Material[]} materials - The details of the material or materials.
 */
NetworkMaterialResource::ParsedMaterials NetworkMaterialResource::parseJSONMaterials(const QJsonDocument& materialJSON, const QUrl& baseUrl) {
    ParsedMaterials toReturn;
    if (!materialJSON.isNull() && materialJSON.isObject()) {
        QJsonObject materialJSONObject = materialJSON.object();
        for (auto& key : materialJSONObject.keys()) {
            if (key == "materialVersion") {
                auto value = materialJSONObject.value(key);
                if (value.isDouble()) {
                    toReturn.version = (uint)value.toInt();
                }
            } else if (key == "materials") {
                auto materialsValue = materialJSONObject.value(key);
                if (materialsValue.isArray()) {
                    QJsonArray materials = materialsValue.toArray();
                    for (auto material : materials) {
                        if (!material.isNull() && material.isObject()) {
                            auto parsedMaterial = parseJSONMaterial(material.toObject(), baseUrl);
                            toReturn.networkMaterials[parsedMaterial.first] = parsedMaterial.second;
                            toReturn.names.push_back(parsedMaterial.first);
                        }
                    }
                } else if (materialsValue.isObject()) {
                    auto parsedMaterial = parseJSONMaterial(materialsValue.toObject(), baseUrl);
                    toReturn.networkMaterials[parsedMaterial.first] = parsedMaterial.second;
                    toReturn.names.push_back(parsedMaterial.first);
                }
            }
        }
    }

    return toReturn;
}

/**jsdoc
 * A material such as may be used by a {@link Entities.EntityType|Material} entity.
 * @typedef {object} Material
 * @property {string} model="hifi_pbr" - Different material models support different properties and rendering modes.
 *     Supported models are: "hifi_pbr"
 * @property {string} name="" - A name for the material. Supported by all material models.
 * @property {Color|RGBS|string} emissive - The emissive color, i.e., the color that the material emits. A {@link Color} value
 *     is treated as sRGB. A {@link RGBS} value can be either RGB or sRGB.  Set to <code>"fallthrough"</code> to fallthrough to
 *     the material below.  "hifi_pbr" model only.
 * @property {number|string} opacity=1.0 - The opacity, <code>0.0</code> &ndash; <code>1.0</code>.  Set to <code>"fallthrough"</code> to fallthrough to
 *     the material below.  "hifi_pbr" model only.
 * @property {boolean|string} unlit=false - If <code>true</code>, the material is not lit.  Set to <code>"fallthrough"</code> to fallthrough to
 *     the material below.  "hifi_pbr" model only.
 * @property {Color|RGBS|string} albedo - The albedo color. A {@link Color} value is treated as sRGB. A {@link RGBS} value can
 *     be either RGB or sRGB.  Set to <code>"fallthrough"</code> to fallthrough to the material below.  Set to <code>"fallthrough"</code> to fallthrough to
 *     the material below.  "hifi_pbr" model only.
 * @property {number|string} roughness - The roughness, <code>0.0</code> &ndash; <code>1.0</code>.  Set to <code>"fallthrough"</code> to fallthrough to
 *     the material below.  "hifi_pbr" model only.
 * @property {number|string} metallic - The metallicness, <code>0.0</code> &ndash; <code>1.0</code>.  Set to <code>"fallthrough"</code> to fallthrough to
 *     the material below.  "hifi_pbr" model only.
 * @property {number|string} scattering - The scattering, <code>0.0</code> &ndash; <code>1.0</code>.  Set to <code>"fallthrough"</code> to fallthrough to
 *     the material below.  "hifi_pbr" model only.
 * @property {string} emissiveMap - URL of emissive texture image.  Set to <code>"fallthrough"</code> to fallthrough to
 *     the material below.  "hifi_pbr" model only.
 * @property {string} albedoMap - URL of albedo texture image.  Set to <code>"fallthrough"</code> to fallthrough to
 *     the material below.  "hifi_pbr" model only.
 * @property {string} opacityMap - URL of opacity texture image. Set value the same as the <code>albedoMap</code> value for 
 *     transparency.  "hifi_pbr" model only.
 * @property {string} roughnessMap - URL of roughness texture image. Can use this or <code>glossMap</code>, but not both.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} glossMap - URL of gloss texture image. Can use this or <code>roughnessMap</code>, but not both.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} metallicMap - URL of metallic texture image. Can use this or <code>specularMap</code>, but not both.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} specularMap - URL of specular texture image. Can use this or <code>metallicMap</code>, but not both.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} normalMap - URL of normal texture image. Can use this or <code>bumpMap</code>, but not both.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} bumpMap - URL of bump texture image. Can use this or <code>normalMap</code>, but not both.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} occlusionMap - URL of occlusion texture image.  Set to <code>"fallthrough"</code> to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} scatteringMap - URL of scattering texture image. Only used if <code>normalMap</code> or 
 *     <code>bumpMap</code> is specified.  Set to <code>"fallthrough"</code> to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} lightMap - URL of light map texture image. <em>Currently not used.</em>.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} texCoordTransform0 - The transform to use for all of the maps besides occlusionMap and lightMap.  Currently unused.  Set to
 *     <code>"fallthrough"</code> to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} texCoordTransform1 - The transform to use for occlusionMap and lightMap.  Currently unused.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} lightmapParams - Parameters for controlling how lightMap is used.  Currently unused.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {string} materialParams - Parameters for controlling the material projection and repition.  Currently unused.  Set to <code>"fallthrough"</code>
 *     to fallthrough to the material below.  "hifi_pbr" model only.
 * @property {bool} defaultFallthrough=false - If <code>true</code>, all properties will fallthrough to the material below unless they are set.  If
 *     <code>false</code>, they will respect the individual properties' fallthrough state.  "hifi_pbr" model only.
 */
// Note: See MaterialEntityItem.h for default values used in practice.
std::pair<std::string, std::shared_ptr<NetworkMaterial>> NetworkMaterialResource::parseJSONMaterial(const QJsonObject& materialJSON, const QUrl& baseUrl) {
    std::string name = "";
    std::shared_ptr<NetworkMaterial> material = std::make_shared<NetworkMaterial>();

    const std::string HIFI_PBR = "hifi_pbr";
    std::string modelString = HIFI_PBR;
    auto modelJSONIter = materialJSON.find("model");
    if (modelJSONIter != materialJSON.end() && modelJSONIter.value().isString()) {
        modelString = modelJSONIter.value().toString().toStdString();
        material->setModel(modelString);
    }

    if (modelString == HIFI_PBR) {
        const QString FALLTHROUGH("fallthrough");
        for (auto& key : materialJSON.keys()) {
            if (key == "name") {
                auto nameJSON = materialJSON.value(key);
                if (nameJSON.isString()) {
                    name = nameJSON.toString().toStdString();
                }
            } else if (key == "model") {
                auto modelJSON = materialJSON.value(key);
                if (modelJSON.isString()) {
                    material->setModel(modelJSON.toString().toStdString());
                }
            } else if (key == "emissive") {
                auto value = materialJSON.value(key);
                if (value.isString() && value.toString() == FALLTHROUGH) {
                    material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::EMISSIVE_VAL_BIT);
                } else {
                    glm::vec3 color;
                    bool isSRGB;
                    bool valid = parseJSONColor(value, color, isSRGB);
                    if (valid) {
                        material->setEmissive(color, isSRGB);
                    }
                }
            } else if (key == "opacity") {
                auto value = materialJSON.value(key);
                if (value.isString() && value.toString() == FALLTHROUGH) {
                    material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::OPACITY_VAL_BIT);
                } else if (value.isDouble()) {
                    material->setOpacity(value.toDouble());
                }
            } else if (key == "unlit") {
                auto value = materialJSON.value(key);
                if (value.isString() && value.toString() == FALLTHROUGH) {
                    material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::UNLIT_VAL_BIT);
                } else if (value.isBool()) {
                    material->setUnlit(value.toBool());
                }
            } else if (key == "albedo") {
                auto value = materialJSON.value(key);
                if (value.isString() && value.toString() == FALLTHROUGH) {
                    material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::ALBEDO_VAL_BIT);
                } else {
                    glm::vec3 color;
                    bool isSRGB;
                    bool valid = parseJSONColor(value, color, isSRGB);
                    if (valid) {
                        material->setAlbedo(color, isSRGB);
                    }
                }
            } else if (key == "roughness") {
                auto value = materialJSON.value(key);
                if (value.isString() && value.toString() == FALLTHROUGH) {
                    material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::GLOSSY_VAL_BIT);
                } else if (value.isDouble()) {
                    material->setRoughness(value.toDouble());
                }
            } else if (key == "metallic") {
                auto value = materialJSON.value(key);
                if (value.isString() && value.toString() == FALLTHROUGH) {
                    material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::METALLIC_VAL_BIT);
                } else if (value.isDouble()) {
                    material->setMetallic(value.toDouble());
                }
            } else if (key == "scattering") {
                auto value = materialJSON.value(key);
                if (value.isString() && value.toString() == FALLTHROUGH) {
                    material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::SCATTERING_VAL_BIT);
                } else if (value.isDouble()) {
                    material->setScattering(value.toDouble());
                }
            } else if (key == "emissiveMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::EMISSIVE_MAP_BIT);
                    } else {
                        material->setEmissiveMap(baseUrl.resolved(valueString));
                    }
                }
            } else if (key == "albedoMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    QString valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::ALBEDO_MAP_BIT);
                    } else {
                        bool useAlphaChannel = false;
                        auto opacityMap = materialJSON.find("opacityMap");
                        if (opacityMap != materialJSON.end() && opacityMap->isString() && opacityMap->toString() == valueString) {
                            useAlphaChannel = true;
                        }
                        material->setAlbedoMap(baseUrl.resolved(valueString), useAlphaChannel);
                    }
                }
            } else if (key == "roughnessMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::ROUGHNESS_MAP_BIT);
                    } else {
                        material->setRoughnessMap(baseUrl.resolved(valueString), false);
                    }
                }
            } else if (key == "glossMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::ROUGHNESS_MAP_BIT);
                    } else {
                        material->setRoughnessMap(baseUrl.resolved(valueString), true);
                    }
                }
            } else if (key == "metallicMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::METALLIC_MAP_BIT);
                    } else {
                        material->setMetallicMap(baseUrl.resolved(valueString), false);
                    }
                }
            } else if (key == "specularMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::METALLIC_MAP_BIT);
                    } else {
                        material->setMetallicMap(baseUrl.resolved(valueString), true);
                    }
                }
            } else if (key == "normalMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::NORMAL_MAP_BIT);
                    } else {
                        material->setNormalMap(baseUrl.resolved(valueString), false);
                    }
                }
            } else if (key == "bumpMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::NORMAL_MAP_BIT);
                    } else {
                        material->setNormalMap(baseUrl.resolved(valueString), true);
                    }
                }
            } else if (key == "occlusionMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::OCCLUSION_MAP_BIT);
                    } else {
                        material->setOcclusionMap(baseUrl.resolved(valueString));
                    }
                }
            } else if (key == "scatteringMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::SCATTERING_MAP_BIT);
                    } else {
                        material->setScatteringMap(baseUrl.resolved(valueString));
                    }
                }
            } else if (key == "lightMap") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::MaterialKey::FlagBit::LIGHTMAP_MAP_BIT);
                    } else {
                        material->setLightmapMap(baseUrl.resolved(valueString));
                    }
                }
            } else if (key == "texCoordTransform0") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::Material::ExtraFlagBit::TEXCOORDTRANSFORM0);
                    }
                }
                // TODO: implement texCoordTransform0
            } else if (key == "texCoordTransform1") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::Material::ExtraFlagBit::TEXCOORDTRANSFORM1);
                    }
                }
                // TODO: implement texCoordTransform1
            } else if (key == "lightmapParams") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::Material::ExtraFlagBit::LIGHTMAP_PARAMS);
                    }
                }
                // TODO: implement lightmapParams
            } else if (key == "materialParams") {
                auto value = materialJSON.value(key);
                if (value.isString()) {
                    auto valueString = value.toString();
                    if (valueString == FALLTHROUGH) {
                        material->setPropertyDoesFallthrough(graphics::Material::ExtraFlagBit::MATERIAL_PARAMS);
                    }
                }
                // TODO: implement materialParams
            } else if (key == "defaultFallthrough") {
                auto value = materialJSON.value(key);
                if (value.isBool()) {
                    material->setDefaultFallthrough(value.toBool());
                }
            }
        }
    }
    return std::pair<std::string, std::shared_ptr<NetworkMaterial>>(name, material);
}

MaterialCache& MaterialCache::instance() {
    static MaterialCache _instance;
    return _instance;
}

NetworkMaterialResourcePointer MaterialCache::getMaterial(const QUrl& url) {
    return ResourceCache::getResource(url, QUrl(), nullptr).staticCast<NetworkMaterialResource>();
}

QSharedPointer<Resource> MaterialCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback, const void* extra) {
    return QSharedPointer<Resource>(new NetworkMaterialResource(url), &Resource::deleter);
}