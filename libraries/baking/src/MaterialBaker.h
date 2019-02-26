//
//  MaterialBaker.h
//  libraries/baking/src
//
//  Created by Sam Gondelman on 2/26/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MaterialBaker_h
#define hifi_MaterialBaker_h

#include "Baker.h"

#include "TextureBaker.h"

#include <material-networking/MaterialCache.h>

static const QString BAKED_MATERIAL_EXTENSION = ".baked.json";

class MaterialBaker : public Baker {
    Q_OBJECT
public:
    MaterialBaker(const QString& materialData, bool isURL, const QString& bakedOutputDir);

    QString getMaterialData() const { return _materialData; }
    bool isURL() const { return _isURL; }
    QString getBakedMaterialData() const { return _bakedMaterialData; }

public slots:
    virtual void bake() override;

signals:
    void originalMaterialLoaded();

private slots:
    void processMaterial();
    void outputMaterial();

private:
    void loadMaterial();

    QString _materialData;
    bool _isURL;

    NetworkMaterialResourcePointer _materialResource;
    size_t _numTexturesToLoad { 0 };
    size_t _numTexturesLoaded { 0 };

    QHash<QUrl, QSharedPointer<TextureBaker>> _textureBakers;

    QString _bakedOutputDir;
    QString _bakedMaterialData;
};

#endif // !hifi_MaterialBaker_h
