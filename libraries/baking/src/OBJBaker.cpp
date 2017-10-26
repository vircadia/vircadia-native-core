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

#include <PathUtils.h>
#include <NetworkAccessManager.h>

#include "OBJBaker.h"
#include "OBJReader.h"
#include "FBXWriter.h"

const double UNIT_SCALE_FACTOR = 100;
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

OBJBaker::OBJBaker(const QUrl& objURL, TextureBakerThreadGetter textureThreadGetter,
                   const QString& bakedOutputDir, const QString& originalOutputDir) :
    ModelBaker(objURL, textureThreadGetter, bakedOutputDir, originalOutputDir)
{

}

OBJBaker::~OBJBaker() {
    if (_tempDir.exists()) {
        if (!_tempDir.remove(_originalOBJFilePath)) {
            qCWarning(model_baking) << "Failed to remove temporary copy of OBJ file:" << _originalOBJFilePath;
        }
        if (!_tempDir.rmdir(".")) {
            qCWarning(model_baking) << "Failed to remove temporary directory:" << _tempDir;
        }
    }
}

void OBJBaker::bake() {
    qDebug() << "OBJBaker" << modelURL << "bake starting";

    auto tempDir = PathUtils::generateTemporaryDir();

    if (tempDir.isEmpty()) {
        handleError("Failed to create a temporary directory.");
        return;
    }

    _tempDir = tempDir;

    _originalOBJFilePath = _tempDir.filePath(modelURL.fileName());
    qDebug() << "Made temporary dir " << _tempDir;
    qDebug() << "Origin file path: " << _originalOBJFilePath;
    
    // trigger bakeOBJ once OBJ is loaded
    connect(this, &OBJBaker::OBJLoaded, this, &OBJBaker::bakeOBJ);

    // make a local copy of the OBJ
    loadOBJ();
}

void OBJBaker::loadOBJ() {
    // check if the OBJ is local or it needs to be downloaded
    if (modelURL.isLocalFile()) {
        // loading the local OBJ
        QFile localOBJ{ modelURL.toLocalFile() };

        qDebug() << "Local file url: " << modelURL << modelURL.toString() << modelURL.toLocalFile() << ", copying to: " << _originalOBJFilePath;

        if (!localOBJ.exists()) {
            handleError("Could not find " + modelURL.toString());
            return;
        }

        // make a copy in the output folder
        if (!originalOutputDir.isEmpty()) {
            qDebug() << "Copying to: " << originalOutputDir << "/" << modelURL.fileName();
            localOBJ.copy(originalOutputDir + "/" + modelURL.fileName());
        }
                
        localOBJ.copy(_originalOBJFilePath);

        // local OBJ is loaded emit signal to trigger its baking
        emit OBJLoaded();
    } else {
        // OBJ is remote, start download 
        auto& networkAccessManager = NetworkAccessManager::getInstance();

        QNetworkRequest networkRequest;

        // setup the request to follow re-directs and always hit the network
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
        networkRequest.setUrl(modelURL);
        
        qCDebug(model_baking) << "Downloading" << modelURL;
        auto networkReply = networkAccessManager.get(networkRequest);
        
        connect(networkReply, &QNetworkReply::finished, this, &OBJBaker::handleOBJNetworkReply);
    }
}

void OBJBaker::handleOBJNetworkReply() {
    auto requestReply = qobject_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        qCDebug(model_baking) << "Downloaded" << modelURL;

        // grab the contents of the reply and make a copy in the output folder
        QFile copyOfOriginal(_originalOBJFilePath);

        qDebug(model_baking) << "Writing copy of original obj to" << _originalOBJFilePath << copyOfOriginal.fileName();

        if (!copyOfOriginal.open(QIODevice::WriteOnly)) {
            // add an error to the error list for this obj stating that a duplicate of the original obj could not be made
            handleError("Could not create copy of " + modelURL.toString() + " (Failed to open " + _originalOBJFilePath + ")");
            return;
        }
        if (copyOfOriginal.write(requestReply->readAll()) == -1) {
            handleError("Could not create copy of " + modelURL.toString() + " (Failed to write)");
            return;
        }

        // close that file now that we are done writing to it
        copyOfOriginal.close();

        if (!originalOutputDir.isEmpty()) {
            copyOfOriginal.copy(originalOutputDir + "/" + modelURL.fileName());
        }

        // remote OBJ is loaded emit signal to trigger its baking
        emit OBJLoaded();
    } else {
        // add an error to our list stating that the OBJ could not be downloaded
        handleError("Failed to download " + modelURL.toString());
    }
}


void OBJBaker::bakeOBJ() {
    // Read the OBJ file
    QFile objFile(_originalOBJFilePath);
    if (!objFile.open(QIODevice::ReadOnly)) {
        handleError("Error opening " + _originalOBJFilePath + " for reading");
        return;
    }

    QByteArray objData = objFile.readAll();

    bool combineParts = true; // set true so that OBJReader reads material info from material library
    OBJReader reader;
    FBXGeometry* geometry = reader.readOBJ(objData, QVariantHash(), combineParts, modelURL);
   
    // Write OBJ Data as FBX tree nodes
    FBXNode rootNode;
    createFBXNodeTree(rootNode, *geometry);

    // Serialize the resultant FBX tree
    auto encodedFBX = FBXWriter::encodeFBX(rootNode);

    // Export as baked FBX
    auto fileName = modelURL.fileName();
    auto baseName = fileName.left(fileName.lastIndexOf('.'));
    auto bakedFilename = baseName + ".baked.fbx";

    _bakedOBJFilePath = bakedOutputDir + "/" + bakedFilename;

    QFile bakedFile;
    bakedFile.setFileName(_bakedOBJFilePath);
    if (!bakedFile.open(QIODevice::WriteOnly)) {
        handleError("Error opening " + _bakedOBJFilePath + " for writing");
        return;
    }

    bakedFile.write(encodedFBX);

    // Export successful
    _outputFiles.push_back(_bakedOBJFilePath);
    qCDebug(model_baking) << "Exported" << modelURL << "to" << _bakedOBJFilePath;
    
    // Export done emit finished
    emit finished();
}

void OBJBaker::createFBXNodeTree(FBXNode& rootNode, FBXGeometry& geometry) {
    // Generating FBX Header Node
    FBXNode headerNode;
    headerNode.name = FBX_HEADER_EXTENSION;

    // Generating global settings node
    // Required for Unit Scale Factor
    FBXNode globalSettingsNode;
    globalSettingsNode.name = GLOBAL_SETTINGS_NODE_NAME;
    
    // Setting the tree hierarchy: GlobalSettings -> Properties70 -> P -> Properties
    FBXNode properties70Node;
    properties70Node.name = PROPERTIES70_NODE_NAME;
    FBXNode pNode;
    pNode.name = P_NODE_NAME;
    setNodeProperties(pNode);
    properties70Node.children = { pNode };
    globalSettingsNode.children = { properties70Node };

    // Generating Object node
    _objectNode.name = OBJECTS_NODE_NAME;

    // Generating Object node's child - Geometry node 
    FBXNode geometryNode;
    geometryNode.name = GEOMETRY_NODE_NAME;
    setNodeProperties(geometryNode);
    
    // Compress the mesh information and store in dracoNode
    bool hasDeformers = false; // No concept of deformers for an OBJ
    FBXNode dracoNode; 
    this->compressMesh(geometry.meshes[0], hasDeformers, dracoNode);
    geometryNode.children.append(dracoNode);

    // Generating Object node's child - Model node
    FBXNode modelNode;
    modelNode.name = MODEL_NODE_NAME;
    setNodeProperties(modelNode);
    
    _objectNode.children = { geometryNode, modelNode };

    // Generating Objects node's child - Material node
    auto meshParts = geometry.meshes[0].parts;
    for (auto meshPart : meshParts) {
        FBXNode materialNode;
        materialNode.name = MATERIAL_NODE_NAME;
        if (geometry.materials.size() == 1) {
            // case when no material information is provided, OBJReader considers it as a single default material
            foreach(QString materialID, geometry.materials.keys()) {
                setMaterialNodeProperties(materialNode, materialID, geometry);
            }
        } else {
            setMaterialNodeProperties(materialNode, meshPart.materialID, geometry);
        }
        
        _objectNode.children.append(materialNode);
    }
    
    // Generating Texture Node
    int count = 0;
    // iterate through mesh parts and process the associated textures
    for (int i = 0;i < meshParts.size();i++) {
        QString material = meshParts[i].materialID;
        FBXMaterial currentMaterial = geometry.materials[material];
        if (!currentMaterial.albedoTexture.filename.isEmpty() || !currentMaterial.specularTexture.filename.isEmpty()) {
            _textureID = _nodeID;
            _mapTextureMaterial.push_back(QPair<qlonglong, int>(_textureID, i));
            
            FBXNode textureNode;
            textureNode.name = TEXTURE_NODE_NAME;
            QVariant textureProperty(_nodeID++);
            textureNode.properties = { textureProperty };
            
            // Texture node child - TextureName node
            FBXNode textureNameNode;
            textureNameNode.name = TEXTURENAME_NODE_NAME;
            QByteArray propertyString = (!currentMaterial.albedoTexture.filename.isEmpty()) ? "Kd" : "Ka";
            textureProperty = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
            textureNameNode.properties = { textureProperty };

            // Texture node child - Relative Filename node
            FBXNode relativeFilenameNode;
            relativeFilenameNode.name = RELATIVEFILENAME_NODE_NAME;
            
            QByteArray textureFileName = (!currentMaterial.albedoTexture.filename.isEmpty()) ? currentMaterial.albedoTexture.filename : currentMaterial.specularTexture.filename;
            
            // Callback to get Texture content and type
            getTextureTypeCallback textureContentTypeCallback = [=]() {
                return (!currentMaterial.albedoTexture.filename.isEmpty()) ? image::TextureUsage::Type::ALBEDO_TEXTURE : image::TextureUsage::Type::SPECULAR_TEXTURE;
            };
            
            // Compress the texture using ModelBaker::compressTexture() and store compressed file's name in the node
            QByteArray* textureFile = this->compressTexture(textureFileName, textureContentTypeCallback);
            if (textureFile) {
                textureProperty = QVariant::fromValue(QByteArray(textureFile->data(), (int)textureFile->size()));
            } else {
                // Baking failed return
                return;
            }
            relativeFilenameNode.properties = { textureProperty };
            
            textureNode.children = { textureNameNode, relativeFilenameNode };

            _objectNode.children.append(textureNode);
        }
    }
    
    // Generating Connections node
    FBXNode connectionsNode;
    connectionsNode.name = CONNECTIONS_NODE_NAME;
    
    // connect Geometry to Model 
    FBXNode cNode;
    cNode.name = C_NODE_NAME;
    QByteArray propertyString(CONNECTIONS_NODE_PROPERTY);
    QVariant property0 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
    qlonglong childID = _geometryID;
    QVariant property1(childID);
    qlonglong parentID = _modelID;
    QVariant property2(parentID);
    cNode.properties = { property0, property1, property2 };
    connectionsNode.children = { cNode };

    // connect all materials to model
    for (int i = 0;i < geometry.materials.size();i++) {
        FBXNode cNode1;
        cNode1.name = C_NODE_NAME;
        property0 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
        property1 = _materialIDs[i];
        property2 = _modelID;
        cNode1.properties = { property0, property1, property2 };
        connectionsNode.children.append(cNode1);
    }
    
    // Connect textures to materials
    for (int i = 0;i < _mapTextureMaterial.size();i++) {
        FBXNode cNode2;
        cNode2.name = C_NODE_NAME;
        propertyString = CONNECTIONS_NODE_PROPERTY_1;
        property0 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
        property1 = _mapTextureMaterial[i].first;
        int matID = _mapTextureMaterial[i].second;
        property2 = _materialIDs[matID];
        propertyString = "AmbientFactor";
        QVariant property3 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
        cNode2.properties = { property0, property1, property2, property3 };
        connectionsNode.children.append(cNode2);

        FBXNode cNode3;
        cNode3.name = C_NODE_NAME;
        propertyString = CONNECTIONS_NODE_PROPERTY_1;
        property0 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
        property1 = _mapTextureMaterial[i].first;
        property2 = _materialIDs[matID];
        propertyString = "DiffuseColor";
        property3 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
        cNode3.properties = { property0, property1, property2, property3 };
        connectionsNode.children.append(cNode3);
    }
    
    // Make all generated nodes children of rootNode
    rootNode.children = { globalSettingsNode, _objectNode, connectionsNode };
}

// Set properties for P Node and Sub-Object nodes
void OBJBaker::setNodeProperties(FBXNode& parentNode) {
    if (parentNode.name == P_NODE_NAME) {
        std::vector<QByteArray> stringProperties{ "UnitScaleFactor", "double", "Number", "" };
        std::vector<double> numericProperties{ UNIT_SCALE_FACTOR };

        setPropertiesList(stringProperties, numericProperties, parentNode.properties);
    } else if (parentNode.name == GEOMETRY_NODE_NAME) {
        _geometryID = _nodeID;
        QVariant property0(_nodeID++);
        QByteArray propertyString(GEOMETRY_NODE_NAME);
        QVariant property1 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
        propertyString = MESH;
        QVariant property2 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));

        parentNode.properties = { property0, property1, property2 };
    } else if (parentNode.name == MODEL_NODE_NAME) {
        _modelID = _nodeID;
        QVariant property0(_nodeID++);
        QByteArray propertyString(MODEL_NODE_NAME);
        QVariant property1 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
        propertyString = MESH;
        QVariant property2 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));

        parentNode.properties = { property0, property1, property2 };
    }
}

// Set properties for material nodes
void OBJBaker::setMaterialNodeProperties(FBXNode& materialNode, QString material, FBXGeometry& geometry) {
    _materialIDs.push_back(_nodeID);
    QVariant property0(_nodeID++);
    QByteArray propertyString(material.toLatin1());
    QVariant property1 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));
    propertyString = MESH;
    QVariant property2 = QVariant::fromValue(QByteArray(propertyString.data(), (int)propertyString.size()));

    materialNode.properties = { property0, property1, property2 };
    
    FBXMaterial currentMaterial = geometry.materials[material];
    
    // Setting the hierarchy: Material -> Properties70 -> P -> Properties
    FBXNode properties70Node;
    properties70Node.name = PROPERTIES70_NODE_NAME;

    // Set diffuseColor
    FBXNode pNodeDiffuseColor;
    pNodeDiffuseColor.name = P_NODE_NAME;

    std::vector<QByteArray> stringProperties{ "DiffuseColor", "Color", "", "A" };
    std::vector<double> numericProperties{ currentMaterial.diffuseColor[0], currentMaterial.diffuseColor[1], currentMaterial.diffuseColor[2] };
    setPropertiesList(stringProperties, numericProperties, pNodeDiffuseColor.properties);

    properties70Node.children.append(pNodeDiffuseColor);

    // Set specularColor
    FBXNode pNodeSpecularColor;
    pNodeSpecularColor.name = P_NODE_NAME;

    stringProperties = { "SpecularColor", "Color", "", "A" };
    numericProperties = { currentMaterial.specularColor[0], currentMaterial.specularColor[1], currentMaterial.specularColor[2] };
    setPropertiesList(stringProperties, numericProperties, pNodeSpecularColor.properties);

    properties70Node.children.append(pNodeSpecularColor);

    // Set Shininess
    FBXNode pNodeShininess;
    pNodeShininess.name = P_NODE_NAME;

    stringProperties = { "Shininess", "Number", "", "A" };
    numericProperties = { currentMaterial.shininess };
    setPropertiesList(stringProperties, numericProperties, pNodeShininess.properties);
    
    properties70Node.children.append(pNodeShininess);

    // Set Opacity
    FBXNode pNodeOpacity;
    pNodeOpacity.name = P_NODE_NAME;

    stringProperties = { "Opacity", "Number", "", "A" };
    numericProperties = { currentMaterial.opacity };
    setPropertiesList(stringProperties, numericProperties, pNodeOpacity.properties);

    properties70Node.children.append(pNodeOpacity);

    materialNode.children.append(properties70Node);
}

// Bundle various String and numerical type properties into a single QVariantList
template<typename N>
void OBJBaker::setPropertiesList(std::vector<QByteArray>& stringProperties, std::vector<N>& numericProperties, QVariantList& propertiesList) {
    foreach(auto stringProperty, stringProperties) {
        auto propertyValue = QVariant::fromValue(QByteArray(stringProperty.data(), (int)stringProperty.size()));
        propertiesList.append(propertyValue);
    }

    foreach(auto numericProperty, numericProperties) {
        QVariant propertyValue(numericProperty);
        propertiesList.append(propertyValue);
    }
}
