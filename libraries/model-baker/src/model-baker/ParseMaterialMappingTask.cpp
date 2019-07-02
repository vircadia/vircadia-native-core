//
//  Created by Sam Gondelman on 2/7/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ParseMaterialMappingTask.h"

#include "ModelBakerLogging.h"

#include <QJsonArray>

void processMaterialMapping(MaterialMapping& materialMapping, const QJsonObject& materialMap, const hifi::URL& url) {
    auto mappingKeys = materialMap.keys();
    for (auto mapping : mappingKeys) {
        auto mappingJSON = materialMap[mapping];
        if (mappingJSON.isObject()) {
            auto mappingValue = mappingJSON.toObject();

            // Old subsurface scattering mapping
            {
                auto scatteringIter = mappingValue.find("scattering");
                auto scatteringMapIter = mappingValue.find("scatteringMap");
                if (scatteringIter != mappingValue.end() || scatteringMapIter != mappingValue.end()) {
                    std::shared_ptr<NetworkMaterial> material = std::make_shared<NetworkMaterial>();

                    if (scatteringIter != mappingValue.end()) {
                        float scattering = (float)scatteringIter.value().toDouble();
                        material->setScattering(scattering);
                    }

                    if (scatteringMapIter != mappingValue.end()) {
                        QString scatteringMap = scatteringMapIter.value().toString();
                        material->setScatteringMap(scatteringMap);
                    }

                    material->setDefaultFallthrough(true);

                    NetworkMaterialResourcePointer materialResource = NetworkMaterialResourcePointer(new NetworkMaterialResource(),
                                                                        [](NetworkMaterialResource* ptr) { ptr->deleteLater(); });
                    materialResource->moveToThread(qApp->thread());
                    materialResource->parsedMaterials.names.push_back("scattering");
                    materialResource->parsedMaterials.networkMaterials["scattering"] = material;

                    materialMapping.push_back(std::pair<std::string, NetworkMaterialResourcePointer>("mat::" + mapping.toStdString(), materialResource));
                    continue;
                }
            }

            // Material JSON description
            {
                NetworkMaterialResourcePointer materialResource = NetworkMaterialResourcePointer(new NetworkMaterialResource(),
                                                                    [](NetworkMaterialResource* ptr) { ptr->deleteLater(); });
                materialResource->moveToThread(qApp->thread());
                materialResource->parsedMaterials = NetworkMaterialResource::parseJSONMaterials(QJsonDocument(mappingValue), url);
                materialMapping.push_back(std::pair<std::string, NetworkMaterialResourcePointer>(mapping.toStdString(), materialResource));
            }

        } else if (mappingJSON.isString()) {
            auto mappingValue = mappingJSON.toString();
            materialMapping.push_back(std::pair<std::string, NetworkMaterialResourcePointer>(mapping.toStdString(),
                DependencyManager::get<MaterialCache>()->getMaterial(url.resolved(mappingValue))));
        }
    }
}

void ParseMaterialMappingTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& mapping = input.get0();
    const auto& url = input.get1();
    MaterialMapping materialMapping;

    auto mappingIter = mapping.find("materialMap");
    if (mappingIter != mapping.end()) {
        QByteArray materialMapValue = mappingIter.value().toByteArray();
        QJsonDocument materialMapJSON = QJsonDocument::fromJson(materialMapValue);
        if (materialMapJSON.isEmpty()) {
            qCDebug(model_baker) << "Material Map found but did not produce valid JSON:" << materialMapValue;
        } else if (materialMapJSON.isObject()) {
            QJsonObject materialMap = materialMapJSON.object();
            processMaterialMapping(materialMapping, materialMap, url);
        } else {
            QJsonArray materialMapArray = materialMapJSON.array();
            for (auto materialMapIter : materialMapArray) {
                if (materialMapIter.isObject()) {
                    QJsonObject materialMap = materialMapIter.toObject();
                    processMaterialMapping(materialMapping, materialMap, url);
                }
            }
        }
    }

    output = materialMapping;
}
