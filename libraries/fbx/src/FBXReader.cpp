//
//  FBXReader.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QStringList>
#include <QTextStream>
#include <QtDebug>
#include <QtEndian>
#include <QFileInfo>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <FaceshiftConstants.h>
#include <GeometryUtil.h>
#include <GLMHelpers.h>
#include <NumericalConstants.h>
#include <OctalCode.h>
#include <gpu/Format.h>
#include <LogHandler.h>

#include "FBXReader.h"
#include "ModelFormatLogging.h"

// TOOL: Uncomment the following line to enable the filtering of all the unkwnon fields of a node so we can break point easily while loading a model with problems...
//#define DEBUG_FBXREADER

using namespace std;

int FBXGeometryPointerMetaTypeId = qRegisterMetaType<FBXGeometry::Pointer>();

QStringList FBXGeometry::getJointNames() const {
    QStringList names;
    foreach (const FBXJoint& joint, joints) {
        names.append(joint.name);
    }
    return names;
}

bool FBXGeometry::hasBlendedMeshes() const {
    if (!meshes.isEmpty()) {
        foreach (const FBXMesh& mesh, meshes) {
            if (!mesh.blendshapes.isEmpty()) {
                return true;
            }
        }
    }
    return false;
}

Extents FBXGeometry::getUnscaledMeshExtents() const {
    const Extents& extents = meshExtents;

    // even though our caller asked for "unscaled" we need to include any fst scaling, translation, and rotation, which
    // is captured in the offset matrix
    glm::vec3 minimum = glm::vec3(offset * glm::vec4(extents.minimum, 1.0f));
    glm::vec3 maximum = glm::vec3(offset * glm::vec4(extents.maximum, 1.0f));
    Extents scaledExtents = { minimum, maximum };

    return scaledExtents;
}

// TODO: Move to model::Mesh when Sam's ready
bool FBXGeometry::convexHullContains(const glm::vec3& point) const {
    if (!getUnscaledMeshExtents().containsPoint(point)) {
        return false;
    }

    auto checkEachPrimitive = [=](FBXMesh& mesh, QVector<int> indices, int primitiveSize) -> bool {
        // Check whether the point is "behind" all the primitives.
        int verticesSize = mesh.vertices.size();
        for (int j = 0;
             j < indices.size() - 2; // -2 in case the vertices aren't the right size -- we access j + 2 below
             j += primitiveSize) {
            if (indices[j] < verticesSize &&
                indices[j + 1] < verticesSize &&
                indices[j + 2] < verticesSize &&
                !isPointBehindTrianglesPlane(point,
                                             mesh.vertices[indices[j]],
                                             mesh.vertices[indices[j + 1]],
                                             mesh.vertices[indices[j + 2]])) {
                // it's not behind at least one so we bail
                return false;
            }
        }
        return true;
    };

    // Check that the point is contained in at least one convex mesh.
    for (auto mesh : meshes) {
        bool insideMesh = true;

        // To be considered inside a convex mesh,
        // the point needs to be "behind" all the primitives respective planes.
        for (auto part : mesh.parts) {
            // run through all the triangles and quads
            if (!checkEachPrimitive(mesh, part.triangleIndices, 3) ||
                !checkEachPrimitive(mesh, part.quadIndices, 4)) {
                // If not, the point is outside, bail for this mesh
                insideMesh = false;
                continue;
            }
        }
        if (insideMesh) {
            // It's inside this mesh, return true.
            return true;
        }
    }

    // It wasn't in any mesh, return false.
    return false;
}

QString FBXGeometry::getModelNameOfMesh(int meshIndex) const {
    if (meshIndicesToModelNames.contains(meshIndex)) {
        return meshIndicesToModelNames.value(meshIndex);
    }
    return QString();
}

int fbxGeometryMetaTypeId = qRegisterMetaType<FBXGeometry>();
int fbxAnimationFrameMetaTypeId = qRegisterMetaType<FBXAnimationFrame>();
int fbxAnimationFrameVectorMetaTypeId = qRegisterMetaType<QVector<FBXAnimationFrame> >();


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

QString processID(const QString& id) {
    // Blender (at least) prepends a type to the ID, so strip it out
    return id.mid(id.lastIndexOf(':') + 1);
}

QString getName(const QVariantList& properties) {
    QString name;
    if (properties.size() == 3) {
        name = properties.at(1).toString();
        name = processID(name.left(name.indexOf(QChar('\0'))));
    } else {
        name = processID(properties.at(0).toString());
    }
    return name;
}

QString getID(const QVariantList& properties, int index = 0) {
    return processID(properties.at(index).toString());
}

const char* HUMANIK_JOINTS[] = {
    "RightHand",
    "RightForeArm",
    "RightArm",
    "Head",
    "LeftArm",
    "LeftForeArm",
    "LeftHand",
    "Neck",
    "Spine",
    "Hips",
    "RightUpLeg",
    "LeftUpLeg",
    "RightLeg",
    "LeftLeg",
    "RightFoot",
    "LeftFoot",
    ""
};

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
};

glm::mat4 getGlobalTransform(const QMultiMap<QString, QString>& _connectionParentMap,
        const QHash<QString, FBXModel>& models, QString nodeID, bool mixamoHack) {
    glm::mat4 globalTransform;
    while (!nodeID.isNull()) {
        const FBXModel& model = models.value(nodeID);
        globalTransform = glm::translate(model.translation) * model.preTransform * glm::mat4_cast(model.preRotation *
            model.rotation * model.postRotation) * model.postTransform * globalTransform;
        if (mixamoHack) {
            // there's something weird about the models from Mixamo Fuse; they don't skin right with the full transform
            return globalTransform;
        }
        QList<QString> parentIDs = _connectionParentMap.values(nodeID);
        nodeID = QString();
        foreach (const QString& parentID, parentIDs) {
            if (models.contains(parentID)) {
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
    FBXBlendshape blendshape;
};

void printNode(const FBXNode& node, int indentLevel) {
    int indentLength = 2;
    QByteArray spaces(indentLevel * indentLength, ' ');
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
        QHash<QString, FBXModel>& models, QSet<QString>& remainingModels, QVector<QString>& modelIDs) {
    if (remainingModels.contains(parentID)) {
        modelIDs.append(parentID);
        remainingModels.remove(parentID);
    }
    int parentIndex = modelIDs.size() - 1;
    foreach (const QString& childID, connectionChildMap.values(parentID)) {
        if (remainingModels.contains(childID)) {
            FBXModel& model = models[childID];
            if (model.parentIndex == -1) {
                model.parentIndex = parentIndex;
                appendModelIDs(childID, connectionChildMap, models, remainingModels, modelIDs);
            }
        }
    }
}

FBXBlendshape extractBlendshape(const FBXNode& object) {
    FBXBlendshape blendshape;
    foreach (const FBXNode& data, object.children) {
        if (data.name == "Indexes") {
            blendshape.indices = FBXReader::getIntVector(data);

        } else if (data.name == "Vertices") {
            blendshape.vertices = FBXReader::createVec3Vector(FBXReader::getDoubleVector(data));

        } else if (data.name == "Normals") {
            blendshape.normals = FBXReader::createVec3Vector(FBXReader::getDoubleVector(data));
        }
    }
    return blendshape;
}


void setTangents(FBXMesh& mesh, int firstIndex, int secondIndex) {
    const glm::vec3& normal = mesh.normals.at(firstIndex);
    glm::vec3 bitangent = glm::cross(normal, mesh.vertices.at(secondIndex) - mesh.vertices.at(firstIndex));
    if (glm::length(bitangent) < EPSILON) {
        return;
    }
    glm::vec2 texCoordDelta = mesh.texCoords.at(secondIndex) - mesh.texCoords.at(firstIndex);
    glm::vec3 normalizedNormal = glm::normalize(normal);
    mesh.tangents[firstIndex] += glm::cross(glm::angleAxis(-atan2f(-texCoordDelta.t, texCoordDelta.s), normalizedNormal) *
        glm::normalize(bitangent), normalizedNormal);
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
        FBXBlendshape& blendshape = extractedMesh.mesh.blendshapes[index.first];
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
        const QHash<QString, FBXModel>& models, const QString& modelID) {
    QString topID = modelID;
    forever {
        foreach (const QString& parentID, connectionParentMap.values(topID)) {
            if (models.contains(parentID)) {
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

typedef std::vector<glm::vec3> ShapeVertices;

class AnimationCurve {
public:
    QVector<float> values;
};

bool checkMaterialsHaveTextures(const QHash<QString, FBXMaterial>& materials,
        const QHash<QString, QByteArray>& textureFilenames, const QMultiMap<QString, QString>& _connectionChildMap) {
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


FBXLight extractLight(const FBXNode& object) {
    FBXLight light;
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
                        light.color = FBXReader::getVec3(property.properties, valIndex);
                    }
                }
            }
        } else if ( subobject.name == "GeometryVersion"
                   || subobject.name == "TypeFlags") {
        }
    }
#if defined(DEBUG_FBXREADER)

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

QByteArray fileOnUrl(const QByteArray& filepath, const QString& url) {
    QString path = QFileInfo(url).path();
    QByteArray filename = filepath;
    QFileInfo checkFile(path + "/" + filepath);

    // check if the file exists at the RelativeFilename
    if (!(checkFile.exists() && checkFile.isFile())) {
        // if not, assume it is in the fbx directory
        filename = filename.mid(filename.lastIndexOf('/') + 1);
    }

    return filename;
}

FBXGeometry* FBXReader::extractFBXGeometry(const QVariantHash& mapping, const QString& url) {
    const FBXNode& node = _fbxNode;
    QMap<QString, ExtractedMesh> meshes;
    QHash<QString, QString> modelIDsToNames;
    QHash<QString, int> meshIDsToMeshIndices;
    QHash<QString, QString> ooChildToParent;

    QVector<ExtractedBlendshape> blendshapes;

    QHash<QString, FBXModel> models;
    QHash<QString, Cluster> clusters; 
    QHash<QString, AnimationCurve> animationCurves;

    QHash<QString, QString> typeFlags;

    QHash<QString, QString> localRotations;
    QHash<QString, QString> localTranslations;
    QHash<QString, QString> xComponents;
    QHash<QString, QString> yComponents;
    QHash<QString, QString> zComponents;

    std::map<QString, FBXLight> lights;

    QVariantHash joints = mapping.value("joint").toHash();
    QString jointEyeLeftName = processID(getString(joints.value("jointEyeLeft", "jointEyeLeft")));
    QString jointEyeRightName = processID(getString(joints.value("jointEyeRight", "jointEyeRight")));
    QString jointNeckName = processID(getString(joints.value("jointNeck", "jointNeck")));
    QString jointRootName = processID(getString(joints.value("jointRoot", "jointRoot")));
    QString jointLeanName = processID(getString(joints.value("jointLean", "jointLean")));
    QString jointHeadName = processID(getString(joints.value("jointHead", "jointHead")));
    QString jointLeftHandName = processID(getString(joints.value("jointLeftHand", "jointLeftHand")));
    QString jointRightHandName = processID(getString(joints.value("jointRightHand", "jointRightHand")));
    QString jointEyeLeftID;
    QString jointEyeRightID;
    QString jointNeckID;
    QString jointRootID;
    QString jointLeanID;
    QString jointHeadID;
    QString jointLeftHandID;
    QString jointRightHandID;
    QString jointLeftToeID;
    QString jointRightToeID;


    QVector<QString> humanIKJointNames;
    for (int i = 0;; i++) {
        QByteArray jointName = HUMANIK_JOINTS[i];
        if (jointName.isEmpty()) {
            break;
        }
        humanIKJointNames.append(processID(getString(joints.value(jointName, jointName))));
    }
    QVector<QString> humanIKJointIDs(humanIKJointNames.size());

    QVariantHash blendshapeMappings = mapping.value("bs").toHash();

    QMultiHash<QByteArray, WeightedIndex> blendshapeIndices;
    for (int i = 0;; i++) {
        QByteArray blendshapeName = FACESHIFT_BLENDSHAPES[i];
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
#if defined(DEBUG_FBXREADER)
    int unknown = 0;
#endif
    FBXGeometry* geometryPtr = new FBXGeometry;
    FBXGeometry& geometry = *geometryPtr;

    float unitScaleFactor = 1.0f;
    glm::vec3 ambientColor;
    QString hifiGlobalNodeID;
    unsigned int meshIndex = 0;
    foreach (const FBXNode& child, node.children) {

        if (child.name == "FBXHeaderExtension") {
            foreach (const FBXNode& object, child.children) {
                if (object.name == "SceneInfo") {
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "MetaData") {
                            foreach (const FBXNode& subsubobject, subobject.children) {
                                if (subsubobject.name == "Author") {
                                    geometry.author = subsubobject.properties.at(0).toString();
                                }
                            }
                        } else if (subobject.name == "Properties70") {
                            foreach (const FBXNode& subsubobject, subobject.children) {
                                if (subsubobject.name == "P" && subsubobject.properties.size() >= 5 &&
                                        subsubobject.properties.at(0) == "Original|ApplicationName") {
                                    geometry.applicationName = subsubobject.properties.at(4).toString();
                                }
                            }
                        }
                    }
                }
            }
        } else if (child.name == "GlobalSettings") {
            foreach (const FBXNode& object, child.children) {
                if (object.name == "Properties70") {
                    QString propertyName = "P";
                    int index = 4;
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == propertyName) {
                            QString subpropName = subobject.properties.at(0).toString();
                            if (subpropName == "UnitScaleFactor") {
                                unitScaleFactor = subobject.properties.at(index).toFloat();
                            } else if (subpropName == "AmbientColor") {
                                ambientColor = getVec3(subobject.properties, index);
                            }
                        }
                    }
                }
            }
        } else if (child.name == "Objects") {
            foreach (const FBXNode& object, child.children) {
                if (object.name == "Geometry") {
                    if (object.properties.at(2) == "Mesh") {
                        meshes.insert(getID(object.properties), extractMesh(object, meshIndex));
                    } else { // object.properties.at(2) == "Shape"
                        ExtractedBlendshape extracted = { getID(object.properties), extractBlendshape(object) };
                        blendshapes.append(extracted);
                    }
                } else if (object.name == "Model") {
                    QString name = getName(object.properties);
                    QString id = getID(object.properties);
                    modelIDsToNames.insert(id, name);

                    QString modelname = name.toLower();
                    if (modelname.startsWith("hifi")) {
                        hifiGlobalNodeID = id;
                    }

                    if (name == jointEyeLeftName || name == "EyeL" || name == "joint_Leye") {
                        jointEyeLeftID = getID(object.properties);

                    } else if (name == jointEyeRightName || name == "EyeR" || name == "joint_Reye") {
                        jointEyeRightID = getID(object.properties);

                    } else if (name == jointNeckName || name == "NeckRot" || name == "joint_neck") {
                        jointNeckID = getID(object.properties);

                    } else if (name == jointRootName) {
                        jointRootID = getID(object.properties);

                    } else if (name == jointLeanName) {
                        jointLeanID = getID(object.properties);

                    } else if (name == jointHeadName) {
                        jointHeadID = getID(object.properties);

                    } else if (name == jointLeftHandName || name == "LeftHand" || name == "joint_L_hand") {
                        jointLeftHandID = getID(object.properties);

                    } else if (name == jointRightHandName || name == "RightHand" || name == "joint_R_hand") {
                        jointRightHandID = getID(object.properties);

                    } else if (name == "LeftToe" || name == "joint_L_toe" || name == "LeftToe_End") {
                        jointLeftToeID = getID(object.properties);

                    } else if (name == "RightToe" || name == "joint_R_toe" || name == "RightToe_End") {
                        jointRightToeID = getID(object.properties);
                    }

                    int humanIKJointIndex = humanIKJointNames.indexOf(name);
                    if (humanIKJointIndex != -1) {
                        humanIKJointIDs[humanIKJointIndex] = getID(object.properties);
                    }

                    glm::vec3 translation;
                    // NOTE: the euler angles as supplied by the FBX file are in degrees
                    glm::vec3 rotationOffset;
                    glm::vec3 preRotation, rotation, postRotation;
                    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
                    glm::vec3 scalePivot, rotationPivot, scaleOffset;
                    bool rotationMinX = false, rotationMinY = false, rotationMinZ = false;
                    bool rotationMaxX = false, rotationMaxY = false, rotationMaxZ = false;
                    glm::vec3 rotationMin, rotationMax;
                    FBXModel model = { name, -1, glm::vec3(), glm::mat4(), glm::quat(), glm::quat(), glm::quat(),
                                       glm::mat4(), glm::vec3(), glm::vec3()};
                    ExtractedMesh* mesh = NULL;
                    QVector<ExtractedBlendshape> blendshapes;
                    foreach (const FBXNode& subobject, object.children) {
                        bool properties = false;
                        QByteArray propertyName;
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
                            foreach (const FBXNode& property, subobject.children) {
                                if (property.name == propertyName) {
                                    if (property.properties.at(0) == "Lcl Translation") {
                                        translation = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "RotationOffset") {
                                        rotationOffset = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "RotationPivot") {
                                        rotationPivot = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "PreRotation") {
                                        preRotation = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "Lcl Rotation") {
                                        rotation = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "PostRotation") {
                                        postRotation = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "ScalingPivot") {
                                        scalePivot = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "Lcl Scaling") {
                                        scale = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "ScalingOffset") {
                                        scaleOffset = getVec3(property.properties, index);

                                    // NOTE: these rotation limits are stored in degrees (NOT radians)
                                    } else if (property.properties.at(0) == "RotationMin") {
                                        rotationMin = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "RotationMax") {
                                        rotationMax = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "RotationMinX") {
                                        rotationMinX = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMinY") {
                                        rotationMinY = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMinZ") {
                                        rotationMinZ = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMaxX") {
                                        rotationMaxX = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMaxY") {
                                        rotationMaxY = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMaxZ") {
                                        rotationMaxZ = property.properties.at(index).toBool();
                                    }
                                }
                            }
                        } else if (subobject.name == "Vertices") {
                            // it's a mesh as well as a model
                            mesh = &meshes[getID(object.properties)];
                            *mesh = extractMesh(object, meshIndex);

                        } else if (subobject.name == "Shape") {
                            ExtractedBlendshape blendshape =  { subobject.properties.at(0).toString(),
                                extractBlendshape(subobject) };
                            blendshapes.append(blendshape);
                        }
#if defined(DEBUG_FBXREADER)
                        else if (subobject.name == "TypeFlags") {
                            QString attributetype = subobject.properties.at(0).toString();
                            if (!attributetype.empty()) {
                                if (attributetype == "Light") {
                                    QString lightprop;
                                    foreach (const QVariant& vprop, subobject.properties) {
                                        lightprop = vprop.toString();
                                    }

                                    FBXLight light = extractLight(object);
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
                    model.translation = translation;

                    model.preTransform = glm::translate(rotationOffset) * glm::translate(rotationPivot);
                    model.preRotation = glm::quat(glm::radians(preRotation));
                    model.rotation = glm::quat(glm::radians(rotation));
                    model.postRotation = glm::inverse(glm::quat(glm::radians(postRotation)));
                    model.postTransform = glm::translate(-rotationPivot) * glm::translate(scaleOffset) *
                        glm::translate(scalePivot) * glm::scale(scale) * glm::translate(-scalePivot);
                    // NOTE: angles from the FBX file are in degrees
                    // so we convert them to radians for the FBXModel class
                    model.rotationMin = glm::radians(glm::vec3(rotationMinX ? rotationMin.x : -180.0f,
                        rotationMinY ? rotationMin.y : -180.0f, rotationMinZ ? rotationMin.z : -180.0f));
                    model.rotationMax = glm::radians(glm::vec3(rotationMaxX ? rotationMax.x : 180.0f,
                        rotationMaxY ? rotationMax.y : 180.0f, rotationMaxZ ? rotationMax.z : 180.0f));
                    models.insert(getID(object.properties), model);

                } else if (object.name == "Texture") {
                    TextureParam tex;
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "RelativeFilename") {
                            QByteArray filename = subobject.properties.at(0).toByteArray();
                            QByteArray filepath = filename.replace('\\', '/');
                            filename = fileOnUrl(filepath, url);
                            _textureFilepaths.insert(getID(object.properties), filepath);
                            _textureFilenames.insert(getID(object.properties), filename);
                        } else if (subobject.name == "TextureName") {
                            // trim the name from the timestamp
                            QString name = QString(subobject.properties.at(0).toByteArray());
                            name = name.left(name.indexOf('['));
                            _textureNames.insert(getID(object.properties), name);
                        } else if (subobject.name == "Texture_Alpha_Source") {
                            tex.assign<uint8_t>(tex.alphaSource, subobject.properties.at(0).value<int>());
                        } else if (subobject.name == "ModelUVTranslation") {
                            tex.assign(tex.UVTranslation, glm::vec2(subobject.properties.at(0).value<double>(),
                                                                subobject.properties.at(1).value<double>()));
                        } else if (subobject.name == "ModelUVScaling") {
                            tex.assign(tex.UVScaling, glm::vec2(subobject.properties.at(0).value<double>(),
                                                                subobject.properties.at(1).value<double>()));
                            if (tex.UVScaling.x == 0.0f) {
                                tex.UVScaling.x = 1.0f;
                            }
                            if (tex.UVScaling.y == 0.0f) {
                                tex.UVScaling.y = 1.0f;
                            }
                        } else if (subobject.name == "Cropping") {
                            tex.assign(tex.cropping, glm::vec4(subobject.properties.at(0).value<int>(),
                                                                subobject.properties.at(1).value<int>(),
                                                                subobject.properties.at(2).value<int>(),
                                                                subobject.properties.at(3).value<int>()));
                        } else if (subobject.name == "Properties70") {
                            QByteArray propertyName;
                            int index;
                                propertyName = "P";
                                index = 4;
                                foreach (const FBXNode& property, subobject.children) {
                                    if (property.name == propertyName) {
                                        QString v = property.properties.at(0).toString();
                                        if (property.properties.at(0) == "UVSet") {
                                            std::string uvName = property.properties.at(index).toString().toStdString();
                                            tex.assign(tex.UVSet, property.properties.at(index).toString());
                                        } else if (property.properties.at(0) == "CurrentTextureBlendMode") {
                                            tex.assign<uint8_t>(tex.currentTextureBlendMode, property.properties.at(index).value<int>());
                                        } else if (property.properties.at(0) == "UseMaterial") {
                                            tex.assign<bool>(tex.useMaterial, property.properties.at(index).value<int>());
                                        } else if (property.properties.at(0) == "Translation") {
                                            tex.assign(tex.translation, getVec3(property.properties, index));
                                        } else if (property.properties.at(0) == "Rotation") {
                                            tex.assign(tex.rotation, getVec3(property.properties, index));
                                        } else if (property.properties.at(0) == "Scaling") {
                                            tex.assign(tex.scaling, getVec3(property.properties, index));
                                            if (tex.scaling.x == 0.0f) {
                                                tex.scaling.x = 1.0f;
                                            }
                                            if (tex.scaling.y == 0.0f) {
                                                tex.scaling.y = 1.0f;
                                            }
                                            if (tex.scaling.z == 0.0f) {
                                                tex.scaling.z = 1.0f;
                                            }
                                        }
#if defined(DEBUG_FBXREADER)
                                        else {
                                            QString propName = v;
                                            unknown++;
                                        }
#endif
                                    }
                                }
                        }
#if defined(DEBUG_FBXREADER)
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
                    QByteArray filepath;
                    QByteArray content;
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "RelativeFilename") {
                            filepath= subobject.properties.at(0).toByteArray();
                            filepath = filepath.replace('\\', '/');

                        } else if (subobject.name == "Content" && !subobject.properties.isEmpty()) {
                            content = subobject.properties.at(0).toByteArray();
                        }
                    }
                    if (!content.isEmpty()) {
                        _textureContent.insert(filepath, content);
                    }
                } else if (object.name == "Material") {
                    FBXMaterial material;
                    material.name = (object.properties.at(1).toString());
                    foreach (const FBXNode& subobject, object.children) {
                        bool properties = false;

                        QByteArray propertyName;
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
                            foreach(const FBXNode& property, subobject.children) {
                                if (property.name == propertyName) {
                                    if (property.properties.at(0) == "DiffuseColor") {
                                        material.diffuseColor = getVec3(property.properties, index);
                                    } else if (property.properties.at(0) == "DiffuseFactor") {
                                        material.diffuseFactor = property.properties.at(index).value<double>();
                                    } else if (property.properties.at(0) == "Diffuse") {
                                        // NOTE: this is uneeded but keep it for now for debug
                                        //  material.diffuseColor = getVec3(property.properties, index);
                                        //  material.diffuseFactor = 1.0;

                                    } else if (property.properties.at(0) == "SpecularColor") {
                                        material.specularColor = getVec3(property.properties, index);
                                    } else if (property.properties.at(0) == "SpecularFactor") {
                                        material.specularFactor = property.properties.at(index).value<double>();
                                    } else if (property.properties.at(0) == "Specular") {
                                        // NOTE: this is uneeded but keep it for now for debug
                                        //  material.specularColor = getVec3(property.properties, index);
                                        //  material.specularFactor = 1.0;

                                    } else if (property.properties.at(0) == "EmissiveColor") {
                                        material.emissiveColor = getVec3(property.properties, index);
                                    } else if (property.properties.at(0) == "EmissiveFactor") {
                                        material.emissiveFactor = property.properties.at(index).value<double>();
                                    } else if (property.properties.at(0) == "Emissive") {
                                        // NOTE: this is uneeded but keep it for now for debug
                                        //  material.emissiveColor = getVec3(property.properties, index);
                                        //  material.emissiveFactor = 1.0;

                                    } else if (property.properties.at(0) == "AmbientFactor") {
                                        material.ambientFactor = property.properties.at(index).value<double>();
                                        // Detected just for BLender AO vs lightmap
                                    } else if (property.properties.at(0) == "Shininess") {
                                        material.shininess = property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == "Opacity") {
                                        material.opacity = property.properties.at(index).value<double>();
                                    }

                                    // Sting Ray Material Properties!!!!
                                    else if (property.properties.at(0) == "Maya|use_normal_map") {
                                        material.isPBSMaterial = true;
                                        material.useNormalMap = (bool)property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == "Maya|base_color") {
                                        material.isPBSMaterial = true;
                                        material.diffuseColor = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "Maya|use_color_map") {
                                        material.isPBSMaterial = true;
                                        material.useAlbedoMap = (bool) property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == "Maya|roughness") {
                                        material.isPBSMaterial = true;
                                        material.roughness = property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == "Maya|use_roughness_map") {
                                        material.isPBSMaterial = true;
                                        material.useRoughnessMap = (bool)property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == "Maya|metallic") {
                                        material.isPBSMaterial = true;
                                        material.metallic = property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == "Maya|use_metallic_map") {
                                        material.isPBSMaterial = true;
                                        material.useMetallicMap = (bool)property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == "Maya|emissive") {
                                        material.isPBSMaterial = true;
                                        material.emissiveColor = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "Maya|emissive_intensity") {
                                        material.isPBSMaterial = true;
                                        material.emissiveIntensity = property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == "Maya|use_emissive_map") {
                                        material.isPBSMaterial = true;
                                        material.useEmissiveMap = (bool)property.properties.at(index).value<double>();

                                    } else if (property.properties.at(0) == "Maya|use_ao_map") {
                                        material.isPBSMaterial = true;
                                        material.useOcclusionMap = (bool)property.properties.at(index).value<double>();

                                    } else {
                                        const QString propname = property.properties.at(0).toString();
                                        unknowns.push_back(propname.toStdString());
                                    }
                                }
                            }
                        }
#if defined(DEBUG_FBXREADER)
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
                    _fbxMaterials.insert(material.materialID, material);


                } else if (object.name == "NodeAttribute") {
#if defined(DEBUG_FBXREADER)
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
                            FBXLight light = extractLight(object);
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
                        clusters.insert(getID(object.properties), cluster);

                    } else if (object.properties.last() == "BlendShapeChannel") {
                        QByteArray name = object.properties.at(1).toByteArray();

                        name = name.left(name.indexOf('\0'));
                        if (!blendshapeIndices.contains(name)) {
                            // try everything after the dot
                            name = name.mid(name.lastIndexOf('.') + 1);
                        }
                        QString id = getID(object.properties);
                        geometry.blendshapeChannelNames << name;
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
#if defined(DEBUG_FBXREADER)
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
            foreach (const FBXNode& connection, child.children) {
                if (connection.name == "C" || connection.name == "Connect") {
                    if (connection.properties.at(0) == "OO") {
                        QString childID = getID(connection.properties, 1);
                        QString parentID = getID(connection.properties, 2);
                        ooChildToParent.insert(childID, parentID);
                        if (!hifiGlobalNodeID.isEmpty() && (parentID == hifiGlobalNodeID)) {
                            std::map< QString, FBXLight >::iterator lightIt = lights.find(childID);
                            if (lightIt != lights.end()) {
                                _lightmapLevel = (*lightIt).second.intensity;
                                if (_lightmapLevel <= 0.0f) {
                                    _loadLightmaps = false;
                                }
                                _lightmapOffset = glm::clamp((*lightIt).second.color.x, 0.f, 1.f);
                            }
                        }
                    }
                    if (connection.properties.at(0) == "OP") {
                        int counter = 0;
                        QByteArray type = connection.properties.at(3).toByteArray().toLower();
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
                    _connectionParentMap.insert(getID(connection.properties, 1), getID(connection.properties, 2));
                    _connectionChildMap.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                }
            }
        }
#if defined(DEBUG_FBXREADER)
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
    geometry.offset = glm::translate(glm::vec3(mapping.value("tx").toFloat(), mapping.value("ty").toFloat(),
        mapping.value("tz").toFloat())) * glm::mat4_cast(offsetRotation) *
            glm::scale(glm::vec3(offsetScale, offsetScale, offsetScale));

    // get the list of models in depth-first traversal order
    QVector<QString> modelIDs;
    QSet<QString> remainingModels;
    for (QHash<QString, FBXModel>::const_iterator model = models.constBegin(); model != models.constEnd(); model++) {
        // models with clusters must be parented to the cluster top
        foreach (const QString& deformerID, _connectionChildMap.values(model.key())) {
            foreach (const QString& clusterID, _connectionChildMap.values(deformerID)) {
                if (!clusters.contains(clusterID)) {
                    continue;
                }
                QString topID = getTopModelID(_connectionParentMap, models, _connectionChildMap.value(clusterID));
                _connectionChildMap.remove(_connectionParentMap.take(model.key()), model.key());
                _connectionParentMap.insert(model.key(), topID);
                goto outerBreak;
            }
        }
        outerBreak:

        // make sure the parent is in the child map
        QString parent = _connectionParentMap.value(model.key());
        if (!_connectionChildMap.contains(parent, model.key())) {
            _connectionChildMap.insert(parent, model.key());
        }
        remainingModels.insert(model.key());
    }
    while (!remainingModels.isEmpty()) {
        QString first = *remainingModels.constBegin();
        foreach (const QString& id, remainingModels) {
            if (id < first) {
                first = id;
            }
        }
        QString topID = getTopModelID(_connectionParentMap, models, first);
        appendModelIDs(_connectionParentMap.value(topID), _connectionChildMap, models, remainingModels, modelIDs);
    }

    // figure the number of animation frames from the curves
    int frameCount = 1;
    foreach (const AnimationCurve& curve, animationCurves) {
        frameCount = qMax(frameCount, curve.values.size());
    }
    for (int i = 0; i < frameCount; i++) {
        FBXAnimationFrame frame;
        frame.rotations.resize(modelIDs.size());
        frame.translations.resize(modelIDs.size());
        geometry.animationFrames.append(frame);
    }

    // convert the models to joints
    QVariantList freeJoints = mapping.values("freeJoint");
    geometry.hasSkeletonJoints = false;
    foreach (const QString& modelID, modelIDs) {
        const FBXModel& model = models[modelID];
        FBXJoint joint;
        joint.isFree = freeJoints.contains(model.name);
        joint.parentIndex = model.parentIndex;

        // get the indices of all ancestors starting with the first free one (if any)
        int jointIndex = geometry.joints.size();
        joint.freeLineage.append(jointIndex);
        int lastFreeIndex = joint.isFree ? 0 : -1;
        for (int index = joint.parentIndex; index != -1; index = geometry.joints.at(index).parentIndex) {
            if (geometry.joints.at(index).isFree) {
                lastFreeIndex = joint.freeLineage.size();
            }
            joint.freeLineage.append(index);
        }
        joint.freeLineage.remove(lastFreeIndex + 1, joint.freeLineage.size() - lastFreeIndex - 1);
        joint.translation = model.translation; // these are usually in centimeters
        joint.preTransform = model.preTransform;
        joint.preRotation = model.preRotation;
        joint.rotation = model.rotation;
        joint.postRotation = model.postRotation;
        joint.postTransform = model.postTransform;
        joint.rotationMin = model.rotationMin;
        joint.rotationMax = model.rotationMax;
        glm::quat combinedRotation = joint.preRotation * joint.rotation * joint.postRotation;
        if (joint.parentIndex == -1) {
            joint.transform = geometry.offset * glm::translate(joint.translation) * joint.preTransform *
                glm::mat4_cast(combinedRotation) * joint.postTransform;
            joint.inverseDefaultRotation = glm::inverse(combinedRotation);
           joint.distanceToParent = 0.0f;

        } else {
            const FBXJoint& parentJoint = geometry.joints.at(joint.parentIndex);
            joint.transform = parentJoint.transform * glm::translate(joint.translation) *
                joint.preTransform * glm::mat4_cast(combinedRotation) * joint.postTransform;
            joint.inverseDefaultRotation = glm::inverse(combinedRotation) * parentJoint.inverseDefaultRotation;
            joint.distanceToParent = glm::distance(extractTranslation(parentJoint.transform),
                extractTranslation(joint.transform));
        }
        joint.inverseBindRotation = joint.inverseDefaultRotation;
        joint.name = model.name;

        foreach (const QString& childID, _connectionChildMap.values(modelID)) {
            QString type = typeFlags.value(childID);
            if (!type.isEmpty()) {
                geometry.hasSkeletonJoints |= (joint.isSkeletonJoint = type.toLower().contains("Skeleton"));
                break;
            }
        }

        joint.bindTransformFoundInCluster = false;

        geometry.joints.append(joint);
        geometry.jointIndices.insert(model.name, geometry.joints.size());

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
            geometry.animationFrames[i].rotations[jointIndex] = glm::quat(glm::radians(glm::vec3(
                xRotCurve.values.isEmpty() ? defaultRotValues.x : xRotCurve.values.at(i % xRotCurve.values.size()),
                yRotCurve.values.isEmpty() ? defaultRotValues.y : yRotCurve.values.at(i % yRotCurve.values.size()),
                zRotCurve.values.isEmpty() ? defaultRotValues.z : zRotCurve.values.at(i % zRotCurve.values.size()))));
            geometry.animationFrames[i].translations[jointIndex] = glm::vec3(
                xPosCurve.values.isEmpty() ? defaultPosValues.x : xPosCurve.values.at(i % xPosCurve.values.size()),
                yPosCurve.values.isEmpty() ? defaultPosValues.y : yPosCurve.values.at(i % yPosCurve.values.size()),
                zPosCurve.values.isEmpty() ? defaultPosValues.z : zPosCurve.values.at(i % zPosCurve.values.size()));
        }
    }

    // NOTE: shapeVertices are in joint-frame
    std::vector<ShapeVertices> shapeVertices;
    shapeVertices.resize(geometry.joints.size());

    // find our special joints
    geometry.leftEyeJointIndex = modelIDs.indexOf(jointEyeLeftID);
    geometry.rightEyeJointIndex = modelIDs.indexOf(jointEyeRightID);
    geometry.neckJointIndex = modelIDs.indexOf(jointNeckID);
    geometry.rootJointIndex = modelIDs.indexOf(jointRootID);
    geometry.leanJointIndex = modelIDs.indexOf(jointLeanID);
    geometry.headJointIndex = modelIDs.indexOf(jointHeadID);
    geometry.leftHandJointIndex = modelIDs.indexOf(jointLeftHandID);
    geometry.rightHandJointIndex = modelIDs.indexOf(jointRightHandID);
    geometry.leftToeJointIndex = modelIDs.indexOf(jointLeftToeID);
    geometry.rightToeJointIndex = modelIDs.indexOf(jointRightToeID);

    foreach (const QString& id, humanIKJointIDs) {
        geometry.humanIKJointIndices.append(modelIDs.indexOf(id));
    }

    // extract the translation component of the neck transform
    if (geometry.neckJointIndex != -1) {
        const glm::mat4& transform = geometry.joints.at(geometry.neckJointIndex).transform;
        geometry.neckPivot = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
    }

    geometry.bindExtents.reset();
    geometry.meshExtents.reset();

    // Create the Material Library
    consolidateFBXMaterials(mapping);
    geometry.materials = _fbxMaterials;

    // see if any materials have texture children
    bool materialsHaveTextures = checkMaterialsHaveTextures(_fbxMaterials, _textureFilenames, _connectionChildMap);

    for (QMap<QString, ExtractedMesh>::iterator it = meshes.begin(); it != meshes.end(); it++) {
        ExtractedMesh& extracted = it.value();

        extracted.mesh.meshExtents.reset();

        // accumulate local transforms
        QString modelID = models.contains(it.key()) ? it.key() : _connectionParentMap.value(it.key());
        glm::mat4 modelTransform = getGlobalTransform(_connectionParentMap, models, modelID, geometry.applicationName == "mixamo.com");

        // compute the mesh extents from the transformed vertices
        foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
            glm::vec3 transformedVertex = glm::vec3(modelTransform * glm::vec4(vertex, 1.0f));
            geometry.meshExtents.minimum = glm::min(geometry.meshExtents.minimum, transformedVertex);
            geometry.meshExtents.maximum = glm::max(geometry.meshExtents.maximum, transformedVertex);

            extracted.mesh.meshExtents.minimum = glm::min(extracted.mesh.meshExtents.minimum, transformedVertex);
            extracted.mesh.meshExtents.maximum = glm::max(extracted.mesh.meshExtents.maximum, transformedVertex);
            extracted.mesh.modelTransform = modelTransform;
        }

        // look for textures, material properties
        // allocate the Part material library
        int materialIndex = 0;
        int textureIndex = 0;
        bool generateTangents = false;
        QList<QString> children = _connectionChildMap.values(modelID);
        for (int i = children.size() - 1; i >= 0; i--) {

            const QString& childID = children.at(i);
            if (_fbxMaterials.contains(childID)) {
                // the pure material associated with this part
                FBXMaterial material = _fbxMaterials.value(childID);

                for (int j = 0; j < extracted.partMaterialTextures.size(); j++) {
                    if (extracted.partMaterialTextures.at(j).first == materialIndex) {
                        FBXMeshPart& part = extracted.mesh.parts[j];
                        part.materialID = material.materialID;
                        generateTangents |= material.needTangentSpace();
                    }
                }

                materialIndex++;

            } else if (_textureFilenames.contains(childID)) {
                FBXTexture texture = getTexture(childID);
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

        // if we have a normal map (and texture coordinates), we must compute tangents
        if (generateTangents && !extracted.mesh.texCoords.isEmpty()) {
            extracted.mesh.tangents.resize(extracted.mesh.vertices.size());
            foreach (const FBXMeshPart& part, extracted.mesh.parts) {
                for (int i = 0; i < part.quadIndices.size(); i += 4) {
                    setTangents(extracted.mesh, part.quadIndices.at(i), part.quadIndices.at(i + 1));
                    setTangents(extracted.mesh, part.quadIndices.at(i + 1), part.quadIndices.at(i + 2));
                    setTangents(extracted.mesh, part.quadIndices.at(i + 2), part.quadIndices.at(i + 3));
                    setTangents(extracted.mesh, part.quadIndices.at(i + 3), part.quadIndices.at(i));
                }
                // <= size - 3 in order to prevent overflowing triangleIndices when (i % 3) != 0
                // This is most likely evidence of a further problem in extractMesh()
                for (int i = 0; i <= part.triangleIndices.size() - 3; i += 3) {
                    setTangents(extracted.mesh, part.triangleIndices.at(i), part.triangleIndices.at(i + 1));
                    setTangents(extracted.mesh, part.triangleIndices.at(i + 1), part.triangleIndices.at(i + 2));
                    setTangents(extracted.mesh, part.triangleIndices.at(i + 2), part.triangleIndices.at(i));
                }
                if ((part.triangleIndices.size() % 3) != 0){
                    qCDebug(modelformat) << "Error in extractFBXGeometry part.triangleIndices.size() is not divisible by three ";
                }
            }
        }

        // find the clusters with which the mesh is associated
        QVector<QString> clusterIDs;
        foreach (const QString& childID, _connectionChildMap.values(it.key())) {
            foreach (const QString& clusterID, _connectionChildMap.values(childID)) {
                if (!clusters.contains(clusterID)) {
                    continue;
                }
                FBXCluster fbxCluster;
                const Cluster& cluster = clusters[clusterID];
                clusterIDs.append(clusterID);

                // see http://stackoverflow.com/questions/13566608/loading-skinning-information-from-fbx for a discussion
                // of skinning information in FBX
                QString jointID = _connectionChildMap.value(clusterID);
                fbxCluster.jointIndex = modelIDs.indexOf(jointID);
                if (fbxCluster.jointIndex == -1) {
                    qCDebug(modelformat) << "Joint not in model list: " << jointID;
                    fbxCluster.jointIndex = 0;
                }
                fbxCluster.inverseBindMatrix = glm::inverse(cluster.transformLink) * modelTransform;
                extracted.mesh.clusters.append(fbxCluster);

                // override the bind rotation with the transform link
                FBXJoint& joint = geometry.joints[fbxCluster.jointIndex];
                joint.inverseBindRotation = glm::inverse(extractRotation(cluster.transformLink));
                joint.bindTransform = cluster.transformLink;
                joint.bindTransformFoundInCluster = true;

                // update the bind pose extents
                glm::vec3 bindTranslation = extractTranslation(geometry.offset * joint.bindTransform);
                geometry.bindExtents.addPoint(bindTranslation);
            }
        }

        // if we don't have a skinned joint, parent to the model itself
        if (extracted.mesh.clusters.isEmpty()) {
            FBXCluster cluster;
            cluster.jointIndex = modelIDs.indexOf(modelID);
            if (cluster.jointIndex == -1) {
                qCDebug(modelformat) << "Model not in model list: " << modelID;
                cluster.jointIndex = 0;
            }
            extracted.mesh.clusters.append(cluster);
        }

        // whether we're skinned depends on how many clusters are attached
        const FBXCluster& firstFBXCluster = extracted.mesh.clusters.at(0);
        int maxJointIndex = firstFBXCluster.jointIndex;
        glm::mat4 inverseModelTransform = glm::inverse(modelTransform);
        if (clusterIDs.size() > 1) {
            // this is a multi-mesh joint
            extracted.mesh.clusterIndices.resize(extracted.mesh.vertices.size());
            extracted.mesh.clusterWeights.resize(extracted.mesh.vertices.size());
            float maxWeight = 0.0f;
            for (int i = 0; i < clusterIDs.size(); i++) {
                QString clusterID = clusterIDs.at(i);
                const Cluster& cluster = clusters[clusterID];
                const FBXCluster& fbxCluster = extracted.mesh.clusters.at(i);
                int jointIndex = fbxCluster.jointIndex;
                FBXJoint& joint = geometry.joints[jointIndex];
                glm::mat4 transformJointToMesh = inverseModelTransform * joint.bindTransform;
                glm::vec3 boneEnd = extractTranslation(transformJointToMesh);
                glm::vec3 boneBegin = boneEnd;
                glm::vec3 boneDirection;
                float boneLength = 0.0f;
                if (joint.parentIndex != -1) {
                    boneBegin = extractTranslation(inverseModelTransform * geometry.joints[joint.parentIndex].bindTransform);
                    boneDirection = boneEnd - boneBegin;
                    boneLength = glm::length(boneDirection);
                    if (boneLength > EPSILON) {
                        boneDirection /= boneLength;
                    }
                }

                float clusterScale = extractUniformScale(fbxCluster.inverseBindMatrix);
                glm::mat4 meshToJoint = glm::inverse(joint.bindTransform) * modelTransform;
                ShapeVertices& points = shapeVertices.at(jointIndex);

                float totalWeight = 0.0f;
                for (int j = 0; j < cluster.indices.size(); j++) {
                    int oldIndex = cluster.indices.at(j);
                    float weight = cluster.weights.at(j);
                    totalWeight += weight;
                    for (QMultiHash<int, int>::const_iterator it = extracted.newIndices.constFind(oldIndex);
                            it != extracted.newIndices.end() && it.key() == oldIndex; it++) {

                        // remember vertices with at least 1/4 weight
                        const float EXPANSION_WEIGHT_THRESHOLD = 0.99f;
                        if (weight > EXPANSION_WEIGHT_THRESHOLD) {
                            // transform to joint-frame and save for later
                            const glm::mat4 vertexTransform = meshToJoint * glm::translate(extracted.mesh.vertices.at(it.value()));
                            points.push_back(extractTranslation(vertexTransform) * clusterScale);
                        }

                        // look for an unused slot in the weights vector
                        glm::vec4& weights = extracted.mesh.clusterWeights[it.value()];
                        int lowestIndex = -1;
                        float lowestWeight = FLT_MAX;
                        int k = 0;
                        for (; k < 4; k++) {
                            if (weights[k] == 0.0f) {
                                extracted.mesh.clusterIndices[it.value()][k] = i;
                                weights[k] = weight;
                                break;
                            }
                            if (weights[k] < lowestWeight) {
                                lowestIndex = k;
                                lowestWeight = weights[k];
                            }
                        }
                        if (k == 4 && weight > lowestWeight) {
                            // no space for an additional weight; we must replace the lowest
                            weights[lowestIndex] = weight;
                            extracted.mesh.clusterIndices[it.value()][lowestIndex] = i;
                        }
                    }
                }
                if (totalWeight > maxWeight) {
                    maxWeight = totalWeight;
                    maxJointIndex = jointIndex;
                }
            }
            // normalize the weights if they don't add up to one
            for (int i = 0; i < extracted.mesh.clusterWeights.size(); i++) {
                glm::vec4& weights = extracted.mesh.clusterWeights[i];
                float total = weights.x + weights.y + weights.z + weights.w;
                if (total != 1.0f && total != 0.0f) {
                    weights /= total;
                }
            }
        } else {
            // this is a single-mesh joint
            int jointIndex = maxJointIndex;
            FBXJoint& joint = geometry.joints[jointIndex];

            // transform cluster vertices to joint-frame and save for later
            float clusterScale = extractUniformScale(firstFBXCluster.inverseBindMatrix);
            glm::mat4 meshToJoint = glm::inverse(joint.bindTransform) * modelTransform;
            ShapeVertices& points = shapeVertices.at(jointIndex);
            foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
                const glm::mat4 vertexTransform = meshToJoint * glm::translate(vertex);
                points.push_back(extractTranslation(vertexTransform) * clusterScale);
            }

        }
        extracted.mesh.isEye = (maxJointIndex == geometry.leftEyeJointIndex || maxJointIndex == geometry.rightEyeJointIndex);

        buildModelMesh(extracted.mesh, url);

        if (extracted.mesh.isEye) {
            if (maxJointIndex == geometry.leftEyeJointIndex) {
                geometry.leftEyeSize = extracted.mesh.meshExtents.largestDimension() * offsetScale;
            } else {
                geometry.rightEyeSize = extracted.mesh.meshExtents.largestDimension() * offsetScale;
            }
        }

        geometry.meshes.append(extracted.mesh);
        int meshIndex = geometry.meshes.size() - 1;
        meshIDsToMeshIndices.insert(it.key(), meshIndex);
    }

    const float INV_SQRT_3 = 0.57735026918f;
    ShapeVertices cardinalDirections = {
        Vectors::UNIT_X,
        Vectors::UNIT_Y,
        Vectors::UNIT_Z,
        glm::vec3(INV_SQRT_3,  INV_SQRT_3,  INV_SQRT_3),
        glm::vec3(INV_SQRT_3, -INV_SQRT_3,  INV_SQRT_3),
        glm::vec3(INV_SQRT_3,  INV_SQRT_3, -INV_SQRT_3),
        glm::vec3(INV_SQRT_3, -INV_SQRT_3, -INV_SQRT_3)
    };

    // now that all joints have been scanned compute a k-Dop bounding volume of mesh
    glm::vec3 defaultCapsuleAxis(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < geometry.joints.size(); ++i) {
        FBXJoint& joint = geometry.joints[i];

        // NOTE: points are in joint-frame
        ShapeVertices& points = shapeVertices.at(i);
        if (points.size() > 0) {
            // compute average point
            glm::vec3 avgPoint = glm::vec3(0.0f);
            for (uint32_t j = 0; j < points.size(); ++j) {
                avgPoint += points[j];
            }
            avgPoint /= (float)points.size();

            // compute a k-Dop bounding volume
            for (uint32_t j = 0; j < cardinalDirections.size(); ++j) {
                float maxDot = -FLT_MAX;
                float minDot = FLT_MIN;
                for (uint32_t k = 0; k < points.size(); ++k) {
                    float kDot = glm::dot(cardinalDirections[j], points[k] - avgPoint);
                    if (kDot > maxDot) {
                        maxDot = kDot;
                    }
                    if (kDot < minDot) {
                        minDot = kDot;
                    }
                }
                joint.shapeInfo.points.push_back(avgPoint + maxDot * cardinalDirections[j]);
                joint.shapeInfo.points.push_back(avgPoint + minDot * cardinalDirections[j]);
            }
        }
    }
    geometry.palmDirection = parseVec3(mapping.value("palmDirection", "0, -1, 0").toString());

    // Add sitting points
    QVariantHash sittingPoints = mapping.value("sit").toHash();
    for (QVariantHash::const_iterator it = sittingPoints.constBegin(); it != sittingPoints.constEnd(); it++) {
        SittingPoint sittingPoint;
        sittingPoint.name = it.key();

        QVariantList properties = it->toList();
        sittingPoint.position = parseVec3(properties.at(0).toString());
        sittingPoint.rotation = glm::quat(glm::radians(parseVec3(properties.at(1).toString())));

        geometry.sittingPoints.append(sittingPoint);
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
                geometry.meshIndicesToModelNames.insert(meshIndex, modelName);
            }
        }
    }

    return geometryPtr;
}

FBXGeometry* readFBX(const QByteArray& model, const QVariantHash& mapping, const QString& url, bool loadLightmaps, float lightmapLevel) {
    QBuffer buffer(const_cast<QByteArray*>(&model));
    buffer.open(QIODevice::ReadOnly);
    return readFBX(&buffer, mapping, url, loadLightmaps, lightmapLevel);
}

FBXGeometry* readFBX(QIODevice* device, const QVariantHash& mapping, const QString& url, bool loadLightmaps, float lightmapLevel) {
    FBXReader reader;
    reader._fbxNode = FBXReader::parseFBX(device);
    reader._loadLightmaps = loadLightmaps;
    reader._lightmapLevel = lightmapLevel;

    return reader.extractFBXGeometry(mapping, url);
}
