//
//  Created by Sam Gondelman on 2/9/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_MaterialCache_h
#define hifi_MaterialCache_h

#include <QtCore/QSharedPointer>

#include "glm/glm.hpp"

#include <ResourceCache.h>
#include <graphics/Material.h>
#include <hfm/HFM.h>

#include <material-networking/TextureCache.h>

class NetworkMaterial : public graphics::Material {
public:
    using MapChannel = graphics::Material::MapChannel;

    NetworkMaterial() : _textures(MapChannel::NUM_MAP_CHANNELS) {}
    NetworkMaterial(const HFMMaterial& material, const QUrl& textureBaseUrl);
    NetworkMaterial(const NetworkMaterial& material);

    void setAlbedoMap(const QUrl& url, bool useAlphaChannel);
    void setNormalMap(const QUrl& url, bool isBumpmap);
    void setRoughnessMap(const QUrl& url, bool isGloss);
    void setMetallicMap(const QUrl& url, bool isSpecular);
    void setOcclusionMap(const QUrl& url);
    void setEmissiveMap(const QUrl& url);
    void setScatteringMap(const QUrl& url);
    void setLightMap(const QUrl& url);

    virtual bool isMissingTexture();
    virtual bool checkResetOpacityMap();

    class Texture {
    public:
        QString name;
        NetworkTexturePointer texture;
    };
    struct MapChannelHash {
        std::size_t operator()(MapChannel mapChannel) const {
            return static_cast<std::size_t>(mapChannel);
        }
    };
    using Textures = std::unordered_map<MapChannel, Texture, MapChannelHash>;
    Textures getTextures() { return _textures; }

protected:
    friend class Geometry;

    Textures _textures;

    static const QString NO_TEXTURE;
    const QString& getTextureName(MapChannel channel);

    void setTextures(const QVariantMap& textureMap);

    const bool& isOriginal() const { return _isOriginal; }

private:
    // Helpers for the ctors
    QUrl getTextureUrl(const QUrl& baseUrl, const HFMTexture& hfmTexture);
    graphics::TextureMapPointer fetchTextureMap(const QUrl& baseUrl, const HFMTexture& hfmTexture,
                                                image::TextureUsage::Type type, MapChannel channel);
    graphics::TextureMapPointer fetchTextureMap(const QUrl& url, image::TextureUsage::Type type, MapChannel channel);

    Transform _albedoTransform;
    Transform _lightmapTransform;
    vec2 _lightmapParams;

    bool _isOriginal { true };
};

class NetworkMaterialResource : public Resource {
public:
    NetworkMaterialResource() : Resource() {}
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
    static ParsedMaterials parseMaterialForUUID(const QJsonValue& entityIDJSON);
    static std::pair<std::string, std::shared_ptr<NetworkMaterial>> parseJSONMaterial(const QJsonValue& materialJSONValue, const QUrl& baseUrl = QUrl());

private:
    static bool parseJSONColor(const QJsonValue& array, glm::vec3& color, bool& isSRGB);
};

using NetworkMaterialResourcePointer = QSharedPointer<NetworkMaterialResource>;
using MaterialMapping = std::vector<std::pair<std::string, NetworkMaterialResourcePointer>>;
Q_DECLARE_METATYPE(MaterialMapping)

class MaterialCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    NetworkMaterialResourcePointer getMaterial(const QUrl& url);

protected:
    virtual QSharedPointer<Resource> createResource(const QUrl& url) override;
    QSharedPointer<Resource> createResourceCopy(const QSharedPointer<Resource>& resource) override;
};

#endif
