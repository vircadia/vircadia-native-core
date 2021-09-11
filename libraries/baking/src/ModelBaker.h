//
//  ModelBaker.h
//  libraries/baking/src
//
//  Created by Utkarsh Gautam on 9/29/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelBaker_h
#define hifi_ModelBaker_h

#include <QtCore/QFutureSynchronizer>
#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>
#include <QJsonArray>
#include <QtCore/QSharedPointer>

#include "Baker.h"
#include "MaterialBaker.h"

#include "ModelBakingLoggingCategory.h"

#include <FBX.h>
#include <hfm/HFM.h>

using GetMaterialIDCallback = std::function <int(int)>;

static const QString FST_EXTENSION { ".fst" };
static const QString BAKED_FST_EXTENSION { ".baked.fst" };
static const QString FBX_EXTENSION { ".fbx" };
static const QString BAKED_FBX_EXTENSION { ".baked.fbx" };
static const QString OBJ_EXTENSION { ".obj" };
static const QString GLTF_EXTENSION { ".gltf" };

class ModelBaker : public Baker {
    Q_OBJECT

public:
    ModelBaker(const QUrl& inputModelURL, const QString& bakedOutputDirectory, const QString& originalOutputDirectory = "", bool hasBeenBaked = false);

    void setOutputURLSuffix(const QUrl& urlSuffix);
    void setMappingURL(const QUrl& mappingURL);
    void setMapping(const hifi::VariantHash& mapping);

    void initializeOutputDirs();

    bool buildDracoMeshNode(FBXNode& dracoMeshNode, const QByteArray& dracoMeshBytes, const std::vector<hifi::ByteArray>& dracoMaterialList);
    virtual void setWasAborted(bool wasAborted) override;

    QUrl getModelURL() const { return _modelURL; }
    QUrl getOriginalInputModelURL() const { return _originalInputModelURL; }
    virtual QUrl getFullOutputMappingURL() const;
    QUrl getBakedModelURL() const { return _bakedModelURL; }

signals:
    void modelLoaded();

public slots:
    virtual void bake() override;
    virtual void abort() override;

protected:
    void saveSourceModel();
    virtual void bakeProcessedSource(const hfm::Model::Pointer& hfmModel, const std::vector<hifi::ByteArray>& dracoMeshes, const std::vector<std::vector<hifi::ByteArray>>& dracoMaterialLists) = 0;
    void exportScene();

    FBXNode _rootNode;
    QUrl _originalInputModelURL;
    QUrl _modelURL;
    QUrl _outputURLSuffix;
    QUrl _mappingURL;
    hifi::VariantHash _mapping;
    QString _bakedOutputDir;
    QString _originalOutputDir;
    QString _originalOutputModelPath;
    QString _outputMappingURL;
    QUrl _bakedModelURL;

protected slots:
    void handleModelNetworkReply();
    virtual void bakeSourceCopy();
    void handleFinishedMaterialBaker();
    void handleFinishedMaterialMapBaker();

private:
    void outputUnbakedFST();
    void outputBakedFST();
    void bakeMaterialMap();

    bool _hasBeenBaked { false };

    hfm::Model::Pointer _hfmModel;
    MaterialMapping _materialMapping;
    int _materialMapIndex { 0 };
    QJsonArray _materialMappingJSON;
    QSharedPointer<MaterialBaker> _materialBaker;
};

#endif // hifi_ModelBaker_h
