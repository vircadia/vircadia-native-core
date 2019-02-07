//
//  Created by Sam Gondelman on 2/7/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ApplyMaterialMappingTask.h"

#include "ModelBakerLogging.h"

void ApplyMaterialMappingTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& materialsIn = input.get0();
    const auto& mapping = input.get1();

    auto materialsOut = materialsIn;

    auto mappingIter = mapping.find("materialMap");
    if (mappingIter != mapping.end()) {
        QByteArray materialMapValue = mappingIter.value().toByteArray();
        QJsonObject materialMap = QJsonDocument::fromJson(materialMapValue).object();
        if (materialMap.isEmpty()) {
            qCDebug(model_baker) << "Material Map found but did not produce valid JSON:" << materialMapValue;
        } else {
            for (auto& material : materialsOut) {
                auto materialMapIter = materialMap.find(material.name);
                if (materialMapIter != materialMap.end()) {
                    QJsonObject materialOptions = materialMapIter.value().toObject();
                    qCDebug(model_baker) << "Mapping material:" << material.name << " with HFMaterial: " << materialOptions;

                    {
                        auto scatteringIter = materialOptions.find("scattering");
                        if (scatteringIter != materialOptions.end()) {
                            float scattering = (float)scatteringIter.value().toDouble();
                            material._material->setScattering(scattering);
                        }
                    }

                    {
                        auto scatteringMapIter = materialOptions.find("scatteringMap");
                        if (scatteringMapIter != materialOptions.end()) {
                            QByteArray scatteringMap = scatteringMapIter.value().toVariant().toByteArray();
                            material.scatteringTexture = HFMTexture();
                            material.scatteringTexture.name = material.name + ".scatteringMap";
                            material.scatteringTexture.filename = scatteringMap;
                        }
                    }
                }
            }
        }
    }

    output = materialsOut;
}
