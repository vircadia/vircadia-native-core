//
//  FBXSerializer.cpp
//  libraries/model-serializers/src
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FBXSerializer.h"

#include <QBuffer>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <BlendshapeConstants.h>

#include <hfm/ModelFormatLogging.h>

// TOOL: Uncomment the following line to enable the filtering of all the unkwnon fields of a node so we can break point easily while loading a model with problems...
//#define DEBUG_FBXSERIALIZER

using namespace std;

glm::vec3 parseVec3(const QString& string) {
    QStringList elements = string.split(',');
    if (elements.isEmpty()) {
        return glm::vec3();
    }
    glm::vec3 value;
    for (int i = 0; i < 3; i++) {
        // duplicate last value if there aren't three elements
        value[i] = elements.at(min(i, elements.size() - 1)).trimmed().toFloat();
    }
    return value;
}

enum RotationOrder {
    OrderXYZ = 0,
    OrderXZY,
    OrderYZX,
    OrderYXZ,
    OrderZXY,
    OrderZYX,
    OrderSphericXYZ
};

bool haveReportedUnhandledRotationOrder = false; // Report error only once per FBX file.

glm::vec3 convertRotationToXYZ(int rotationOrder, const glm::vec3& rotation) {
    // Convert rotation with given rotation order to have order XYZ.
    if (rotationOrder == OrderXYZ) {
        return rotation;
    }

    glm::quat xyzRotation;

    switch (rotationOrder) {
        case OrderXZY:
            xyzRotation = glm::quat(glm::radians(glm::vec3(0, rotation.y, 0)))
                * (glm::quat(glm::radians(glm::vec3(0, 0, rotation.z))) * glm::quat(glm::radians(glm::vec3(rotation.x, 0, 0))));
            break;
        case OrderYZX:
            xyzRotation = glm::quat(glm::radians(glm::vec3(rotation.x, 0, 0)))
                * (glm::quat(glm::radians(glm::vec3(0, 0, rotation.z))) * glm::quat(glm::radians(glm::vec3(0, rotation.y, 0))));
            break;
        case OrderYXZ:
            xyzRotation = glm::quat(glm::radians(glm::vec3(0, 0, rotation.z)))
                * (glm::quat(glm::radians(glm::vec3(rotation.x, 0, 0))) * glm::quat(glm::radians(glm::vec3(0, rotation.y, 0))));
            break;
        case OrderZXY:
            xyzRotation = glm::quat(glm::radians(glm::vec3(0, rotation.y, 0)))
                * (glm::quat(glm::radians(glm::vec3(rotation.x, 0, 0))) * glm::quat(glm::radians(glm::vec3(0, 0, rotation.z))));
            break;
        case OrderZYX:
            xyzRotation = glm::quat(glm::radians(glm::vec3(rotation.x, 0, 0)))
                * (glm::quat(glm::radians(glm::vec3(0, rotation.y, 0))) * glm::quat(glm::radians(glm::vec3(0, 0, rotation.z))));
            break;
        default:
            // FIXME: Handle OrderSphericXYZ.
            if (!haveReportedUnhandledRotationOrder) {
                qCDebug(modelformat) << "ERROR: Unhandled rotation order in FBX file:" << rotationOrder;
                haveReportedUnhandledRotationOrder = true;
            }
            return rotation;
    }

    return glm::degrees(safeEulerAngles(xyzRotation));
}

QString processID(const QString& id) {
    // Blender (at least) prepends a type to the ID, so strip it out
    return id.mid(id.lastIndexOf(':') + 1);
}

QString getModelName(const QVariantList& properties) {
    QString name;
    if (properties.size() == 3) {
        name = properties.at(1).toString();
        name = processID(name.left(name.indexOf(QChar('\0'))));
    } else {
        name = processID(properties.at(0).toString());
    }
    return name;
}

QString getMaterialName(const QVariantList& properties) {
    QString name;
    if (properties.size() == 1 || properties.at(1).toString().isEmpty()) {
        name = properties.at(0).toString();
        name = processID(name.left(name.indexOf(QChar('\0'))));
    } else {
        name = processID(properties.at(1).toString());
    }
    return name;
}

QString getID(const QVariantList& properties, int index = 0) {
    return processID(properties.at(index).toString());
}

class FBXModel {
public:
    QString name;

    int parentIndex;
    glm::vec3 translation;
    glm::mat4 preTransform;
    glm::quat preRotation;
    glm::quat rotation;
    glm::quat postRotation;
    glm::mat4 postTransform;

    glm::vec3 rotationMin;  // radians
    glm::vec3 rotationMax;  // radians

    bool hasGeometricOffset;
    glm::vec3 geometricTranslation;
    glm::quat geometricRotation;
    glm::vec3 geometricScaling;
    bool isLimbNode;  // is this FBXModel transform is a "LimbNode" i.e. a joint
};

glm::mat4 getGlobalTransform(const QMultiMap<QString, QString>& _connectionParentMap,
        const QHash<QString, FBXModel>& fbxModels, QString nodeID, bool mixamoHack, const QString& url) {
    glm::mat4 globalTransform;
    QVector<QString> visitedNodes; // Used to prevent following a cycle
    while (!nodeID.isNull()) {
        visitedNodes.append(nodeID); // Append each node we visit

        const FBXModel& fbxModel = fbxModels.value(nodeID);
        globalTransform = glm::translate(fbxModel.translation) * fbxModel.preTransform * glm::mat4_cast(fbxModel.preRotation *
            fbxModel.rotation * fbxModel.postRotation) * fbxModel.postTransform * globalTransform;
        if (fbxModel.hasGeometricOffset) {
            glm::mat4 geometricOffset = createMatFromScaleQuatAndPos(fbxModel.geometricScaling, fbxModel.geometricRotation, fbxModel.geometricTranslation);
            globalTransform = globalTransform * geometricOffset;
        }

        if (mixamoHack) {
            // there's something weird about the models from Mixamo Fuse; they don't skin right with the full transform
            return globalTransform;
        }
        QList<QString> parentIDs = _connectionParentMap.values(nodeID);
        nodeID = QString();
        foreach (const QString& parentID, parentIDs) {
            if (visitedNodes.contains(parentID)) {
                qCWarning(modelformat) << "Ignoring loop detected in FBX connection map for" << url;
                continue;
            }

            if (fbxModels.contains(parentID)) {
                nodeID = parentID;
                break;
            }
        }
    }
    return globalTransform;
}

class ExtractedBlendshape {
public:
    QString id;
    HFMBlendshape blendshape;
};

void printNode(const FBXNode& node, int indentLevel) {
    int indentLength = 2;
    hifi::ByteArray spaces(indentLevel * indentLength, ' ');
    QDebug nodeDebug = qDebug(modelformat);

    nodeDebug.nospace() << spaces.data() << node.name.data() << ": ";
    foreach (const QVariant& property, node.properties) {
        nodeDebug << property;
    }

    foreach (const FBXNode& child, node.children) {
        printNode(child, indentLevel + 1);
    }
}

class Cluster {
public:
    QVector<int> indices;
    QVector<double> weights;
    glm::mat4 transformLink;
};

void appendModelIDs(const QString& parentID, const QMultiMap<QString, QString>& connectionChildMap,
        QHash<QString, FBXModel>& fbxModels, QSet<QString>& remainingModels, QVector<QString>& modelIDs, bool isRootNode = false) {
    if (remainingModels.contains(parentID)) {
        modelIDs.append(parentID);
        remainingModels.remove(parentID);
    }
    int parentIndex = isRootNode ? -1 : modelIDs.size() - 1;
    foreach (const QString& childID, connectionChildMap.values(parentID)) {
        if (remainingModels.contains(childID)) {
            FBXModel& fbxModel = fbxModels[childID];
            if (fbxModel.parentIndex == -1) {
                fbxModel.parentIndex = parentIndex;
                appendModelIDs(childID, connectionChildMap, fbxModels, remainingModels, modelIDs);
            }
        }
    }
}

HFMBlendshape extractBlendshape(const FBXNode& object) {
    HFMBlendshape blendshape;
    foreach (const FBXNode& data, object.children) {
        if (data.name == "Indexes") {
            blendshape.indices = FBXSerializer::getIntVector(data);

        } else if (data.name == "Vertices") {
            blendshape.vertices = FBXSerializer::createVec3Vector(FBXSerializer::getDoubleVector(data));

        } else if (data.name == "Normals") {
            blendshape.normals = FBXSerializer::createVec3Vector(FBXSerializer::getDoubleVector(data));
        }
    }
    return blendshape;
}

QVector<int> getIndices(const QVector<QString> ids, QVector<QString> modelIDs) {
    QVector<int> indices;
    foreach (const QString& id, ids) {
        int index = modelIDs.indexOf(id);
        if (index != -1) {
            indices.append(index);
        }
    }
    return indices;
}

typedef QPair<int, float> WeightedIndex;

void addBlendshapes(const ExtractedBlendshape& extracted, const QList<WeightedIndex>& indices, ExtractedMesh& extractedMesh) {
    foreach (const WeightedIndex& index, indices) {
        extractedMesh.mesh.blendshapes.resize(max(extractedMesh.mesh.blendshapes.size(), index.first + 1));
        extractedMesh.blendshapeIndexMaps.resize(extractedMesh.mesh.blendshapes.size());
        HFMBlendshape& blendshape = extractedMesh.mesh.blendshapes[index.first];
        QHash<int, int>& blendshapeIndexMap = extractedMesh.blendshapeIndexMaps[index.first];
        for (int i = 0; i < extracted.blendshape.indices.size(); i++) {
            int oldIndex = extracted.blendshape.indices.at(i);
            for (QMultiHash<int, int>::const_iterator it = extractedMesh.newIndices.constFind(oldIndex);
                    it != extractedMesh.newIndices.constEnd() && it.key() == oldIndex; it++) {
                QHash<int, int>::iterator blendshapeIndex = blendshapeIndexMap.find(it.value());
                if (blendshapeIndex == blendshapeIndexMap.end()) {
                    blendshapeIndexMap.insert(it.value(), blendshape.indices.size());
                    blendshape.indices.append(it.value());
                    blendshape.vertices.append(extracted.blendshape.vertices.at(i) * index.second);
                    blendshape.normals.append(extracted.blendshape.normals.at(i) * index.second);
                } else {
                    blendshape.vertices[*blendshapeIndex] += extracted.blendshape.vertices.at(i) * index.second;
                    blendshape.normals[*blendshapeIndex] += extracted.blendshape.normals.at(i) * index.second;
                }
            }
        }
    }
}

QString getTopModelID(const QMultiMap<QString, QString>& connectionParentMap,
        const QHash<QString, FBXModel>& fbxModels, const QString& modelID, const QString& url) {
    QString topID = modelID;
    QVector<QString> visitedNodes; // Used to prevent following a cycle
    forever {
        visitedNodes.append(topID); // Append each node we visit

        foreach (const QString& parentID, connectionParentMap.values(topID)) {
            if (visitedNodes.contains(parentID)) {
                qCWarning(modelformat) << "Ignoring loop detected in FBX connection map for" << url;
                continue;
            }

            if (fbxModels.contains(parentID)) {
                topID = parentID;
                goto outerContinue;
            }
        }
        return topID;

        outerContinue: ;
    }
}

QString getString(const QVariant& value) {
    // if it's a list, return the first entry
    QVariantList list = value.toList();
    return list.isEmpty() ? value.toString() : list.at(0).toString();
}

class AnimationCurve {
public:
    QVector<float> values;
};

bool checkMaterialsHaveTextures(const QHash<QString, HFMMaterial>& materials,
        const QHash<QString, hifi::ByteArray>& textureFilenames, const QMultiMap<QString, QString>& _connectionChildMap) {
    foreach (const QString& materialID, materials.keys()) {
        foreach (const QString& childID, _connectionChildMap.values(materialID)) {
            if (textureFilenames.contains(childID)) {
                return true;
            }
        }
    }
    return false;
}

int matchTextureUVSetToAttributeChannel(const QString& texUVSetName, const QHash<QString, int>& texcoordChannels) {
    if (texUVSetName.isEmpty()) {
        return 0;
    } else {
        QHash<QString, int>::const_iterator tcUnit = texcoordChannels.find(texUVSetName);
        if (tcUnit != texcoordChannels.end()) {
            int channel = (*tcUnit);
            if (channel >= 2) {
                channel = 0;
            }
            return channel;
        } else {
            return 0;
        }
    }
}


HFMLight extractLight(const FBXNode& object) {
    HFMLight light;
    foreach (const FBXNode& subobject, object.children) {
        QString childname = QString(subobject.name);
        if (subobject.name == "Properties70") {
            foreach (const FBXNode& property, subobject.children) {
                int valIndex = 4;
                QString propName = QString(property.name);
                if (property.name == "P") {
                    QString propname = property.properties.at(0).toString();
                    if (propname == "Intensity") {
                        light.intensity = 0.01f * property.properties.at(valIndex).value<float>();
                    } else if (propname == "Color") {
                        light.color = FBXSerializer::getVec3(property.properties, valIndex);
                    }
                }
            }
        } else if ( subobject.name == "GeometryVersion"
                   || subobject.name == "TypeFlags") {
        }
    }
#if defined(DEBUG_FBXSERIALIZER)

    QString type = object.properties.at(0).toString();
    type = object.properties.at(1).toString();
    type = object.properties.at(2).toString();

    foreach (const QVariant& prop, object.properties) {
        QString proptype = prop.typeName();
        QString propval = prop.toString();
        if (proptype == "Properties70") {
        }
    }
#endif

    return light;
}

hifi::ByteArray fileOnUrl(const hifi::ByteArray& filepath, const QString& url) {
    // in order to match the behaviour when loading models from remote URLs
    // we assume that all external textures are right beside the loaded model
    // ignoring any relative paths or absolute paths inside of models

    return filepath.mid(filepath.lastIndexOf('/') + 1);
}

HFMModel* FBXSerializer::extractHFMModel(const hifi::VariantHash& mapping, const QString& url) {
    const FBXNode& node = _rootNode;
    bool deduplicateIndices = mapping["deduplicateIndices"].toBool();

    QMap<QString, ExtractedMesh> meshes;
    QHash<QString, QString> modelIDsToNames;
    QHash<QString, int> meshIDsToMeshIndices;
    QHash<QString, QString> ooChildToParent;

    QVector<ExtractedBlendshape> blendshapes;

    QHash<QString, FBXModel> fbxModels;
    QHash<QString, Cluster> clusters;
    QHash<QString, AnimationCurve> animationCurves;

    QHash<QString, QString> typeFlags;

    QHash<QString, QString> localRotations;
    QHash<QString, QString> localTranslations;
    QHash<QString, QString> xComponents;
    QHash<QString, QString> yComponents;
    QHash<QString, QString> zComponents;

    std::map<QString, HFMLight> lights;

    hifi::VariantMultiHash blendshapeMappings = mapping.value("bs").toHash();

    QMultiHash<hifi::ByteArray, WeightedIndex> blendshapeIndices;
    for (int i = 0;; i++) {
        hifi::ByteArray blendshapeName = BLENDSHAPE_NAMES[i];
        if (blendshapeName.isEmpty()) {
            break;
        }
        QList<QVariant> mappings = blendshapeMappings.values(blendshapeName);
        if (mappings.isEmpty()) {
            blendshapeIndices.insert(blendshapeName, WeightedIndex(i, 1.0f));
        } else {
            foreach (const QVariant& mapping, mappings) {
                QVariantList blendshapeMapping = mapping.toList();
                blendshapeIndices.insert(blendshapeMapping.at(0).toByteArray(),
                   WeightedIndex(i, blendshapeMapping.at(1).toFloat()));
            }
        }
    }
    QMultiHash<QString, WeightedIndex> blendshapeChannelIndices;
#if defined(DEBUG_FBXSERIALIZER)
    int unknown = 0;
#endif
    HFMModel* hfmModelPtr = new HFMModel;
    HFMModel& hfmModel = *hfmModelPtr;

    hfmModel.originalURL = url;

    float unitScaleFactor = 1.0f;
    glm::quat upAxisZRotation;
    bool applyUpAxisZRotation = false;
    glm::vec3 ambientColor;
    QString hifiGlobalNodeID;
    unsigned int meshIndex = 0;
    haveReportedUnhandledRotationOrder = false;
    int fbxVersionNumber = -1;
    foreach (const FBXNode& child, node.children) {

        if (child.name == "FBXHeaderExtension") {
            foreach (const FBXNode& object, child.children) {
                if (object.name == "SceneInfo") {
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "MetaData") {
                            foreach (const FBXNode& subsubobject, subobject.children) {
                                if (subsubobject.name == "Author") {
                                    hfmModel.author = subsubobject.properties.at(0).toString();
                                }
                            }
                        } else if (subobject.name == "Properties70") {
                            foreach (const FBXNode& subsubobject, subobject.children) {
                                static const QVariant APPLICATION_NAME = QVariant(hifi::ByteArray("Original|ApplicationName"));
                                if (subsubobject.name == "P" && subsubobject.properties.size() >= 5 &&
                                        subsubobject.properties.at(0) == APPLICATION_NAME) {
                                    hfmModel.applicationName = subsubobject.properties.at(4).toString();
                                }
                            }
                        }
                    }
                } else if (object.name == "FBXVersion") {
                    fbxVersionNumber = object.properties.at(0).toInt();
                }
            }
        } else if (child.name == "GlobalSettings") {
            foreach (const FBXNode& object, child.children) {
                if (object.name == "Properties70") {
                    QString propertyName = "P";
                    int index = 4;
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == propertyName) {
                            static const QVariant UNIT_SCALE_FACTOR = hifi::ByteArray("UnitScaleFactor");
                            static const QVariant AMBIENT_COLOR = hifi::ByteArray("AmbientColor");
                            static const QVariant UP_AXIS = hifi::ByteArray("UpAxis");
                            const auto& subpropName = subobject.properties.at(0);
                            if (subpropName == UNIT_SCALE_FACTOR) {
                                unitScaleFactor = subobject.properties.at(index).toFloat();
                            } else if (subpropName == AMBIENT_COLOR) {
                                ambientColor = getVec3(subobject.properties, index);
                            } else if (subpropName == UP_AXIS) {
                                constexpr int UP_AXIS_Y = 1;
                                constexpr int UP_AXIS_Z = 2;
                                int upAxis = subobject.properties.at(index).toInt();
                                if (upAxis == UP_AXIS_Y) {
                                    // No update necessary, y up is the default
                                } else if (upAxis == UP_AXIS_Z) {
                                    upAxisZRotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                                    applyUpAxisZRotation = true;
                                }
                            }
                        }
                    }
                }
            }
        } else if (child.name == "Objects") {
            foreach (const FBXNode& object, child.children) {
                if (object.name == "Geometry") {
                    if (object.properties.at(2) == "Mesh") {
                        meshes.insert(getID(object.properties), extractMesh(object, meshIndex, deduplicateIndices));
                    } else { // object.properties.at(2) == "Shape"
                        ExtractedBlendshape extracted = { getID(object.properties), extractBlendshape(object) };
                        blendshapes.append(extracted);
                    }
                } else if (object.name == "Model") {
                    QString name = getModelName(object.properties);
                    QString id = getID(object.properties);
                    modelIDsToNames.insert(id, name);

                    QString modelname = name.toLower();
                    if (modelname.startsWith("hifi")) {
                        hifiGlobalNodeID = id;
                    }

                    glm::vec3 translation;
                    // NOTE: the euler angles as supplied by the FBX file are in degrees
                    glm::vec3 rotationOffset;
                    int rotationOrder = OrderXYZ;  // Default rotation order set in "Definitions" node is assumed to be XYZ.
                    glm::vec3 preRotation, rotation, postRotation;
                    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
                    glm::vec3 scalePivot, rotationPivot, scaleOffset;
                    bool rotationMinX = false, rotationMinY = false, rotationMinZ = false;
                    bool rotationMaxX = false, rotationMaxY = false, rotationMaxZ = false;

                    // local offset transforms from 3ds max
                    bool hasGeometricOffset = false;
                    glm::vec3 geometricTranslation;
                    glm::vec3 geometricScaling(1.0f, 1.0f, 1.0f);
                    glm::vec3 geometricRotation;

                    glm::vec3 rotationMin, rotationMax;

                    bool isLimbNode = object.properties.size() >= 3 && object.properties.at(2) == "LimbNode";
                    FBXModel fbxModel = { name, -1, glm::vec3(), glm::mat4(), glm::quat(), glm::quat(), glm::quat(),
                                          glm::mat4(), glm::vec3(), glm::vec3(),
                                          false, glm::vec3(), glm::quat(), glm::vec3(1.0f), isLimbNode };
                    ExtractedMesh* mesh = NULL;
                    QVector<ExtractedBlendshape> blendshapes;
                    foreach (const FBXNode& subobject, object.children) {
                        bool properties = false;
                        hifi::ByteArray propertyName;
                        int index;
                        if (subobject.name == "Properties60") {
                            properties = true;
                            propertyName = "Property";
                            index = 3;

                        } else if (subobject.name == "Properties70") {
                            properties = true;
                            propertyName = "P";
                            index = 4;
                        }
                        if (properties) {
                            static const QVariant ROTATION_ORDER = hifi::ByteArray("RotationOrder");
                            static const QVariant GEOMETRIC_TRANSLATION = hifi::ByteArray("GeometricTranslation");
                            static const QVariant GEOMETRIC_ROTATION = hifi::ByteArray("GeometricRotation");
                            static const QVariant GEOMETRIC_SCALING = hifi::ByteArray("GeometricScaling");
                            static const QVariant LCL_TRANSLATION = hifi::ByteArray("Lcl Translation");
                            static const QVariant LCL_ROTATION = hifi::ByteArray("Lcl Rotation");
                            static const QVariant LCL_SCALING = hifi::ByteArray("Lcl Scaling");
                            static const QVariant ROTATION_MAX = hifi::ByteArray("RotationMax");
                            static const QVariant ROTATION_MAX_X = hifi::ByteArray("RotationMaxX");
                            static const QVariant ROTATION_MAX_Y = hifi::ByteArray("RotationMaxY");
                            static const QVariant ROTATION_MAX_Z = hifi::ByteArray("RotationMaxZ");
                            static const QVariant ROTATION_MIN = hifi::ByteArray("RotationMin");
                            static const QVariant ROTATION_MIN_X = hifi::ByteArray("RotationMinX");
                            static const QVariant ROTATION_MIN_Y = hifi::ByteArray("RotationMinY");
                            static const QVariant ROTATION_MIN_Z = hifi::ByteArray("RotationMinZ");
                            static const QVariant ROTATION_OFFSET = hifi::ByteArray("RotationOffset");
                            static const QVariant ROTATION_PIVOT = hifi::ByteArray("RotationPivot");
                            static const QVariant SCALING_OFFSET = hifi::ByteArray("ScalingOffset");
                            static const QVariant SCALING_PIVOT = hifi::ByteArray("ScalingPivot");
                            static const QVariant PRE_ROTATION = hifi::ByteArray("PreRotation");
                            static const QVariant POST_ROTATION = hifi::ByteArray("PostRotation");
                            foreach(const FBXNode& property, subobject.children) {
                                const auto& childProperty = property.properties.at(0);
                                if (property.name == propertyName) {
                                    if (childProperty == LCL_TRANSLATION) {
                                        translation = getVec3(property.properties, index);

                                    } else if (childProperty == ROTATION_ORDER) {
                                        rotationOrder = property.properties.at(index).toInt();

                                    } else if (childProperty == ROTATION_OFFSET) {
                                        rotationOffset = getVec3(property.properties, index);

                                    } else if (childProperty == ROTATION_PIVOT) {
                                        rotationPivot = getVec3(property.properties, index);

                                    } else if (childProperty == PRE_ROTATION) {
                                        preRotation = convertRotationToXYZ(rotationOrder, getVec3(property.properties, index));

                                    } else if (childProperty == LCL_ROTATION) {
                                        rotation = convertRotationToXYZ(rotationOrder, getVec3(property.properties, index));

                                    } else if (childProperty == POST_ROTATION) {
                                        postRotation = convertRotationToXYZ(rotationOrder, getVec3(property.properties, index));

                                    } else if (childProperty == SCALING_PIVOT) {
                                        scalePivot = getVec3(property.properties, index);

                                    } else if (childProperty == LCL_SCALING) {
                                        scale = getVec3(property.properties, index);

                                    } else if (childProperty == SCALING_OFFSET) {
                                        scaleOffset = getVec3(property.properties, index);

                                    // NOTE: these rotation limits are stored in degrees (NOT radians)
                                    } else if (childProperty == ROTATION_MIN) {
                                        rotationMin = getVec3(property.properties, index);

                                    } else if (childProperty == ROTATION_MAX) {
                                        rotationMax = getVec3(property.properties, index);

                                    } else if (childProperty == ROTATION_MIN_X) {
                                        rotationMinX = property.properties.at(index).toBool();

                                    } else if (childProperty == ROTATION_MIN_Y) {
                                        rotationMinY = property.properties.at(index).toBool();

                                    } else if (childProperty == ROTATION_MIN_Z) {
                                        rotationMinZ = property.properties.at(index).toBool();

                                    } else if (childProperty == ROTATION_MAX_X) {
                                        rotationMaxX = property.properties.at(index).toBool();

                                    } else if (childProperty == ROTATION_MAX_Y) {
                                        rotationMaxY = property.properties.at(index).toBool();

                                    } else if (childProperty == ROTATION_MAX_Z) {
                                        rotationMaxZ = property.properties.at(index).toBool();
                                    } else if (childProperty == GEOMETRIC_TRANSLATION) {
                                        geometricTranslation = getVec3(property.properties, index);
                                        hasGeometricOffset = true;
                                    } else if (childProperty == GEOMETRIC_ROTATION) {
                                        geometricRotation = getVec3(property.properties, index);
                                        hasGeometricOffset = true;
                                    } else if (childProperty == GEOMETRIC_SCALING) {
                                        geometricScaling = getVec3(property.properties, index);
                                        hasGeometricOffset = true;
                                    }
                                }
                            }
                        } else if (subobject.name == "Vertices" || subobject.name == "DracoMesh") {
                            // it's a mesh as well as a model
                            mesh = &meshes[getID(object.properties)];
                            *mesh = extractMesh(object, meshIndex, deduplicateIndices);

                        } else if (subobject.name == "Shape") {
                            ExtractedBlendshape blendshape =  { subobject.properties.at(0).toString(),
                                extractBlendshape(subobject) };
                            blendshapes.append(blendshape);
                        }
#if defined(DEBUG_FBXSERIALIZER)
                        else if (subobject.name == "TypeFlags") {
                            QString attributetype = subobject.properties.at(0).toString();
                            if (!attributetype.empty()) {
                                if (attributetype == "Light") {
                                    QString lightprop;
                                    foreach (const QVariant& vprop, subobject.properties) {
                                        lightprop = vprop.toString();
                                    }

                                    HFMLight light = extractLight(object);
                                }
                            }
                        } else {
                            QString whatisthat = subobject.name;
                            if (whatisthat == "Shape") {
                            }
                        }
#endif
                    }

                    // add the blendshapes included in the model, if any
                    if (mesh) {
                        foreach (const ExtractedBlendshape& extracted, blendshapes) {
                            addBlendshapes(extracted, blendshapeIndices.values(extracted.id.toLatin1()), *mesh);
                        }
                    }

                    // see FBX documentation, http://download.autodesk.com/us/fbx/20112/FBX_SDK_HELP/index.html
                    fbxModel.translation = translation;

                    fbxModel.preTransform = glm::translate(rotationOffset) * glm::translate(rotationPivot);
                    fbxModel.preRotation = glm::quat(glm::radians(preRotation));
                    fbxModel.rotation = glm::quat(glm::radians(rotation));
                    fbxModel.postRotation = glm::inverse(glm::quat(glm::radians(postRotation)));
                    fbxModel.postTransform = glm::translate(-rotationPivot) * glm::translate(scaleOffset) *
                        glm::translate(scalePivot) * glm::scale(scale) * glm::translate(-scalePivot);
                    // NOTE: angles from the FBX file are in degrees
                    // so we convert them to radians for the FBXModel class
                    fbxModel.rotationMin = glm::radians(glm::vec3(rotationMinX ? rotationMin.x : -180.0f,
                        rotationMinY ? rotationMin.y : -180.0f, rotationMinZ ? rotationMin.z : -180.0f));
                    fbxModel.rotationMax = glm::radians(glm::vec3(rotationMaxX ? rotationMax.x : 180.0f,
                        rotationMaxY ? rotationMax.y : 180.0f, rotationMaxZ ? rotationMax.z : 180.0f));

                    fbxModel.hasGeometricOffset = hasGeometricOffset;
                    fbxModel.geometricTranslation = geometricTranslation;
                    fbxModel.geometricRotation = glm::quat(glm::radians(geometricRotation));
                    fbxModel.geometricScaling = geometricScaling;

                    fbxModels.insert(getID(object.properties), fbxModel);
                } else if (object.name == "Texture") {
                    TextureParam tex;
                    foreach (const FBXNode& subobject, object.children) {
                        const int RELATIVE_FILENAME_MIN_SIZE = 1;
                        const int TEXTURE_NAME_MIN_SIZE = 1;
                        const int TEXTURE_ALPHA_SOURCE_MIN_SIZE = 1;
                        const int MODEL_UV_TRANSLATION_MIN_SIZE = 2;
                        const int MODEL_UV_SCALING_MIN_SIZE = 2;
                        const int CROPPING_MIN_SIZE = 4;
                        if (subobject.name == "RelativeFilename" && subobject.properties.length() >= RELATIVE_FILENAME_MIN_SIZE) {
                            hifi::ByteArray filename = subobject.properties.at(0).toByteArray();
                            hifi::ByteArray filepath = filename.replace('\\', '/');
                            filename = fileOnUrl(filepath, url);
                            _textureFilepaths.insert(getID(object.properties), filepath);
                            _textureFilenames.insert(getID(object.properties), filename);
                        } else if (subobject.name == "TextureName" && subobject.properties.length() >= TEXTURE_NAME_MIN_SIZE) {
                            // trim the name from the timestamp
                            QString name = QString(subobject.properties.at(0).toByteArray());
                            name = name.left(name.indexOf('['));
                            _textureNames.insert(getID(object.properties), name);
                        } else if (subobject.name == "Texture_Alpha_Source" && subobject.properties.length() >= TEXTURE_ALPHA_SOURCE_MIN_SIZE) {
                            tex.assign<uint8_t>(tex.alphaSource, subobject.properties.at(0).value<int>());
                        } else if (subobject.name == "ModelUVTranslation" && subobject.properties.length() >= MODEL_UV_TRANSLATION_MIN_SIZE) {
                            auto newTranslation = glm::vec3(subobject.properties.at(0).value<double>(), subobject.properties.at(1).value<double>(), 0.0);
                            tex.assign(tex.translation, tex.translation + newTranslation);
                        } else if (subobject.name == "ModelUVScaling" && subobject.properties.length() >= MODEL_UV_SCALING_MIN_SIZE) {
                            auto newScaling = glm::vec3(subobject.properties.at(0).value<double>(), subobject.properties.at(1).value<double>(), 1.0);
                            if (newScaling.x == 0.0f) {
                                newScaling.x = 1.0f;
                            }
                            if (newScaling.y == 0.0f) {
                                newScaling.y = 1.0f;
                            }
                            tex.assign(tex.scaling, tex.scaling * newScaling);
                        } else if (subobject.name == "Cropping" && subobject.properties.length() >= CROPPING_MIN_SIZE) {
                            tex.assign(tex.cropping, glm::vec4(subobject.properties.at(0).value<int>(),
                                                                subobject.properties.at(1).value<int>(),
                                                                subobject.properties.at(2).value<int>(),
                                                                subobject.properties.at(3).value<int>()));
                        } else if (subobject.name == "Properties70") {
                            hifi::ByteArray propertyName;
                            int index;
                                propertyName = "P";
                                index = 4;
                                foreach (const FBXNode& property, subobject.children) {
                                    static const QVariant UV_SET = hifi::ByteArray("UVSet");
                                    static const QVariant CURRENT_TEXTURE_BLEND_MODE = hifi::ByteArray("CurrentTextureBlendMode");
                                    static const QVariant USE_MATERIAL = hifi::ByteArray("UseMaterial");
                                    static const QVariant TRANSLATION = hifi::ByteArray("Translation");
                                    static const QVariant ROTATION = hifi::ByteArray("Rotation");
                                    static const QVariant SCALING = hifi::ByteArray("Scaling");
                                    if (property.name == propertyName) {
                                        QString v = property.properties.at(0).toString();
                                        if (property.properties.at(0) == UV_SET) {
                                            std::string uvName = property.properties.at(index).toString().toStdString();
                                            tex.assign(tex.UVSet, property.properties.at(index).toString());
                                        } else if (property.properties.at(0) == CURRENT_TEXTURE_BLEND_MODE) {
                                            tex.assign<uint8_t>(tex.currentTextureBlendMode, property.properties.at(index).value<int>());
                                        } else if (property.properties.at(0) == USE_MATERIAL) {
                                            tex.assign<bool>(tex.useMaterial, property.properties.at(index).value<int>());
                                        } else if (property.properties.at(0) == TRANSLATION) {
                                            tex.assign(tex.translation, tex.translation + getVec3(property.properties, index));
                                        } else if (property.properties.at(0) == ROTATION) {
                                            tex.assign(tex.rotation, getVec3(property.properties, index));
                                        } else if (property.properties.at(0) == SCALING) {
                                            auto newScaling = getVec3(property.properties, index);
                                            if (newScaling.x == 0.0f) {
                                                newScaling.x = 1.0f;
                                            }
                                            if (newScaling.y == 0.0f) {
                                                newScaling.y = 1.0f;
                                            }
                                            if (newScaling.z == 0.0f) {
                                                newScaling.z = 1.0f;
                                            }
                                            tex.assign(tex.scaling, tex.scaling * newScaling);
                                        }
#if defined(DEBUG_FBXSERIALIZER)
                                        else {
                                            QString propName = v;
                                            unknown++;
                                        }
#endif
                                    }
                                }
                        }
#if defined(DEBUG_FBXSERIALIZER)
                        else {
                            if (subobject.name == "Type") {
                            } else if (subobject.name == "Version") {
                            } else if (subobject.name == "FileName") {
                            } else if (subobject.name == "Media") {
                            } else {
                                QString subname = subobject.name.data();
                                unknown++;
                            }
                        }
#endif
                    }

                    if (!tex.isDefault) {
                        _textureParams.insert(getID(object.properties), tex);
                    }
                } else if (object.name == "Video") {
                    hifi::ByteArray filepath;
                    hifi::ByteArray content;
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "RelativeFilename") {
                            filepath = subobject.properties.at(0).toByteArray();
                            filepath = filepath.replace('\\', '/');

                        } else if (subobject.name == "Content" && !subobject.properties.isEmpty()) {
                            content = subobject.properties.at(0).toByteArray();
                        }
                    }
                    if (!content.isEmpty()) {
                        _textureContent.insert(filepath, content);
                    }
                } else if (object.name == "Material") {
                    HFMMaterial material;
                    MaterialParam materialParam;
                    material.name = getMaterialName(object.properties);
                    foreach (const FBXNode& subobject, object.children) {
                        bool properties = false;

                        hifi::ByteArray propertyName;
                        int index;
                        if (subobject.name == "Properties60") {
                            properties = true;
                            propertyName = "Property";
                            index = 3;

                        } else if (subobject.name == "Properties70") {
                            properties = true;
                            propertyName = "P";
                            index = 4;
                        } else if (subobject.name == "ShadingModel") {
                            material.shadingModel = subobject.properties.at(0).toString();
                        }

                        if (properties) {
                            std::vector<std::string> unknowns;
                            static const QVariant DIFFUSE_COLOR = hifi::ByteArray("DiffuseColor");
                            static const QVariant DIFFUSE_FACTOR = hifi::ByteArray("DiffuseFactor");
                            static const QVariant DIFFUSE = hifi::ByteArray("Diffuse");
                            static const QVariant SPECULAR_COLOR = hifi::ByteArray("SpecularColor");
                            static const QVariant SPECULAR_FACTOR = hifi::ByteArray("SpecularFactor");
                            static const QVariant SPECULAR = hifi::ByteArray("Specular");
                            static const QVariant EMISSIVE_COLOR = hifi::ByteArray("EmissiveColor");
                            static const QVariant EMISSIVE_FACTOR = hifi::ByteArray("EmissiveFactor");
                            static const QVariant EMISSIVE = hifi::ByteArray("Emissive");
                            static const QVariant AMBIENT_FACTOR = hifi::ByteArray("AmbientFactor");
                            static const QVariant SHININESS = hifi::ByteArray("Shininess");
                            static const QVariant OPACITY = hifi::ByteArray("Opacity");
                            static const QVariant MAYA_USE_NORMAL_MAP = hifi::ByteArray("Maya|use_normal_map");
                            static const QVariant MAYA_BASE_COLOR = hifi::ByteArray("Maya|base_color");
                            static const QVariant MAYA_USE_COLOR_MAP = hifi::ByteArray("Maya|use_color_map");
                            static const QVariant MAYA_ROUGHNESS = hifi::ByteArray("Maya|roughness");
                            static const QVariant MAYA_USE_ROUGHNESS_MAP = hifi::ByteArray("Maya|use_roughness_map");
                            static const QVariant MAYA_METALLIC = hifi::ByteArray("Maya|metallic");
                            static const QVariant MAYA_USE_METALLIC_MAP = hifi::ByteArray("Maya|use_metallic_map");
                            static const QVariant MAYA_EMISSIVE = hifi::ByteArray("Maya|emissive");
                            static const QVariant MAYA_EMISSIVE_INTENSITY = hifi::ByteArray("Maya|emissive_intensity");
                            static const QVariant MAYA_USE_EMISSIVE_MAP = hifi::ByteArray("Maya|use_emissive_map");
                            static const QVariant MAYA_USE_AO_MAP = hifi::ByteArray("Maya|use_ao_map");
                            static const QVariant MAYA_UV_SCALE = hifi::ByteArray("Maya|uv_scale");
                            static const QVariant MAYA_UV_OFFSET = hifi::ByteArray("Maya|uv_offset");
                            static const int MAYA_UV_OFFSET_PROPERTY_LENGTH = 6;
                            static const int MAYA_UV_SCALE_PROPERTY_LENGTH = 6;




                            foreach(const FBXNode& property, subobject.children) {
                                if (property.name == propertyName) {
                                    if (property.properties.at(0) == DIFFUSE_COLOR) {
                                        material.diffuseColor = getVec3(property.properties, index);
                                    } else if (property.properties.at(0) == DIFFUSE_FACTOR) {
                                        material.diffuseFactor = property.properties.at(index).value<double>();
                                    } else if (property.properties.at(0) == DIFFUSE) {
                                        // NOTE: this is uneeded but keep it for now for debug
                                        //  material.diffuseColor = getVec3(property.properties, index);
                                        //  material.diffuseFactor = 1.0;

                                    } else if (property.properties.at(0) == SPECULAR_COLOR) {
                                        material.specularColor = getVec3(property.properties, index);
                                    } else if (property.properties.at(0) == SPECULAR_FACTOR) {
                                        material.specularFactor = property.properties.at(index).value<double>();
                                    } else if (property.properties.at(0) == SPECULAR) {
                                        // NOTE: this is uneeded but keep it for now for debug
                                        //  material.specularColor = getVec3(property.properties, index);
                                        //  material.specularFactor = 1.0;

                                    } else if (property.properties.at(0) == EMISSIVE_COLOR) {
                                        material.emissiveColor = getVec3(property.properties, index);
                                    } else if (property.properties.at(0) == EMISSIVE_FACTOR) {
                                        material.emissiveFactor = property.properties.at(index).value<double>();
                                    } else if (property.properties.at(0) == EMISSIVE) {
                                        // NOTE: this is uneeded but keep it for now for debug
                                        //  material.emissiveColor = getVec3(property.properties, index);
                                        //  material.emissiveFactor = 1.0;

                                    } else if (property.properties.at(0) == AMBIENT_FACTOR) {
                                        material.ambientFactor = property.properties.at(index).value<double>();
                                        // Detected just for BLender AO vs lightmap
                                    } else if (property.properties.at(0) == SHININESS) {
                                        material.shininess = property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == OPACITY) {
                                        material.opacity = property.properties.at(index).value<double>();
                                    }

                                    // Sting Ray Material Properties!!!!
                                    else if (property.properties.at(0) == MAYA_USE_NORMAL_MAP) {
                                        material.isPBSMaterial = true;
                                        material.useNormalMap = (bool)property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == MAYA_BASE_COLOR) {
                                        material.isPBSMaterial = true;
                                        material.diffuseColor = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == MAYA_USE_COLOR_MAP) {
                                        material.isPBSMaterial = true;
                                        material.useAlbedoMap = (bool) property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == MAYA_ROUGHNESS) {
                                        material.isPBSMaterial = true;
                                        material.roughness = property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == MAYA_USE_ROUGHNESS_MAP) {
                                        material.isPBSMaterial = true;
                                        material.useRoughnessMap = (bool)property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == MAYA_METALLIC) {
                                        material.isPBSMaterial = true;
                                        material.metallic = property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == MAYA_USE_METALLIC_MAP) {
                                        material.isPBSMaterial = true;
                                        material.useMetallicMap = (bool)property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == MAYA_EMISSIVE) {
                                        material.isPBSMaterial = true;
                                        material.emissiveColor = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == MAYA_EMISSIVE_INTENSITY) {
                                        material.isPBSMaterial = true;
                                        material.emissiveIntensity = property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == MAYA_USE_EMISSIVE_MAP) {
                                        material.isPBSMaterial = true;
                                        material.useEmissiveMap = (bool)property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == MAYA_USE_AO_MAP) {
                                        material.isPBSMaterial = true;
                                        material.useOcclusionMap = (bool)property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == MAYA_UV_SCALE) {
                                        if (property.properties.size() == MAYA_UV_SCALE_PROPERTY_LENGTH) {
                                            // properties: { "Maya|uv_scale", "Vector2D", "Vector2", nothing, double, double }
                                            glm::vec3 scale = glm::vec3(property.properties.at(4).value<double>(), property.properties.at(5).value<double>(), 1.0);
                                            if (scale.x == 0.0f) {
                                                scale.x = 1.0f;
                                            }
                                            if (scale.y == 0.0f) {
                                                scale.y = 1.0f;
                                            }
                                            if (scale.z == 0.0f) {
                                                scale.z = 1.0f;
                                            }
                                            materialParam.scaling *= scale;
                                        }
                                    } else if (property.properties.at(0) == MAYA_UV_OFFSET) {
                                        if (property.properties.size() == MAYA_UV_OFFSET_PROPERTY_LENGTH) {
                                            // properties: { "Maya|uv_offset", "Vector2D", "Vector2", nothing, double, double }
                                            glm::vec3 translation = glm::vec3(property.properties.at(4).value<double>(), property.properties.at(5).value<double>(), 1.0);
                                            materialParam.translation += translation;
                                        }
                                    } else {
                                        const QString propname = property.properties.at(0).toString();
                                        unknowns.push_back(propname.toStdString());
                                    }
                                }
                            }
                        }
#if defined(DEBUG_FBXSERIALIZER)
                        else {
                            QString propname = subobject.name.data();
                            int unknown = 0;
                            if ( (propname == "Version")
                                ||(propname == "Multilayer")) {
                            } else {
                                unknown++;
                            }
                        }
#endif
                    }
                    material.materialID = getID(object.properties);
                    _hfmMaterials.insert(material.materialID, material);
                    _materialParams.insert(material.materialID, materialParam);


                } else if (object.name == "NodeAttribute") {
#if defined(DEBUG_FBXSERIALIZER)
                    std::vector<QString> properties;
                    foreach(const QVariant& v, object.properties) {
                        properties.push_back(v.toString());
                    }
#endif
                    QString attribID = getID(object.properties);
                    QString attributetype;
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "TypeFlags") {
                            typeFlags.insert(getID(object.properties), subobject.properties.at(0).toString());
                            attributetype = subobject.properties.at(0).toString();
                        }
                    }

                    if (!attributetype.isEmpty()) {
                        if (attributetype == "Light") {
                            HFMLight light = extractLight(object);
                            lights[attribID] = light;
                        }
                    }

                } else if (object.name == "Deformer") {
                    if (object.properties.last() == "Cluster") {
                        Cluster cluster;
                        foreach (const FBXNode& subobject, object.children) {
                            if (subobject.name == "Indexes") {
                                cluster.indices = getIntVector(subobject);

                            } else if (subobject.name == "Weights") {
                                cluster.weights = getDoubleVector(subobject);

                            } else if (subobject.name == "TransformLink") {
                                QVector<double> values = getDoubleVector(subobject);
                                cluster.transformLink = createMat4(values);
                            }
                        }

                        // skip empty clusters
                        if (cluster.indices.size() > 0 && cluster.weights.size() > 0) {
                            clusters.insert(getID(object.properties), cluster);
                        }

                    } else if (object.properties.last() == "BlendShapeChannel") {
                        hifi::ByteArray name = object.properties.at(1).toByteArray();

                        name = name.left(name.indexOf('\0'));
                        if (!blendshapeIndices.contains(name)) {
                            // try everything after the dot
                            name = name.mid(name.lastIndexOf('.') + 1);
                        }
                        QString id = getID(object.properties);
                        hfmModel.blendshapeChannelNames << name;
                        foreach (const WeightedIndex& index, blendshapeIndices.values(name)) {
                            blendshapeChannelIndices.insert(id, index);
                        }
                    }
                } else if (object.name == "AnimationCurve") {
                    AnimationCurve curve;
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "KeyValueFloat") {
                            curve.values = getFloatVector(subobject);
                        }
                    }
                    animationCurves.insert(getID(object.properties), curve);

                }
#if defined(DEBUG_FBXSERIALIZER)
                 else {
                    QString objectname = object.name.data();
                    if ( objectname == "Pose"
                        || objectname == "AnimationStack"
                        || objectname == "AnimationLayer"
                        || objectname == "AnimationCurveNode") {
                    } else {
                        unknown++;
                    }
                }
#endif
            }
        } else if (child.name == "Connections") {
            static const QVariant OO = hifi::ByteArray("OO");
            static const QVariant OP = hifi::ByteArray("OP");
            foreach (const FBXNode& connection, child.children) {
                if (connection.name == "C" || connection.name == "Connect") {
                    if (connection.properties.at(0) == OO) {
                        QString childID = getID(connection.properties, 1);
                        QString parentID = getID(connection.properties, 2);
                        ooChildToParent.insert(childID, parentID);
                        if (!hifiGlobalNodeID.isEmpty() && (parentID == hifiGlobalNodeID)) {
                            std::map< QString, HFMLight >::iterator lightIt = lights.find(childID);
                            if (lightIt != lights.end()) {
                                _lightmapLevel = (*lightIt).second.intensity;
                                if (_lightmapLevel <= 0.0f) {
                                    _loadLightmaps = false;
                                }
                                _lightmapOffset = glm::clamp((*lightIt).second.color.x, 0.f, 1.f);
                            }
                        }
                    } else if (connection.properties.at(0) == OP) {
                        int counter = 0;
                        hifi::ByteArray type = connection.properties.at(3).toByteArray().toLower();
                        if (type.contains("DiffuseFactor")) {
                            diffuseFactorTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if ((type.contains("diffuse") && !type.contains("tex_global_diffuse"))) {
                            diffuseTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("tex_color_map")) {
                            diffuseTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("transparentcolor")) { // Maya way of passing TransparentMap
                            transparentTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("transparencyfactor")) { // Blender way of passing TransparentMap
                            transparentTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("bump")) {
                            bumpTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("normal")) {
                            normalTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("tex_normal_map")) {
                            normalTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if ((type.contains("specular") && !type.contains("tex_global_specular")) || type.contains("reflection")) {
                            specularTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("tex_metallic_map")) {
                            metallicTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("shininess")) {
                            shininessTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("tex_roughness_map")) {
                            roughnessTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("emissive")) {
                            emissiveTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("tex_emissive_map")) {
                            emissiveTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("ambientcolor")) {
                            ambientTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("ambientfactor")) {
                            ambientFactorTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type.contains("tex_ao_map")) {
                            occlusionTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type == "lcl rotation") {
                            localRotations.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type == "lcl translation") {
                            localTranslations.insert(getID(connection.properties, 2), getID(connection.properties, 1));

                        } else if (type == "d|x") {
                            xComponents.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type == "d|y") {
                            yComponents.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        } else if (type == "d|z") {
                            zComponents.insert(getID(connection.properties, 2), getID(connection.properties, 1));

                        } else {
                            QString typenam = type.data();
                            counter++;
                        }
                    }
                    if (_connectionParentMap.value(getID(connection.properties, 1)) == "0") {
                        // don't assign the new parent
                        qCDebug(modelformat) << "root node " << getID(connection.properties, 1) << "  has discarded parent " << getID(connection.properties, 2);
                        _connectionChildMap.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                    } else {
                        _connectionParentMap.insert(getID(connection.properties, 1), getID(connection.properties, 2));
                        _connectionChildMap.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                    }
                }
            }
        }
#if defined(DEBUG_FBXSERIALIZER)
        else {
            QString objectname = child.name.data();
            if ( objectname == "Pose"
                || objectname == "CreationTime"
                || objectname == "FileId"
                || objectname == "Creator"
                || objectname == "Documents"
                || objectname == "References"
                || objectname == "Definitions"
                || objectname == "Takes"
                || objectname == "AnimationStack"
                || objectname == "AnimationLayer"
                || objectname == "AnimationCurveNode") {
            } else {
                unknown++;
            }
        }
#endif
    }

    // TODO: check if is code is needed
    if (!lights.empty()) {
        if (hifiGlobalNodeID.isEmpty()) {
            auto light = lights.begin();
            _lightmapLevel = (*light).second.intensity;
        }
    }

    // assign the blendshapes to their corresponding meshes
    foreach (const ExtractedBlendshape& extracted, blendshapes) {
        QString blendshapeChannelID = _connectionParentMap.value(extracted.id);
        QString blendshapeID = _connectionParentMap.value(blendshapeChannelID);
        QString meshID = _connectionParentMap.value(blendshapeID);
        addBlendshapes(extracted, blendshapeChannelIndices.values(blendshapeChannelID), meshes[meshID]);
    }

    // get offset transform from mapping
    float offsetScale = mapping.value("scale", 1.0f).toFloat() * unitScaleFactor * METERS_PER_CENTIMETER;
    glm::quat offsetRotation = glm::quat(glm::radians(glm::vec3(mapping.value("rx").toFloat(),
            mapping.value("ry").toFloat(), mapping.value("rz").toFloat())));
    hfmModel.offset = glm::translate(glm::vec3(mapping.value("tx").toFloat(), mapping.value("ty").toFloat(),
        mapping.value("tz").toFloat())) * glm::mat4_cast(offsetRotation) *
            glm::scale(glm::vec3(offsetScale, offsetScale, offsetScale));

    // get the list of models in depth-first traversal order
    QVector<QString> modelIDs;
    QSet<QString> remainingFBXModels;
    for (QHash<QString, FBXModel>::const_iterator fbxModel = fbxModels.constBegin(); fbxModel != fbxModels.constEnd(); fbxModel++) {
        // models with clusters must be parented to the cluster top
        // Unless the model is a root node.
        bool isARootNode = !modelIDs.contains(_connectionParentMap.value(fbxModel.key()));
        if (!isARootNode) {
            foreach(const QString& deformerID, _connectionChildMap.values(fbxModel.key())) {
                foreach(const QString& clusterID, _connectionChildMap.values(deformerID)) {
                    if (!clusters.contains(clusterID)) {
                        continue;
                    }
                    QString topID = getTopModelID(_connectionParentMap, fbxModels, _connectionChildMap.value(clusterID), url);
                    _connectionChildMap.remove(_connectionParentMap.take(fbxModel.key()), fbxModel.key());
                    _connectionParentMap.insert(fbxModel.key(), topID);
                    goto outerBreak;
                }
            }
            outerBreak: ;
        }

        // make sure the parent is in the child map
        QString parent = _connectionParentMap.value(fbxModel.key());
        if (!_connectionChildMap.contains(parent, fbxModel.key())) {
            _connectionChildMap.insert(parent, fbxModel.key());
        }
        remainingFBXModels.insert(fbxModel.key());
    }
    while (!remainingFBXModels.isEmpty()) {
        QString first = *remainingFBXModels.constBegin();
        foreach (const QString& id, remainingFBXModels) {
            if (id < first) {
                first = id;
            }
        }
        QString topID = getTopModelID(_connectionParentMap, fbxModels, first, url);
        appendModelIDs(_connectionParentMap.value(topID), _connectionChildMap, fbxModels, remainingFBXModels, modelIDs, true);
    }

    // figure the number of animation frames from the curves
    int frameCount = 1;
    foreach (const AnimationCurve& curve, animationCurves) {
        frameCount = qMax(frameCount, curve.values.size());
    }
    for (int i = 0; i < frameCount; i++) {
        HFMAnimationFrame frame;
        frame.rotations.resize(modelIDs.size());
        frame.translations.resize(modelIDs.size());
        hfmModel.animationFrames.append(frame);
    }

    // convert the models to joints
    hfmModel.hasSkeletonJoints = false;

    foreach (const QString& modelID, modelIDs) {
        const FBXModel& fbxModel = fbxModels[modelID];
        HFMJoint joint;
        joint.parentIndex = fbxModel.parentIndex;
        int jointIndex = hfmModel.joints.size();

        joint.translation = fbxModel.translation; // these are usually in centimeters
        joint.preTransform = fbxModel.preTransform;
        joint.preRotation = fbxModel.preRotation;
        joint.rotation = fbxModel.rotation;
        joint.postRotation = fbxModel.postRotation;
        joint.postTransform = fbxModel.postTransform;
        joint.rotationMin = fbxModel.rotationMin;
        joint.rotationMax = fbxModel.rotationMax;

        joint.hasGeometricOffset = fbxModel.hasGeometricOffset;
        joint.geometricTranslation = fbxModel.geometricTranslation;
        joint.geometricRotation = fbxModel.geometricRotation;
        joint.geometricScaling = fbxModel.geometricScaling;
        joint.isSkeletonJoint = fbxModel.isLimbNode;
        hfmModel.hasSkeletonJoints = (hfmModel.hasSkeletonJoints || joint.isSkeletonJoint);
        if (applyUpAxisZRotation && joint.parentIndex == -1) {
            joint.rotation *= upAxisZRotation;
            joint.translation = upAxisZRotation * joint.translation;
        }
        glm::quat combinedRotation = joint.preRotation * joint.rotation * joint.postRotation;
        if (joint.parentIndex == -1) {
            joint.transform = hfmModel.offset * glm::translate(joint.translation) * joint.preTransform *
                glm::mat4_cast(combinedRotation) * joint.postTransform;
            joint.inverseDefaultRotation = glm::inverse(combinedRotation);
            joint.distanceToParent = 0.0f;

        } else {
            const HFMJoint& parentJoint = hfmModel.joints.at(joint.parentIndex);
            joint.transform = parentJoint.transform * glm::translate(joint.translation) *
                joint.preTransform * glm::mat4_cast(combinedRotation) * joint.postTransform;
            joint.inverseDefaultRotation = glm::inverse(combinedRotation) * parentJoint.inverseDefaultRotation;
            joint.distanceToParent = glm::distance(extractTranslation(parentJoint.transform),
                extractTranslation(joint.transform));
        }
        joint.inverseBindRotation = joint.inverseDefaultRotation;
        joint.name = fbxModel.name;

        joint.bindTransformFoundInCluster = false;

        QString rotationID = localRotations.value(modelID);
        AnimationCurve xRotCurve = animationCurves.value(xComponents.value(rotationID));
        AnimationCurve yRotCurve = animationCurves.value(yComponents.value(rotationID));
        AnimationCurve zRotCurve = animationCurves.value(zComponents.value(rotationID));

        QString translationID = localTranslations.value(modelID);
        AnimationCurve xPosCurve = animationCurves.value(xComponents.value(translationID));
        AnimationCurve yPosCurve = animationCurves.value(yComponents.value(translationID));
        AnimationCurve zPosCurve = animationCurves.value(zComponents.value(translationID));

        glm::vec3 defaultRotValues = glm::degrees(safeEulerAngles(joint.rotation));
        glm::vec3 defaultPosValues = joint.translation;

        for (int i = 0; i < frameCount; i++) {
            hfmModel.animationFrames[i].rotations[jointIndex] = glm::quat(glm::radians(glm::vec3(
                xRotCurve.values.isEmpty() ? defaultRotValues.x : xRotCurve.values.at(i % xRotCurve.values.size()),
                yRotCurve.values.isEmpty() ? defaultRotValues.y : yRotCurve.values.at(i % yRotCurve.values.size()),
                zRotCurve.values.isEmpty() ? defaultRotValues.z : zRotCurve.values.at(i % zRotCurve.values.size()))));
            hfmModel.animationFrames[i].translations[jointIndex] = glm::vec3(
                xPosCurve.values.isEmpty() ? defaultPosValues.x : xPosCurve.values.at(i % xPosCurve.values.size()),
                yPosCurve.values.isEmpty() ? defaultPosValues.y : yPosCurve.values.at(i % yPosCurve.values.size()),
                zPosCurve.values.isEmpty() ? defaultPosValues.z : zPosCurve.values.at(i % zPosCurve.values.size()));
            if ((fbxVersionNumber < 7500) && (i == 0)) {
                joint.translation = hfmModel.animationFrames[i].translations[jointIndex];
                joint.rotation = hfmModel.animationFrames[i].rotations[jointIndex];
            }

        }
        hfmModel.joints.append(joint);
    }

    // NOTE: shapeVertices are in joint-frame
    hfmModel.shapeVertices.resize(std::max(1, hfmModel.joints.size()) );

    hfmModel.bindExtents.reset();
    hfmModel.meshExtents.reset();

    // Create the Material Library
    consolidateHFMMaterials();

    // We can't allow the scaling of a given image to different sizes, because the hash used for the KTX cache is based on the original image
    // Allowing scaling of the same image to different sizes would cause different KTX files to target the same cache key
#if 0
    // HACK: until we get proper LOD management we're going to cap model textures
    // according to how many unique textures the model uses:
    //   1 - 8 textures --> 2048
    //   8 - 32 textures --> 1024
    //   33 - 128 textures --> 512
    // etc...
    QSet<QString> uniqueTextures;
    for (auto& material : _hfmMaterials) {
        material.getTextureNames(uniqueTextures);
    }
    int numTextures = uniqueTextures.size();
    const int MAX_NUM_TEXTURES_AT_MAX_RESOLUTION = 8;
    int maxWidth = sqrt(MAX_NUM_PIXELS_FOR_FBX_TEXTURE);

    if (numTextures > MAX_NUM_TEXTURES_AT_MAX_RESOLUTION) {
        int numTextureThreshold = MAX_NUM_TEXTURES_AT_MAX_RESOLUTION;
        const int MIN_MIP_TEXTURE_WIDTH = 64;
        do {
            maxWidth /= 2;
            numTextureThreshold *= 4;
        } while (numTextureThreshold < numTextures && maxWidth > MIN_MIP_TEXTURE_WIDTH);

        qCDebug(modelformat) << "Capped square texture width =" << maxWidth << "for model" << url << "with" << numTextures << "textures";
        for (auto& material : _hfmMaterials) {
            material.setMaxNumPixelsPerTexture(maxWidth * maxWidth);
        }
    }
#endif
    hfmModel.materials = _hfmMaterials;

    // see if any materials have texture children
    bool materialsHaveTextures = checkMaterialsHaveTextures(_hfmMaterials, _textureFilenames, _connectionChildMap);

    for (QMap<QString, ExtractedMesh>::iterator it = meshes.begin(); it != meshes.end(); it++) {
        ExtractedMesh& extracted = it.value();

        extracted.mesh.meshExtents.reset();

        // accumulate local transforms
        QString modelID = fbxModels.contains(it.key()) ? it.key() : _connectionParentMap.value(it.key());
        glm::mat4 modelTransform = getGlobalTransform(_connectionParentMap, fbxModels, modelID, hfmModel.applicationName == "mixamo.com", url);

        // compute the mesh extents from the transformed vertices
        foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
            glm::vec3 transformedVertex = glm::vec3(modelTransform * glm::vec4(vertex, 1.0f));
            hfmModel.meshExtents.minimum = glm::min(hfmModel.meshExtents.minimum, transformedVertex);
            hfmModel.meshExtents.maximum = glm::max(hfmModel.meshExtents.maximum, transformedVertex);

            extracted.mesh.meshExtents.minimum = glm::min(extracted.mesh.meshExtents.minimum, transformedVertex);
            extracted.mesh.meshExtents.maximum = glm::max(extracted.mesh.meshExtents.maximum, transformedVertex);
            extracted.mesh.modelTransform = modelTransform;
        }

        // look for textures, material properties
        // allocate the Part material library
        // NOTE: extracted.partMaterialTextures is empty for FBX_DRACO_MESH_VERSION >= 2. In that case, the mesh part's materialID string is already defined.
        int materialIndex = 0;
        int textureIndex = 0;
        QList<QString> children = _connectionChildMap.values(modelID);
        for (int i = children.size() - 1; i >= 0; i--) {

            const QString& childID = children.at(i);
            if (_hfmMaterials.contains(childID)) {
                // the pure material associated with this part
                HFMMaterial material = _hfmMaterials.value(childID);

                for (int j = 0; j < extracted.partMaterialTextures.size(); j++) {
                    if (extracted.partMaterialTextures.at(j).first == materialIndex) {
                        HFMMeshPart& part = extracted.mesh.parts[j];
                        part.materialID = material.materialID;
                    }
                }

                materialIndex++;
            } else if (_textureFilenames.contains(childID)) {
                // NOTE (Sabrina 2019/01/11): getTextures now takes in the materialID as a second parameter, because FBX material nodes can sometimes have uv transform information (ex: "Maya|uv_scale")
                // I'm leaving the second parameter blank right now as this code may never be used.
                HFMTexture texture = getTexture(childID, "");
                for (int j = 0; j < extracted.partMaterialTextures.size(); j++) {
                    int partTexture = extracted.partMaterialTextures.at(j).second;
                    if (partTexture == textureIndex && !(partTexture == 0 && materialsHaveTextures)) {
                        // TODO: DO something here that replaces this legacy code
                        // Maybe create a material just for this part with the correct textures?
                        // extracted.mesh.parts[j].diffuseTexture = texture;
                    }
                }
                textureIndex++;
            }
        }

        // find the clusters with which the mesh is associated
        QVector<QString> clusterIDs;
        foreach (const QString& childID, _connectionChildMap.values(it.key())) {
            foreach (const QString& clusterID, _connectionChildMap.values(childID)) {
                if (!clusters.contains(clusterID)) {
                    continue;
                }
                HFMCluster hfmCluster;
                const Cluster& cluster = clusters[clusterID];
                clusterIDs.append(clusterID);

                // see http://stackoverflow.com/questions/13566608/loading-skinning-information-from-fbx for a discussion
                // of skinning information in FBX
                QString jointID = _connectionChildMap.value(clusterID);
                hfmCluster.jointIndex = modelIDs.indexOf(jointID);
                if (hfmCluster.jointIndex == -1) {
                    qCDebug(modelformat) << "Joint not in model list: " << jointID;
                    hfmCluster.jointIndex = 0;
                }

                hfmCluster.inverseBindMatrix = glm::inverse(cluster.transformLink) * modelTransform;

                // slam bottom row to (0, 0, 0, 1), we KNOW this is not a perspective matrix and
                // sometimes floating point fuzz can be introduced after the inverse.
                hfmCluster.inverseBindMatrix[0][3] = 0.0f;
                hfmCluster.inverseBindMatrix[1][3] = 0.0f;
                hfmCluster.inverseBindMatrix[2][3] = 0.0f;
                hfmCluster.inverseBindMatrix[3][3] = 1.0f;

                hfmCluster.inverseBindTransform = Transform(hfmCluster.inverseBindMatrix);

                extracted.mesh.clusters.append(hfmCluster);

                // override the bind rotation with the transform link
                HFMJoint& joint = hfmModel.joints[hfmCluster.jointIndex];
                joint.inverseBindRotation = glm::inverse(extractRotation(cluster.transformLink));
                joint.bindTransform = cluster.transformLink;
                joint.bindTransformFoundInCluster = true;

                // update the bind pose extents
                glm::vec3 bindTranslation = extractTranslation(hfmModel.offset * joint.bindTransform);
                hfmModel.bindExtents.addPoint(bindTranslation);
            }
        }

        // the last cluster is the root cluster
        {
            HFMCluster cluster;
            cluster.jointIndex = modelIDs.indexOf(modelID);
            if (cluster.jointIndex == -1) {
                qCDebug(modelformat) << "Model not in model list: " << modelID;
                cluster.jointIndex = 0;
            }
            extracted.mesh.clusters.append(cluster);
        }

        // whether we're skinned depends on how many clusters are attached
        if (clusterIDs.size() > 1) {
            // this is a multi-mesh joint
            const int WEIGHTS_PER_VERTEX = 4;
            int numClusterIndices = extracted.mesh.vertices.size() * WEIGHTS_PER_VERTEX;
            extracted.mesh.clusterIndices.fill(extracted.mesh.clusters.size() - 1, numClusterIndices);
            QVector<float> weightAccumulators;
            weightAccumulators.fill(0.0f, numClusterIndices);

            for (int i = 0; i < clusterIDs.size(); i++) {
                QString clusterID = clusterIDs.at(i);
                const Cluster& cluster = clusters[clusterID];
                const HFMCluster& hfmCluster = extracted.mesh.clusters.at(i);
                int jointIndex = hfmCluster.jointIndex;
                HFMJoint& joint = hfmModel.joints[jointIndex];

                glm::mat4 meshToJoint = glm::inverse(joint.bindTransform) * modelTransform;
                ShapeVertices& points = hfmModel.shapeVertices.at(jointIndex);

                for (int j = 0; j < cluster.indices.size(); j++) {
                    int oldIndex = cluster.indices.at(j);
                    float weight = cluster.weights.at(j);
                    for (QMultiHash<int, int>::const_iterator it = extracted.newIndices.constFind(oldIndex);
                            it != extracted.newIndices.end() && it.key() == oldIndex; it++) {
                        int newIndex = it.value();

                        // remember vertices with at least 1/4 weight
                        // FIXME: vertices with no weightpainting won't get recorded here
                        const float EXPANSION_WEIGHT_THRESHOLD = 0.25f;
                        if (weight >= EXPANSION_WEIGHT_THRESHOLD) {
                            // transform to joint-frame and save for later
                            const glm::mat4 vertexTransform = meshToJoint * glm::translate(extracted.mesh.vertices.at(newIndex));
                            points.push_back(extractTranslation(vertexTransform));
                        }

                        // look for an unused slot in the weights vector
                        int weightIndex = newIndex * WEIGHTS_PER_VERTEX;
                        int lowestIndex = -1;
                        float lowestWeight = FLT_MAX;
                        int k = 0;
                        for (; k < WEIGHTS_PER_VERTEX; k++) {
                            if (weightAccumulators[weightIndex + k] == 0.0f) {
                                extracted.mesh.clusterIndices[weightIndex + k] = i;
                                weightAccumulators[weightIndex + k] = weight;
                                break;
                            }
                            if (weightAccumulators[weightIndex + k] < lowestWeight) {
                                lowestIndex = k;
                                lowestWeight = weightAccumulators[weightIndex + k];
                            }
                        }
                        if (k == WEIGHTS_PER_VERTEX && weight > lowestWeight) {
                            // no space for an additional weight; we must replace the lowest
                            weightAccumulators[weightIndex + lowestIndex] = weight;
                            extracted.mesh.clusterIndices[weightIndex + lowestIndex] = i;
                        }
                    }
                }
            }

            // now that we've accumulated the most relevant weights for each vertex
            // normalize and compress to 16-bits
            extracted.mesh.clusterWeights.fill(0, numClusterIndices);
            int numVertices = extracted.mesh.vertices.size();
            for (int i = 0; i < numVertices; ++i) {
                int j = i * WEIGHTS_PER_VERTEX;

                // normalize weights into uint16_t
                float totalWeight = 0.0f;
                for (int k = j; k < j + WEIGHTS_PER_VERTEX; ++k) {
                    totalWeight += weightAccumulators[k];
                }

                const float ALMOST_HALF = 0.499f;
                if (totalWeight > 0.0f) {
                    float weightScalingFactor = (float)(UINT16_MAX) / totalWeight;
                    for (int k = j; k < j + WEIGHTS_PER_VERTEX; ++k) {
                        extracted.mesh.clusterWeights[k] = (uint16_t)(weightScalingFactor * weightAccumulators[k] + ALMOST_HALF);
                    }
                } else {
                    extracted.mesh.clusterWeights[j] = (uint16_t)((float)(UINT16_MAX) + ALMOST_HALF);
                }
            }
        } else {
            // this is a single-joint mesh
            const HFMCluster& firstHFMCluster = extracted.mesh.clusters.at(0);
            int jointIndex = firstHFMCluster.jointIndex;
            HFMJoint& joint = hfmModel.joints[jointIndex];

            // transform cluster vertices to joint-frame and save for later
            glm::mat4 meshToJoint = glm::inverse(joint.bindTransform) * modelTransform;
            ShapeVertices& points = hfmModel.shapeVertices.at(jointIndex);
            foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
                const glm::mat4 vertexTransform = meshToJoint * glm::translate(vertex);
                points.push_back(extractTranslation(vertexTransform));
            }

            // Apply geometric offset, if present, by transforming the vertices directly
            if (joint.hasGeometricOffset) {
                glm::mat4 geometricOffset = createMatFromScaleQuatAndPos(joint.geometricScaling, joint.geometricRotation, joint.geometricTranslation);
                for (int i = 0; i < extracted.mesh.vertices.size(); i++) {
                    extracted.mesh.vertices[i] = transformPoint(geometricOffset, extracted.mesh.vertices[i]);
                }
            }
        }

        hfmModel.meshes.append(extracted.mesh);
        int meshIndex = hfmModel.meshes.size() - 1;
        meshIDsToMeshIndices.insert(it.key(), meshIndex);
    }

    // attempt to map any meshes to a named model
    for (QHash<QString, int>::const_iterator m = meshIDsToMeshIndices.constBegin();
            m != meshIDsToMeshIndices.constEnd(); m++) {

        const QString& meshID = m.key();
        int meshIndex = m.value();

        if (ooChildToParent.contains(meshID)) {
            const QString& modelID = ooChildToParent.value(meshID);
            if (modelIDsToNames.contains(modelID)) {
                const QString& modelName = modelIDsToNames.value(modelID);
                hfmModel.meshIndicesToModelNames.insert(meshIndex, modelName);
            }
        }
    }

    if (applyUpAxisZRotation) {
        hfmModelPtr->meshExtents.transform(glm::mat4_cast(upAxisZRotation));
        hfmModelPtr->bindExtents.transform(glm::mat4_cast(upAxisZRotation));
        for (auto &mesh : hfmModelPtr->meshes) {
            mesh.modelTransform *= glm::mat4_cast(upAxisZRotation);
            mesh.meshExtents.transform(glm::mat4_cast(upAxisZRotation));
        }
    }
    return hfmModelPtr;
}

MediaType FBXSerializer::getMediaType() const {
    MediaType mediaType("fbx");
    mediaType.extensions.push_back("fbx");
    mediaType.fileSignatures.emplace_back("Kaydara FBX Binary  \x00", 0);
    return mediaType;
}

std::unique_ptr<hfm::Serializer::Factory> FBXSerializer::getFactory() const {
    return std::make_unique<hfm::Serializer::SimpleFactory<FBXSerializer>>();
}

HFMModel::Pointer FBXSerializer::read(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url) {
    QBuffer buffer(const_cast<hifi::ByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);

    _rootNode = parseFBX(&buffer);

    // FBXSerializer's mapping parameter supports the bool "deduplicateIndices," which is passed into FBXSerializer::extractMesh as "deduplicate"

    auto hfmModel = extractHFMModel(mapping, url.toString());

    //hfmModel->debugDump();

    return HFMModel::Pointer(hfmModel);
}
