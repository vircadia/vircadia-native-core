//
//  OBJBaker.cpp
//  libraries/baking/src
//
//  Created by Utkarsh Gautam on 9/29/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OBJBaker.h"

#include <PathUtils.h>
#include <NetworkAccessManager.h>

#include "OBJSerializer.h"
#include "FBXWriter.h"

const double UNIT_SCALE_FACTOR = 100.0;
const QByteArray PROPERTIES70_NODE_NAME = "Properties70";
const QByteArray P_NODE_NAME = "P";
const QByteArray C_NODE_NAME = "C";
const QByteArray FBX_HEADER_EXTENSION = "FBXHeaderExtension";
const QByteArray GLOBAL_SETTINGS_NODE_NAME = "GlobalSettings";
const QByteArray OBJECTS_NODE_NAME = "Objects";
const QByteArray GEOMETRY_NODE_NAME = "Geometry";
const QByteArray MODEL_NODE_NAME = "Model";
const QByteArray MATERIAL_NODE_NAME = "Material";
const QByteArray TEXTURE_NODE_NAME = "Texture";
const QByteArray TEXTURENAME_NODE_NAME = "TextureName";
const QByteArray RELATIVEFILENAME_NODE_NAME = "RelativeFilename";
const QByteArray CONNECTIONS_NODE_NAME = "Connections";
const QByteArray CONNECTIONS_NODE_PROPERTY = "OO";
const QByteArray CONNECTIONS_NODE_PROPERTY_1 = "OP";
const QByteArray MESH = "Mesh";

void OBJBaker::bakeProcessedSource(const hfm::Model::Pointer& hfmModel, const std::vector<hifi::ByteArray>& dracoMeshes, const std::vector<std::vector<hifi::ByteArray>>& dracoMaterialLists) {
    // Write OBJ Data as FBX tree nodes
    createFBXNodeTree(_rootNode, hfmModel, dracoMeshes[0], dracoMaterialLists[0]);
}

void OBJBaker::createFBXNodeTree(FBXNode& rootNode, const hfm::Model::Pointer& hfmModel, const hifi::ByteArray& dracoMesh, const std::vector<hifi::ByteArray>& dracoMaterialList) {
    // Make all generated nodes children of rootNode
    rootNode.children = { FBXNode(), FBXNode(), FBXNode() };
    FBXNode& globalSettingsNode = rootNode.children[0];
    FBXNode& objectNode = rootNode.children[1];
    FBXNode& connectionsNode = rootNode.children[2];

    // Generating FBX Header Node
    FBXNode headerNode;
    headerNode.name = FBX_HEADER_EXTENSION;

    // Generating global settings node
    // Required for Unit Scale Factor
    globalSettingsNode.name = GLOBAL_SETTINGS_NODE_NAME;

    // Setting the tree hierarchy: GlobalSettings -> Properties70 -> P -> Properties
    {
        globalSettingsNode.children.push_back(FBXNode());
        FBXNode& properties70Node = globalSettingsNode.children.back();
        properties70Node.name = PROPERTIES70_NODE_NAME;

        FBXNode pNode;
        {
            pNode.name = P_NODE_NAME;
            pNode.properties.append({
                "UnitScaleFactor", "double", "Number", "",
                UNIT_SCALE_FACTOR
            });
        }
        properties70Node.children = { pNode };

    }

    // Generating Object node
    objectNode.name = OBJECTS_NODE_NAME;
    objectNode.children = { FBXNode(), FBXNode() };
    FBXNode& geometryNode = objectNode.children[0];
    FBXNode& modelNode = objectNode.children[1];

    // Generating Object node's child - Geometry node
    geometryNode.name = GEOMETRY_NODE_NAME;
    NodeID geometryID;
    {
        geometryID = nextNodeID();
        geometryNode.properties = {
            geometryID,
            GEOMETRY_NODE_NAME,
            MESH
        };
    }
    
    // Generating Object node's child - Model node
    modelNode.name = MODEL_NODE_NAME;
    NodeID modelID;
    {
        modelID = nextNodeID();
        modelNode.properties = { modelID, MODEL_NODE_NAME, MESH };
    }

    // Generating Objects node's child - Material node

    // Each material ID should only appear once thanks to deduplication in BuildDracoMeshTask, but we want to make sure they are created in the right order
    std::unordered_map<QString, uint32_t> materialIDToIndex;
    for (uint32_t materialIndex = 0; materialIndex < hfmModel->materials.size(); ++materialIndex) {
        const auto& material = hfmModel->materials[materialIndex];
        materialIDToIndex[material.materialID] = materialIndex;
    }

    // Create nodes for each material in the material list
    for (const auto& dracoMaterial : dracoMaterialList) {
        const QString materialID = QString(dracoMaterial);
        const uint32_t materialIndex = materialIDToIndex[materialID];
        const auto& material = hfmModel->materials[materialIndex];
        FBXNode materialNode;
        materialNode.name = MATERIAL_NODE_NAME;
        setMaterialNodeProperties(materialNode, material.materialID, material, hfmModel);
        objectNode.children.append(materialNode);
    }

    // Store the draco node containing the compressed mesh information, along with the per-meshPart material IDs the draco node references
    // Because we redefine the material IDs when initializing the material nodes above, we pass that in for the material list
    // The nth mesh part gets the nth material
    if (!dracoMesh.isEmpty()) {
        std::vector<hifi::ByteArray> newMaterialList;
        newMaterialList.reserve(_materialIDs.size());
        for (auto materialID : _materialIDs) {
            newMaterialList.push_back(hifi::ByteArray(std::to_string((int)materialID).c_str()));
        }
        FBXNode dracoNode;
        buildDracoMeshNode(dracoNode, dracoMesh, newMaterialList);
        geometryNode.children.append(dracoNode);
    } else {
        handleWarning("Baked mesh for OBJ model '" + _modelURL.toString() + "' is empty");
    }

    // Generating Connections node
    connectionsNode.name = CONNECTIONS_NODE_NAME;

    // connect Geometry to Model
    {
        FBXNode cNode;
        cNode.name = C_NODE_NAME;
        cNode.properties = { CONNECTIONS_NODE_PROPERTY, geometryID, modelID };
        connectionsNode.children.push_back(cNode);
    }

    // connect all materials to model
    for (auto& materialID : _materialIDs) {
        FBXNode cNode;
        cNode.name = C_NODE_NAME;
        cNode.properties = { CONNECTIONS_NODE_PROPERTY, materialID, modelID };
        connectionsNode.children.append(cNode);
    }
}

// Set properties for material nodes
void OBJBaker::setMaterialNodeProperties(FBXNode& materialNode, const QString& materialName, const hfm::Material& material, const hfm::Model::Pointer& hfmModel) {
    auto materialID = nextNodeID();
    _materialIDs.push_back(materialID);
    materialNode.properties = { materialID, materialName, MESH };

    // Setting the hierarchy: Material -> Properties70 -> P -> Properties
    FBXNode properties70Node;
    properties70Node.name = PROPERTIES70_NODE_NAME;

    // Set diffuseColor
    FBXNode pNodeDiffuseColor;
    {
        pNodeDiffuseColor.name = P_NODE_NAME;
        pNodeDiffuseColor.properties.append({
            "DiffuseColor", "Color", "", "A",
            material.diffuseColor[0], material.diffuseColor[1], material.diffuseColor[2]
        });
    }
    properties70Node.children.append(pNodeDiffuseColor);

    // Set specularColor
    FBXNode pNodeSpecularColor;
    {
        pNodeSpecularColor.name = P_NODE_NAME;
        pNodeSpecularColor.properties.append({
            "SpecularColor", "Color", "", "A",
            material.specularColor[0], material.specularColor[1], material.specularColor[2]
        });
    }
    properties70Node.children.append(pNodeSpecularColor);

    // Set Shininess
    FBXNode pNodeShininess;
    {
        pNodeShininess.name = P_NODE_NAME;
        pNodeShininess.properties.append({
            "Shininess", "Number", "", "A",
            material.shininess
        });
    }
    properties70Node.children.append(pNodeShininess);

    // Set Opacity
    FBXNode pNodeOpacity;
    {
        pNodeOpacity.name = P_NODE_NAME;
        pNodeOpacity.properties.append({
            "Opacity", "Number", "", "A",
            material.opacity
        });
    }
    properties70Node.children.append(pNodeOpacity);

    materialNode.children.append(properties70Node);
}
