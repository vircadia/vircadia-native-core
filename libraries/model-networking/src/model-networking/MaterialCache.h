//
//  Created by Sam Gondelman on 2/9/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_MaterialCache_h
#define hifi_MaterialCache_h

#include <ResourceCache.h>

#include "glm/glm.hpp"

#include "ModelCache.h"

class NetworkMaterialResource : public Resource {
public:
    NetworkMaterialResource(const QUrl& url);

    QString getType() const override { return "NetworkMaterial"; }

    virtual void downloadFinished(const QByteArray& data) override;

    typedef struct ParsedMaterials {
        uint version { 1 };
        std::vector<std::string> names;
        std::unordered_map<std::string, std::shared_ptr<NetworkMaterial>> networkMaterials;

        void reset() {
            version = 1;
            names.clear();
            networkMaterials.clear();
        }

    } ParsedMaterials;

    ParsedMaterials parsedMaterials;

    static ParsedMaterials parseJSONMaterials(const QJsonDocument& materialJSON, const QUrl& baseUrl);
    static std::pair<std::string, std::shared_ptr<NetworkMaterial>> parseJSONMaterial(const QJsonObject& materialJSON, const QUrl& baseUrl);

private:
    static bool parseJSONColor(const QJsonValue& array, glm::vec3& color, bool& isSRGB);
};

using NetworkMaterialResourcePointer = QSharedPointer<NetworkMaterialResource>;

class MaterialCache : public ResourceCache {
public:
    static MaterialCache& instance();

    NetworkMaterialResourcePointer getMaterial(const QUrl& url);

protected:
    virtual QSharedPointer<Resource> createResource(const QUrl& url, const QSharedPointer<Resource>& fallback, const void* extra) override;
};

#endif
