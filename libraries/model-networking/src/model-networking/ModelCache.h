//
//  ModelCache.h
//  libraries/model-networking/src/model-networking
//
//  Created by Sam Gateau on 9/21/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelCache_h
#define hifi_ModelCache_h

#include <QMap>
#include <QRunnable>

#include <DependencyManager.h>
#include <ResourceCache.h>

#include "FBXReader.h"
#include "OBJReader.h"

#include <gpu/Batch.h>
#include <gpu/Stream.h>


#include <model/Material.h>
#include <model/Asset.h>

class NetworkGeometry;
class NetworkMesh;
class NetworkTexture;
class NetworkMaterial;
class NetworkShape;

/// Stores cached geometry.
class ModelCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    virtual QSharedPointer<Resource> createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
                                                    bool delayLoad, const void* extra);

    /// Loads geometry from the specified URL.
    /// \param fallback a fallback URL to load if the desired one is unavailable
    /// \param delayLoad if true, don't load the geometry immediately; wait until load is first requested
    QSharedPointer<NetworkGeometry> getGeometry(const QUrl& url, const QUrl& fallback = QUrl(), bool delayLoad = false);

    /// Set a batch to the simple pipeline, returning the previous pipeline
    void useSimpleDrawPipeline(gpu::Batch& batch, bool noBlend = false);

private:
    ModelCache();
    virtual ~ModelCache();

    QHash<QUrl, QWeakPointer<NetworkGeometry> > _networkGeometry;
};

class NetworkGeometry : public QObject {
    Q_OBJECT

public:
    // mapping is only used if url is a .fbx or .obj file, it is essentially the content of an fst file.
    // if delayLoad is true, the url will not be immediately downloaded.
    // use the attemptRequest method to initiate the download.
    NetworkGeometry(const QUrl& url, bool delayLoad, const QVariantHash& mapping, const QUrl& textureBaseUrl = QUrl());
    ~NetworkGeometry();

    const QUrl& getURL() const { return _url; }

    void attemptRequest();

    // true when the geometry is loaded (but maybe not it's associated textures)
    bool isLoaded() const;

    // true when the requested geometry and its textures are loaded.
    bool isLoadedWithTextures() const;

    // WARNING: only valid when isLoaded returns true.
    const FBXGeometry& getFBXGeometry() const { return *_geometry; }
    const std::vector<std::unique_ptr<NetworkMesh>>& getMeshes() const { return _meshes; }
  //  const model::AssetPointer getAsset() const { return _asset; }

   // model::MeshPointer getShapeMesh(int shapeID);
   // int getShapePart(int shapeID);

    // This would be the final verison
    // model::MaterialPointer getShapeMaterial(int shapeID);
    const NetworkMaterial* getShapeMaterial(int shapeID);


    void setTextureWithNameToURL(const QString& name, const QUrl& url);
    QStringList getTextureNames() const;

    enum Error {
        MissingFilenameInMapping = 0,
        MappingRequestError,
        ModelRequestError,
        ModelParseError
    };

signals:
    // Fired when everything has downloaded and parsed successfully.
    void onSuccess(NetworkGeometry& networkGeometry, FBXGeometry& fbxGeometry);

    // Fired when something went wrong.
    void onFailure(NetworkGeometry& networkGeometry, Error error);

protected slots:
    void mappingRequestDone(const QByteArray& data);
    void mappingRequestError(QNetworkReply::NetworkError error);

    void modelRequestDone(const QByteArray& data);
    void modelRequestError(QNetworkReply::NetworkError error);

    void modelParseSuccess(FBXGeometry* geometry);
    void modelParseError(int error, QString str);

protected:
    void attemptRequestInternal();
    void requestMapping(const QUrl& url);
    void requestModel(const QUrl& url);

    enum State { DelayState,
                 RequestMappingState,
                 RequestModelState,
                 ParsingModelState,
                 SuccessState,
                 ErrorState };
    State _state;

    QUrl _url;
    QUrl _mappingUrl;
    QUrl _modelUrl;
    QVariantHash _mapping;
    QUrl _textureBaseUrl;

    Resource* _resource = nullptr;
    std::unique_ptr<FBXGeometry> _geometry; // This should go away evenutally once we can put everything we need in the model::AssetPointer
    std::vector<std::unique_ptr<NetworkMesh>> _meshes;
    std::vector<std::unique_ptr<NetworkMaterial>> _materials;
    std::vector<std::unique_ptr<NetworkShape>> _shapes;


    // The model asset created from this NetworkGeometry
    // model::AssetPointer _asset;

    // cache for isLoadedWithTextures()
    mutable bool _isLoadedWithTextures = false;
};

/// Reads geometry in a worker thread.
class GeometryReader : public QObject, public QRunnable {
    Q_OBJECT
public:
    GeometryReader(const QUrl& url, const QByteArray& data, const QVariantHash& mapping);
    virtual void run();
signals:
    void onSuccess(FBXGeometry* geometry);
    void onError(int error, QString str);
private:
    QUrl _url;
    QByteArray _data;
    QVariantHash _mapping;
};


class NetworkShape {
public:
    int _meshID{ -1 };
    int _partID{ -1 };
    int _materialID{ -1 };
};

class NetworkMaterial {
public:
    model::MaterialPointer _material;
    QString diffuseTextureName;
    QSharedPointer<NetworkTexture> diffuseTexture;
    QString normalTextureName;
    QSharedPointer<NetworkTexture> normalTexture;
    QString specularTextureName;
    QSharedPointer<NetworkTexture> specularTexture;
    QString emissiveTextureName;
    QSharedPointer<NetworkTexture> emissiveTexture;
};


/// The state associated with a single mesh.
class NetworkMesh {
public:
    model::MeshPointer _mesh;
};

#endif // hifi_GeometryCache_h
