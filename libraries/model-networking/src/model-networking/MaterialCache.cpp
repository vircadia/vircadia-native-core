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

NetworkMaterialResource::NetworkMaterialResource(const QUrl& url) :
    Resource(url) {}

void NetworkMaterialResource::downloadFinished(const QByteArray& data) {
    if (_url.toString().endsWith(".json")) {
        QJsonDocument materialJSON = QJsonDocument::fromJson(data);
        if (!materialJSON.isNull() && materialJSON.isObject()) {
            auto parsedMaterial = parseJSONMaterial(materialJSON.object());
            name = parsedMaterial.first;
            networkMaterial = parsedMaterial.second;
        }
    }

    // TODO: parse other material types

    finishedLoading(true);
}

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
    }
    return false;
}

std::pair<QString, std::shared_ptr<NetworkMaterial>> NetworkMaterialResource::parseJSONMaterial(const QJsonObject& materialJSON) {
    QString name = "";
    std::shared_ptr<NetworkMaterial> material = std::make_shared<NetworkMaterial>();
    for (auto& key : materialJSON.keys()) {
        if (key == "name") {
            auto nameJSON = materialJSON.value(key);
            if (nameJSON.isString()) {
                name = nameJSON.toString();
            }
        } else if (key == "emissive") {
            glm::vec3 color;
            bool isSRGB;
            bool valid = parseJSONColor(materialJSON.value(key), color, isSRGB);
            if (valid) {
                material->setEmissive(color, isSRGB);
            }
        } else if (key == "opacity") {
            auto value = materialJSON.value(key);
            if (value.isDouble()) {
                material->setOpacity(value.toDouble());
            }
        } else if (key == "albedo") {
            glm::vec3 color;
            bool isSRGB;
            bool valid = parseJSONColor(materialJSON.value(key), color, isSRGB);
            if (valid) {
                material->setAlbedo(color, isSRGB);
            }
        } else if (key == "roughness") {
            auto value = materialJSON.value(key);
            if (value.isDouble()) {
                material->setRoughness(value.toDouble());
            }
        } else if (key == "fresnel") {
            glm::vec3 color;
            bool isSRGB;
            bool valid = parseJSONColor(materialJSON.value(key), color, isSRGB);
            if (valid) {
                material->setFresnel(color, isSRGB);
            }
        } else if (key == "metallic") {
            auto value = materialJSON.value(key);
            if (value.isDouble()) {
                material->setMetallic(value.toDouble());
            }
        } else if (key == "scattering") {
            auto value = materialJSON.value(key);
            if (value.isDouble()) {
                material->setScattering(value.toDouble());
            }
        } else if (key == "emissiveMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setEmissiveMap(value.toString());
            }
        } else if (key == "albedoMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                bool useAlphaChannel = false;
                auto opacityMap = materialJSON.find("opacityMap");
                if (opacityMap != materialJSON.end() && opacityMap->isString() && opacityMap->toString() == value.toString()) {
                    useAlphaChannel = true;
                }
                material->setAlbedoMap(value.toString(), useAlphaChannel);
            }
        } else if (key == "roughnessMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setRoughnessMap(value.toString(), false);
            }
        } else if (key == "glossMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setRoughnessMap(value.toString(), true);
            }
        } else if (key == "metallicMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setMetallicMap(value.toString(), false);
            }
        } else if (key == "specularMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setMetallicMap(value.toString(), true);
            }
        } else if (key == "normalMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setNormalMap(value.toString(), false);
            }
        } else if (key == "bumpMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setNormalMap(value.toString(), true);
            }
        } else if (key == "occlusionMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setOcclusionMap(value.toString());
            }
        } else if (key == "scatteringMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setScatteringMap(value.toString());
            }
        } else if (key == "lightMap") {
            auto value = materialJSON.value(key);
            if (value.isString()) {
                material->setLightmapMap(value.toString());
            }
        }
    }
    return std::pair<QString, std::shared_ptr<NetworkMaterial>>(name, material);
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