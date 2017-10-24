//
//  OBJBaker.h
//  libraries/baking/src
//
//  Created by Utkarsh Gautam on 9/29/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OBJBaker_h
#define hifi_OBJBaker_h

#include "Baker.h"
#include "TextureBaker.h"
#include "ModelBaker.h"

#include "ModelBakingLoggingCategory.h"

using TextureBakerThreadGetter = std::function<QThread*()>;

class OBJBaker : public ModelBaker {
    Q_OBJECT
public:
    OBJBaker(const QUrl& objURL, TextureBakerThreadGetter textureThreadGetter,
             const QString& bakedOutputDir, const QString& originalOutputDir = "");
    ~OBJBaker() override;
    void loadOBJ();
    void createFBXNodeTree(FBXNode& rootNode, FBXGeometry& geometry);
    void setNodeProperties(FBXNode& parentNode);
    void setMaterialNodeProperties(FBXNode& materialNode, QString material, FBXGeometry& geometry);
    template<typename N>
    void setPropertiesList(std::vector<QByteArray>& stringProperties, std::vector<N>& numericProperties, QVariantList& propertiesList);

public slots:
    virtual void bake() override;

signals:
    void OBJLoaded();

private slots:
    void bakeOBJ();
    void handleOBJNetworkReply();

private:
    qlonglong _nodeID = 0;
    QUrl _objURL;
    QString _bakedOBJFilePath;
    QString _bakedOutputDir;
    QString _originalOutputDir;
    QDir _tempDir;
    QString _originalOBJFilePath;
    TextureBakerThreadGetter _textureThreadGetter;
    QMultiHash<QUrl, QSharedPointer<TextureBaker>> _bakingTextures;

    qlonglong _geometryID;
    qlonglong _modelID;
    std::vector<qlonglong> _materialIDs;
    qlonglong _textureID;
    std::vector<QPair<qlonglong, int>> _mapTextureMaterial;
    FBXNode _objectNode;
};
#endif // hifi_OBJBaker_h
