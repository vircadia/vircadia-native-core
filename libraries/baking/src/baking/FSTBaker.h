//
//  FSTBaker.h
//  libraries/baking/src/baking
//
//  Created by Sabrina Shanman on 2019/03/06.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FSTBaker_h
#define hifi_FSTBaker_h

#include "../ModelBaker.h"

class FSTBaker : public ModelBaker {
    Q_OBJECT

public:
    FSTBaker(const QUrl& inputMappingURL, const QUrl& destinationPath, const QString& bakedOutputDirectory, const QString& originalOutputDirectory = "", bool hasBeenBaked = false);

    virtual QUrl getFullOutputMappingURL() const override;

signals:
    void fstLoaded();

public slots:
    virtual void abort() override;
    
protected:
    std::unique_ptr<ModelBaker> _modelBaker;

protected slots:
    virtual void bakeSourceCopy() override;
    virtual void bakeProcessedSource(const hfm::Model::Pointer& hfmModel, const std::vector<hifi::ByteArray>& dracoMeshes, const std::vector<std::vector<hifi::ByteArray>>& dracoMaterialLists) override {};
    void handleModelBakerAborted();
    void handleModelBakerFinished();

private:
    void handleModelBakerEnded();
};

#endif // hifi_FSTBaker_h
