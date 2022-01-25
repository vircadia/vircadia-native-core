//
//  GLTFSerializer.cpp
//  libraries/model-serializers/src
//
//  Created by Luis Cuenca on 8/30/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLTFSerializer.h"

#include <QtCore/QBuffer>
#include <QtCore/QIODevice>
#include <QtCore/QEventLoop>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qpair.h>
#include <QtCore/qlist.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include <qfile.h>
#include <qfileinfo.h>

#include <shared/NsightHelpers.h>
#include <NetworkAccessManager.h>
#include <ResourceManager.h>
#include <PathUtils.h>
#include <image/ColorChannel.h>
#include <BlendshapeConstants.h>

#include "FBXSerializer.h"

#define GLTF_GET_INDICIES(accCount) int index1 = (indices[n + 0] * accCount); int index2 = (indices[n + 1] * accCount); int index3 = (indices[n + 2] * accCount);

#define GLTF_APPEND_ARRAY_1(newArray, oldArray) GLTF_GET_INDICIES(1) \
newArray.append(oldArray[index1]); \
newArray.append(oldArray[index2]); \
newArray.append(oldArray[index3]);

#define GLTF_APPEND_ARRAY_2(newArray, oldArray) GLTF_GET_INDICIES(2) \
newArray.append(oldArray[index1]); newArray.append(oldArray[index1 + 1]); \
newArray.append(oldArray[index2]); newArray.append(oldArray[index2 + 1]); \
newArray.append(oldArray[index3]); newArray.append(oldArray[index3 + 1]);

#define GLTF_APPEND_ARRAY_3(newArray, oldArray) GLTF_GET_INDICIES(3) \
newArray.append(oldArray[index1]); newArray.append(oldArray[index1 + 1]); newArray.append(oldArray[index1 + 2]); \
newArray.append(oldArray[index2]); newArray.append(oldArray[index2 + 1]); newArray.append(oldArray[index2 + 2]); \
newArray.append(oldArray[index3]); newArray.append(oldArray[index3 + 1]); newArray.append(oldArray[index3 + 2]);

#define GLTF_APPEND_ARRAY_4(newArray, oldArray) GLTF_GET_INDICIES(4) \
newArray.append(oldArray[index1]); newArray.append(oldArray[index1 + 1]); newArray.append(oldArray[index1 + 2]); newArray.append(oldArray[index1 + 3]); \
newArray.append(oldArray[index2]); newArray.append(oldArray[index2 + 1]); newArray.append(oldArray[index2 + 2]); newArray.append(oldArray[index2 + 3]); \
newArray.append(oldArray[index3]); newArray.append(oldArray[index3 + 1]); newArray.append(oldArray[index3 + 2]); newArray.append(oldArray[index3 + 3]);

bool GLTFSerializer::getStringVal(const QJsonObject& object, const QString& fieldname,
                              QString& value, QMap<QString, bool>&  defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isString());
    if (_defined) {
        value = object[fieldname].toString();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getBoolVal(const QJsonObject& object, const QString& fieldname,
                            bool& value, QMap<QString, bool>&  defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isBool());
    if (_defined) {
        value = object[fieldname].toBool();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getIntVal(const QJsonObject& object, const QString& fieldname,
                           int& value, QMap<QString, bool>&  defined) {
    bool _defined = (object.contains(fieldname) && !object[fieldname].isNull());
    if (_defined) {
        value = object[fieldname].toInt();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getDoubleVal(const QJsonObject& object, const QString& fieldname,
                              double& value, QMap<QString, bool>&  defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isDouble());
    if (_defined) {
        value = object[fieldname].toDouble();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}
bool GLTFSerializer::getObjectVal(const QJsonObject& object, const QString& fieldname,
                              QJsonObject& value, QMap<QString, bool>&  defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isObject());
    if (_defined) {
        value = object[fieldname].toObject();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getIntArrayVal(const QJsonObject& object, const QString& fieldname,
                                QVector<int>& values, QMap<QString, bool>&  defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isArray());
    if (_defined) {
        QJsonArray arr = object[fieldname].toArray();
        foreach(const QJsonValue & v, arr) {
            if (!v.isNull()) {
                values.push_back(v.toInt());
            }
        }
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getDoubleArrayVal(const QJsonObject& object, const QString& fieldname,
                                   QVector<double>& values, QMap<QString, bool>&  defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isArray());
    if (_defined) {
        QJsonArray arr = object[fieldname].toArray();
        foreach(const QJsonValue & v, arr) {
            if (v.isDouble()) {
                values.push_back(v.toDouble());
            }
        }
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getObjectArrayVal(const QJsonObject& object, const QString& fieldname,
                                   QJsonArray& objects, QMap<QString, bool>& defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isArray());
    if (_defined) {
        objects = object[fieldname].toArray();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

hifi::ByteArray GLTFSerializer::setGLBChunks(const hifi::ByteArray& data) {
    int byte = 4;
    int jsonStart = data.indexOf("JSON", Qt::CaseSensitive);
    int binStart = data.indexOf("BIN", Qt::CaseSensitive);
    int jsonLength, binLength;
    hifi::ByteArray jsonLengthChunk, binLengthChunk;

    jsonLengthChunk = data.mid(jsonStart - byte, byte);
    QDataStream tempJsonLen(jsonLengthChunk);
    tempJsonLen.setByteOrder(QDataStream::LittleEndian);
    tempJsonLen >> jsonLength;
    hifi::ByteArray jsonChunk = data.mid(jsonStart + byte, jsonLength);

    if (binStart != -1) {
        binLengthChunk = data.mid(binStart - byte, byte);

        QDataStream tempBinLen(binLengthChunk);
        tempBinLen.setByteOrder(QDataStream::LittleEndian);
        tempBinLen >> binLength;

        _glbBinary = data.mid(binStart + byte, binLength);
    }
    return jsonChunk;
}

int GLTFSerializer::getMeshPrimitiveRenderingMode(const QString& type)
{
    if (type == "POINTS") {
        return GLTFMeshPrimitivesRenderingMode::POINTS;
    }
    if (type == "LINES") {
        return GLTFMeshPrimitivesRenderingMode::LINES;
    }
    if (type == "LINE_LOOP") {
        return GLTFMeshPrimitivesRenderingMode::LINE_LOOP;
    }
    if (type == "LINE_STRIP") {
        return GLTFMeshPrimitivesRenderingMode::LINE_STRIP;
    }
    if (type == "TRIANGLES") {
        return GLTFMeshPrimitivesRenderingMode::TRIANGLES;
    }
    if (type == "TRIANGLE_STRIP") {
        return GLTFMeshPrimitivesRenderingMode::TRIANGLE_STRIP;
    }
    if (type == "TRIANGLE_FAN") {
        return GLTFMeshPrimitivesRenderingMode::TRIANGLE_FAN;
    }
    return GLTFMeshPrimitivesRenderingMode::TRIANGLES;
}

int GLTFSerializer::getAccessorType(const QString& type)
{
    if (type == "SCALAR") {
        return GLTFAccessorType::SCALAR;
    }
    if (type == "VEC2") {
        return GLTFAccessorType::VEC2;
    }
    if (type == "VEC3") {
        return GLTFAccessorType::VEC3;
    }
    if (type == "VEC4") {
        return GLTFAccessorType::VEC4;
    }
    if (type == "MAT2") {
        return GLTFAccessorType::MAT2;
    }
    if (type == "MAT3") {
        return GLTFAccessorType::MAT3;
    }
    if (type == "MAT4") {
        return GLTFAccessorType::MAT4;
    }
    return GLTFAccessorType::SCALAR;
}

graphics::MaterialKey::OpacityMapMode GLTFSerializer::getMaterialAlphaMode(const QString& type) {
    if (type == "OPAQUE") {
        return graphics::MaterialKey::OPACITY_MAP_OPAQUE;
    }
    if (type == "MASK") {
        return graphics::MaterialKey::OPACITY_MAP_MASK;
    }
    if (type == "BLEND") {
        return graphics::MaterialKey::OPACITY_MAP_BLEND;
    }
    return graphics::MaterialKey::OPACITY_MAP_BLEND;
}

int GLTFSerializer::getCameraType(const QString& type)
{
    if (type == "orthographic") {
        return GLTFCameraTypes::ORTHOGRAPHIC;
    }
    if (type == "perspective") {
        return GLTFCameraTypes::PERSPECTIVE;
    }
    return GLTFCameraTypes::PERSPECTIVE;
}

int GLTFSerializer::getImageMimeType(const QString& mime)
{
    if (mime == "image/jpeg") {
        return GLTFImageMimetype::JPEG;
    }
    if (mime == "image/png") {
        return GLTFImageMimetype::PNG;
    }
    return GLTFImageMimetype::JPEG;
}

int GLTFSerializer::getAnimationSamplerInterpolation(const QString& interpolation)
{
    if (interpolation == "LINEAR") {
        return GLTFAnimationSamplerInterpolation::LINEAR;
    }
    return GLTFAnimationSamplerInterpolation::LINEAR;
}

bool GLTFSerializer::setAsset(const QJsonObject& object) {
    QJsonObject jsAsset;
    bool isAssetDefined = getObjectVal(object, "asset", jsAsset, _file.defined);
    if (isAssetDefined) {
        if (!getStringVal(jsAsset, "version", _file.asset.version,
                          _file.asset.defined) || _file.asset.version != "2.0") {
            return false;
        }
        getStringVal(jsAsset, "generator", _file.asset.generator, _file.asset.defined);
        getStringVal(jsAsset, "copyright", _file.asset.copyright, _file.asset.defined);
    }
    return isAssetDefined;
}

GLTFAccessor::GLTFAccessorSparse::GLTFAccessorSparseIndices GLTFSerializer::createAccessorSparseIndices(const QJsonObject& object) {
    GLTFAccessor::GLTFAccessorSparse::GLTFAccessorSparseIndices accessorSparseIndices;

    getIntVal(object, "bufferView", accessorSparseIndices.bufferView, accessorSparseIndices.defined);
    getIntVal(object, "byteOffset", accessorSparseIndices.byteOffset, accessorSparseIndices.defined);
    getIntVal(object, "componentType", accessorSparseIndices.componentType, accessorSparseIndices.defined);

    return accessorSparseIndices;
}

GLTFAccessor::GLTFAccessorSparse::GLTFAccessorSparseValues GLTFSerializer::createAccessorSparseValues(const QJsonObject& object) {
    GLTFAccessor::GLTFAccessorSparse::GLTFAccessorSparseValues accessorSparseValues;

    getIntVal(object, "bufferView", accessorSparseValues.bufferView, accessorSparseValues.defined);
    getIntVal(object, "byteOffset", accessorSparseValues.byteOffset, accessorSparseValues.defined);

    return accessorSparseValues;
}

GLTFAccessor::GLTFAccessorSparse GLTFSerializer::createAccessorSparse(const QJsonObject& object) {
    GLTFAccessor::GLTFAccessorSparse accessorSparse;

    getIntVal(object, "count", accessorSparse.count, accessorSparse.defined);
    QJsonObject sparseIndicesObject;
    if (getObjectVal(object, "indices", sparseIndicesObject, accessorSparse.defined)) {
        accessorSparse.indices = createAccessorSparseIndices(sparseIndicesObject);
    }
    QJsonObject sparseValuesObject;
    if (getObjectVal(object, "values", sparseValuesObject, accessorSparse.defined)) {
        accessorSparse.values = createAccessorSparseValues(sparseValuesObject);
    }

    return accessorSparse;
}

bool GLTFSerializer::addAccessor(const QJsonObject& object) {
    GLTFAccessor accessor;

    getIntVal(object, "bufferView", accessor.bufferView, accessor.defined);
    getIntVal(object, "byteOffset", accessor.byteOffset, accessor.defined);
    getIntVal(object, "componentType", accessor.componentType, accessor.defined);
    getIntVal(object, "count", accessor.count, accessor.defined);
    getBoolVal(object, "normalized", accessor.normalized, accessor.defined);
    QString type;
    if (getStringVal(object, "type", type, accessor.defined)) {
        accessor.type = getAccessorType(type);
    }

    QJsonObject sparseObject;
    if (getObjectVal(object, "sparse", sparseObject, accessor.defined)) {
        accessor.sparse = createAccessorSparse(sparseObject);
    }

    getDoubleArrayVal(object, "max", accessor.max, accessor.defined);
    getDoubleArrayVal(object, "min", accessor.min, accessor.defined);

    _file.accessors.push_back(accessor);

    return true;
}

bool GLTFSerializer::addAnimation(const QJsonObject& object) {
    GLTFAnimation animation;

    QJsonArray channels;
    if (getObjectArrayVal(object, "channels", channels, animation.defined)) {
        foreach(const QJsonValue & v, channels) {
            if (v.isObject()) {
                GLTFChannel channel;
                getIntVal(v.toObject(), "sampler", channel.sampler, channel.defined);
                QJsonObject jsChannel;
                if (getObjectVal(v.toObject(), "target", jsChannel, channel.defined)) {
                    getIntVal(jsChannel, "node", channel.target.node, channel.target.defined);
                    getIntVal(jsChannel, "path", channel.target.path, channel.target.defined);
                }
            }
        }
    }

    QJsonArray samplers;
    if (getObjectArrayVal(object, "samplers", samplers, animation.defined)) {
        foreach(const QJsonValue & v, samplers) {
            if (v.isObject()) {
                GLTFAnimationSampler sampler;
                getIntVal(v.toObject(), "input", sampler.input, sampler.defined);
                getIntVal(v.toObject(), "output", sampler.input, sampler.defined);
                QString interpolation;
                if (getStringVal(v.toObject(), "interpolation", interpolation, sampler.defined)) {
                    sampler.interpolation = getAnimationSamplerInterpolation(interpolation);
                }
            }
        }
    }

    _file.animations.push_back(animation);

    return true;
}

bool GLTFSerializer::addBufferView(const QJsonObject& object) {
    GLTFBufferView bufferview;

    getIntVal(object, "buffer", bufferview.buffer, bufferview.defined);
    getIntVal(object, "byteLength", bufferview.byteLength, bufferview.defined);
    getIntVal(object, "byteOffset", bufferview.byteOffset, bufferview.defined);
    getIntVal(object, "target", bufferview.target, bufferview.defined);

    _file.bufferviews.push_back(bufferview);

    return true;
}

bool GLTFSerializer::addBuffer(const QJsonObject& object) {
    GLTFBuffer buffer;

    getIntVal(object, "byteLength", buffer.byteLength, buffer.defined);

    if (_url.path().endsWith("glb")) {
        if (!_glbBinary.isEmpty()) {
            buffer.blob = _glbBinary;
        } else {
            return false;
        }
    }
    if (getStringVal(object, "uri", buffer.uri, buffer.defined)) {
        if (!readBinary(buffer.uri, buffer.blob)) {
            return false;
        }
    }
    _file.buffers.push_back(buffer);

    return true;
}

bool GLTFSerializer::addCamera(const QJsonObject& object) {
    GLTFCamera camera;

    QJsonObject jsPerspective;
    QJsonObject jsOrthographic;
    QString type;
    getStringVal(object, "name", camera.name, camera.defined);
    if (getObjectVal(object, "perspective", jsPerspective, camera.defined)) {
        getDoubleVal(jsPerspective, "aspectRatio", camera.perspective.aspectRatio, camera.perspective.defined);
        getDoubleVal(jsPerspective, "yfov", camera.perspective.yfov, camera.perspective.defined);
        getDoubleVal(jsPerspective, "zfar", camera.perspective.zfar, camera.perspective.defined);
        getDoubleVal(jsPerspective, "znear", camera.perspective.znear, camera.perspective.defined);
        camera.type = GLTFCameraTypes::PERSPECTIVE;
    } else if (getObjectVal(object, "orthographic", jsOrthographic, camera.defined)) {
        getDoubleVal(jsOrthographic, "zfar", camera.orthographic.zfar, camera.orthographic.defined);
        getDoubleVal(jsOrthographic, "znear", camera.orthographic.znear, camera.orthographic.defined);
        getDoubleVal(jsOrthographic, "xmag", camera.orthographic.xmag, camera.orthographic.defined);
        getDoubleVal(jsOrthographic, "ymag", camera.orthographic.ymag, camera.orthographic.defined);
        camera.type = GLTFCameraTypes::ORTHOGRAPHIC;
    } else if (getStringVal(object, "type", type, camera.defined)) {
        camera.type = getCameraType(type);
    }

    _file.cameras.push_back(camera);

    return true;
}

bool GLTFSerializer::addImage(const QJsonObject& object) {
    GLTFImage image;

    QString mime;
    getStringVal(object, "uri", image.uri, image.defined);
    if (image.uri.contains("data:image/png;base64,")) {
        image.mimeType = getImageMimeType("image/png");
    } else if (image.uri.contains("data:image/jpeg;base64,")) {
        image.mimeType = getImageMimeType("image/jpeg");
    }
    if (getStringVal(object, "mimeType", mime, image.defined)) {
        image.mimeType = getImageMimeType(mime);
    }
    getIntVal(object, "bufferView", image.bufferView, image.defined);

    _file.images.push_back(image);

    return true;
}

bool GLTFSerializer::getIndexFromObject(const QJsonObject& object, const QString& field,
                                    int& outidx, QMap<QString, bool>& defined) {
    QJsonObject subobject;
    if (getObjectVal(object, field, subobject, defined)) {
        QMap<QString, bool> tmpdefined = QMap<QString, bool>();
        return getIntVal(subobject, "index", outidx, tmpdefined);
    }
    return false;
}

bool GLTFSerializer::addMaterial(const QJsonObject& object) {
    GLTFMaterial material;

    getStringVal(object, "name", material.name, material.defined);
    getDoubleArrayVal(object, "emissiveFactor", material.emissiveFactor, material.defined);
    getIndexFromObject(object, "emissiveTexture", material.emissiveTexture, material.defined);
    getIndexFromObject(object, "normalTexture", material.normalTexture, material.defined);
    getIndexFromObject(object, "occlusionTexture", material.occlusionTexture, material.defined);
    getBoolVal(object, "doubleSided", material.doubleSided, material.defined);
    QString alphaMode;
    if (getStringVal(object, "alphaMode", alphaMode, material.defined)) {
        material.alphaMode = getMaterialAlphaMode(alphaMode);
    }
    getDoubleVal(object, "alphaCutoff", material.alphaCutoff, material.defined);
    QJsonObject jsMetallicRoughness;
    if (getObjectVal(object, "pbrMetallicRoughness", jsMetallicRoughness, material.defined)) {
        getDoubleArrayVal(jsMetallicRoughness, "baseColorFactor",
                          material.pbrMetallicRoughness.baseColorFactor,
                          material.pbrMetallicRoughness.defined);
        getIndexFromObject(jsMetallicRoughness, "baseColorTexture",
                           material.pbrMetallicRoughness.baseColorTexture,
                           material.pbrMetallicRoughness.defined);
        // Undefined metallicFactor used with pbrMetallicRoughness means metallicFactor == 1.0
        if (!getDoubleVal(jsMetallicRoughness, "metallicFactor",
                     material.pbrMetallicRoughness.metallicFactor,
                     material.pbrMetallicRoughness.defined)) {
            material.pbrMetallicRoughness.metallicFactor = 1.0;
            material.pbrMetallicRoughness.defined["metallicFactor"] = true;
        }
        getDoubleVal(jsMetallicRoughness, "roughnessFactor",
                     material.pbrMetallicRoughness.roughnessFactor,
                     material.pbrMetallicRoughness.defined);
        getIndexFromObject(jsMetallicRoughness, "metallicRoughnessTexture",
                           material.pbrMetallicRoughness.metallicRoughnessTexture,
                           material.pbrMetallicRoughness.defined);
    }
   _file.materials.push_back(material);
    return true;
}

bool GLTFSerializer::addMesh(const QJsonObject& object) {
    GLTFMesh mesh;

    getStringVal(object, "name", mesh.name, mesh.defined);
    getDoubleArrayVal(object, "weights", mesh.weights, mesh.defined);
    QJsonArray jsPrimitives;
    object.keys();
    if (getObjectArrayVal(object, "primitives", jsPrimitives, mesh.defined)) {
        foreach(const QJsonValue & prim, jsPrimitives) {
            if (prim.isObject()) {
                GLTFMeshPrimitive primitive;
                QJsonObject jsPrimitive = prim.toObject();
                getIntVal(jsPrimitive, "mode", primitive.mode, primitive.defined);
                getIntVal(jsPrimitive, "indices", primitive.indices, primitive.defined);
                getIntVal(jsPrimitive, "material", primitive.material, primitive.defined);

                QJsonObject jsAttributes;
                if (getObjectVal(jsPrimitive, "attributes", jsAttributes, primitive.defined)) {
                    QStringList attrKeys = jsAttributes.keys();
                    foreach(const QString & attrKey, attrKeys) {
                        int attrVal;
                        getIntVal(jsAttributes, attrKey, attrVal, primitive.attributes.defined);
                        primitive.attributes.values.insert(attrKey, attrVal);
                    }
                }

                QJsonArray jsTargets;
                if (getObjectArrayVal(jsPrimitive, "targets", jsTargets, primitive.defined)) {
                    foreach(const QJsonValue & tar, jsTargets) {
                        if (tar.isObject()) {
                            QJsonObject jsTarget = tar.toObject();
                            QStringList tarKeys = jsTarget.keys();
                            GLTFMeshPrimitiveAttr target;
                            foreach(const QString & tarKey, tarKeys) {
                                int tarVal;
                                getIntVal(jsTarget, tarKey, tarVal, target.defined);
                                target.values.insert(tarKey, tarVal);
                            }
                            primitive.targets.push_back(target);
                        }
                    }
                }
                mesh.primitives.push_back(primitive);
            }
        }
    }

    QJsonObject jsExtras;
    GLTFMeshExtra extras;
    if (getObjectVal(object, "extras", jsExtras, mesh.defined)) {
        QJsonArray jsTargetNames;
        if (getObjectArrayVal(jsExtras, "targetNames", jsTargetNames, extras.defined)) {
            foreach (const QJsonValue& tarName, jsTargetNames) {
                extras.targetNames.push_back(tarName.toString());
            }
        }
        mesh.extras = extras;
    }

    _file.meshes.push_back(mesh);

    return true;
}

bool GLTFSerializer::addNode(const QJsonObject& object) {
    GLTFNode node;

    getStringVal(object, "name", node.name, node.defined);
    getIntVal(object, "camera", node.camera, node.defined);
    getIntVal(object, "mesh", node.mesh, node.defined);
    getIntArrayVal(object, "children", node.children, node.defined);
    getDoubleArrayVal(object, "translation", node.translation, node.defined);
    getDoubleArrayVal(object, "rotation", node.rotation, node.defined);
    getDoubleArrayVal(object, "scale", node.scale, node.defined);
    getDoubleArrayVal(object, "matrix", node.matrix, node.defined);
    getIntVal(object, "skin", node.skin, node.defined);
    getStringVal(object, "jointName", node.jointName, node.defined);
    getIntArrayVal(object, "skeletons", node.skeletons, node.defined);

    _file.nodes.push_back(node);

    return true;
}

bool GLTFSerializer::addSampler(const QJsonObject& object) {
    GLTFSampler sampler;

    getIntVal(object, "magFilter", sampler.magFilter, sampler.defined);
    getIntVal(object, "minFilter", sampler.minFilter, sampler.defined);
    getIntVal(object, "wrapS", sampler.wrapS, sampler.defined);
    getIntVal(object, "wrapT", sampler.wrapT, sampler.defined);

    _file.samplers.push_back(sampler);

    return true;

}

bool GLTFSerializer::addScene(const QJsonObject& object) {
    GLTFScene scene;

    getStringVal(object, "name", scene.name, scene.defined);
    getIntArrayVal(object, "nodes", scene.nodes, scene.defined);

    _file.scenes.push_back(scene);
    return true;
}

bool GLTFSerializer::addSkin(const QJsonObject& object) {
    GLTFSkin skin;

    getIntVal(object, "inverseBindMatrices", skin.inverseBindMatrices, skin.defined);
    getIntVal(object, "skeleton", skin.skeleton, skin.defined);
    getIntArrayVal(object, "joints", skin.joints, skin.defined);

    _file.skins.push_back(skin);

    return true;
}

bool GLTFSerializer::addTexture(const QJsonObject& object) {
    GLTFTexture texture;
    getIntVal(object, "sampler", texture.sampler, texture.defined);
    getIntVal(object, "source", texture.source, texture.defined);

    _file.textures.push_back(texture);

    return true;
}

bool GLTFSerializer::parseGLTF(const hifi::ByteArray& data) {
    PROFILE_RANGE_EX(resource_parse, __FUNCTION__, 0xffff0000, nullptr);

    hifi::ByteArray jsonChunk = data;

    if (_url.path().endsWith("glb") && data.indexOf("glTF") == 0 && data.contains("JSON")) {
        jsonChunk = setGLBChunks(data);
    }

    QJsonDocument d = QJsonDocument::fromJson(jsonChunk);
    QJsonObject jsFile = d.object();

    bool success = setAsset(jsFile);
    if (success) {
        QJsonArray accessors;
        if (getObjectArrayVal(jsFile, "accessors", accessors, _file.defined)) {
            foreach(const QJsonValue & accVal, accessors) {
                if (accVal.isObject()) {
                    success = success && addAccessor(accVal.toObject());
                }
            }
        }

        QJsonArray animations;
        if (getObjectArrayVal(jsFile, "animations", animations, _file.defined)) {
            foreach(const QJsonValue & animVal, accessors) {
                if (animVal.isObject()) {
                    success = success && addAnimation(animVal.toObject());
                }
            }
        }

        QJsonArray bufferViews;
        if (getObjectArrayVal(jsFile, "bufferViews", bufferViews, _file.defined)) {
            foreach(const QJsonValue & bufviewVal, bufferViews) {
                if (bufviewVal.isObject()) {
                    success = success && addBufferView(bufviewVal.toObject());
                }
            }
        }

        QJsonArray buffers;
        if (getObjectArrayVal(jsFile, "buffers", buffers, _file.defined)) {
            foreach(const QJsonValue & bufVal, buffers) {
                if (bufVal.isObject()) {
                    success = success && addBuffer(bufVal.toObject());
                }
            }
        }

        QJsonArray cameras;
        if (getObjectArrayVal(jsFile, "cameras", cameras, _file.defined)) {
            foreach(const QJsonValue & camVal, cameras) {
                if (camVal.isObject()) {
                    success = success && addCamera(camVal.toObject());
                }
            }
        }

        QJsonArray images;
        if (getObjectArrayVal(jsFile, "images", images, _file.defined)) {
            foreach(const QJsonValue & imgVal, images) {
                if (imgVal.isObject()) {
                    success = success && addImage(imgVal.toObject());
                }
            }
        }

        QJsonArray materials;
        if (getObjectArrayVal(jsFile, "materials", materials, _file.defined)) {
            foreach(const QJsonValue & matVal, materials) {
                if (matVal.isObject()) {
                    success = success && addMaterial(matVal.toObject());
                }
            }
        }

        QJsonArray meshes;
        if (getObjectArrayVal(jsFile, "meshes", meshes, _file.defined)) {
            foreach(const QJsonValue & meshVal, meshes) {
                if (meshVal.isObject()) {
                    success = success && addMesh(meshVal.toObject());
                }
            }
        }

        QJsonArray nodes;
        if (getObjectArrayVal(jsFile, "nodes", nodes, _file.defined)) {
            foreach(const QJsonValue & nodeVal, nodes) {
                if (nodeVal.isObject()) {
                    success = success && addNode(nodeVal.toObject());
                }
            }
        }

        QJsonArray samplers;
        if (getObjectArrayVal(jsFile, "samplers", samplers, _file.defined)) {
            foreach(const QJsonValue & samVal, samplers) {
                if (samVal.isObject()) {
                    success = success && addSampler(samVal.toObject());
                }
            }
        }

        QJsonArray scenes;
        if (getObjectArrayVal(jsFile, "scenes", scenes, _file.defined)) {
            foreach(const QJsonValue & sceneVal, scenes) {
                if (sceneVal.isObject()) {
                    success = success && addScene(sceneVal.toObject());
                }
            }
        }

        QJsonArray skins;
        if (getObjectArrayVal(jsFile, "skins", skins, _file.defined)) {
            foreach(const QJsonValue & skinVal, skins) {
                if (skinVal.isObject()) {
                    success = success && addSkin(skinVal.toObject());
                }
            }
        }

        QJsonArray textures;
        if (getObjectArrayVal(jsFile, "textures", textures, _file.defined)) {
            foreach(const QJsonValue & texVal, textures) {
                if (texVal.isObject()) {
                    success = success && addTexture(texVal.toObject());
                }
            }
        }
    }
    return success;
}

glm::mat4 GLTFSerializer::getModelTransform(const GLTFNode& node) {
    glm::mat4 tmat = glm::mat4(1.0);

    if (node.defined["matrix"] && node.matrix.size() == 16) {
        tmat = glm::mat4(node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
            node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
            node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
            node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
    } else {

        if (node.defined["scale"] && node.scale.size() == 3) {
            glm::vec3 scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
            glm::mat4 s = glm::mat4(1.0);
            s = glm::scale(s, scale);
            tmat = s * tmat;
        }

        if (node.defined["rotation"] && node.rotation.size() == 4) {
            //quat(x,y,z,w) to quat(w,x,y,z)
            glm::quat rotquat = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
            tmat = glm::mat4_cast(rotquat) * tmat;
        }

        if (node.defined["translation"] && node.translation.size() == 3) {
            glm::vec3 trans = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
            glm::mat4 t = glm::mat4(1.0);
            t = glm::translate(t, trans);
            tmat = t * tmat;
        }
    }
    return tmat;
}

void GLTFSerializer::getSkinInverseBindMatrices(std::vector<std::vector<float>>& inverseBindMatrixValues) {
    for (auto &skin : _file.skins) {
        GLTFAccessor& indicesAccessor = _file.accessors[skin.inverseBindMatrices];
        QVector<float> matrices;
        addArrayFromAccessor(indicesAccessor, matrices);
        inverseBindMatrixValues.push_back(std::vector<float>(matrices.begin(), matrices.end()));
    }
}

void GLTFSerializer::generateTargetData(int index, float weight, QVector<glm::vec3>& returnVector) {
    GLTFAccessor& accessor = _file.accessors[index];
    QVector<float> storedValues;
    addArrayFromAccessor(accessor, storedValues);
    for (int n = 0; n + 2 < storedValues.size(); n = n + 3) {
        returnVector.push_back(glm::vec3(weight * storedValues[n], weight * storedValues[n + 1], weight * storedValues[n + 2]));
    }
}

bool GLTFSerializer::buildGeometry(HFMModel& hfmModel, const hifi::VariantHash& mapping, const hifi::URL& url) {
    hfmModel.originalURL = url.toString();

    int numNodes = _file.nodes.size();

    //Build dependencies
    QVector<int> parents;
    QVector<int> sortedNodes;
    parents.fill(-1, numNodes);
    sortedNodes.reserve(numNodes);
    int nodecount = 0;
    foreach(auto &node, _file.nodes) {
        foreach(int child, node.children) {
            parents[child] = nodecount;
        }
        sortedNodes.push_back(nodecount);
        ++nodecount;
    }


    // Build transforms
    nodecount = 0;
    foreach(auto &node, _file.nodes) {
        // collect node transform
        _file.nodes[nodecount].transforms.push_back(getModelTransform(node));
        int parentIndex = parents[nodecount];
        while (parentIndex != -1) {
            const auto& parentNode = _file.nodes[parentIndex];
            // collect transforms for a node's parents, grandparents, etc.
            _file.nodes[nodecount].transforms.push_back(getModelTransform(parentNode));
            parentIndex = parents[parentIndex];
        }
        ++nodecount;
    }


    // since parent indices must exist in the sorted list before any of their children, sortedNodes might not be initialized in the correct order
    // therefore we need to re-initialize the order in which nodes will be parsed
    QVector<bool> hasBeenSorted;
    hasBeenSorted.fill(false, numNodes);
    int i = 0;  // initial index
    while (i < numNodes) {
        int currentNode = sortedNodes[i];
        int parentIndex = parents[currentNode];
        if (parentIndex == -1 || hasBeenSorted[parentIndex]) {
            hasBeenSorted[currentNode] = true;
            ++i;
        } else {
            int j = i + 1; // index of node to be sorted
            while (j < numNodes) {
                int nextNode = sortedNodes[j];
                parentIndex = parents[nextNode];
                if (parentIndex == -1 || hasBeenSorted[parentIndex]) {
                    // swap with currentNode
                    hasBeenSorted[nextNode] = true;
                    sortedNodes[i] = nextNode;
                    sortedNodes[j] = currentNode;
                    ++i;
                    currentNode = sortedNodes[i];
                }
                ++j;
            }
        }
    }


     // Build map from original to new indices
    QVector<int> originalToNewNodeIndexMap;
    originalToNewNodeIndexMap.fill(-1, numNodes);
    for (int i = 0; i < numNodes; ++i) {
        originalToNewNodeIndexMap[sortedNodes[i]] = i;
    }


    // Build joints
    HFMJoint joint;
    joint.distanceToParent = 0;
    hfmModel.jointIndices["x"] = numNodes;
    QVector<glm::mat4> globalTransforms;
    globalTransforms.resize(numNodes);

    for (int nodeIndex : sortedNodes) {
        auto& node = _file.nodes[nodeIndex];

        joint.parentIndex = parents[nodeIndex];
        if (joint.parentIndex != -1) {
            joint.parentIndex = originalToNewNodeIndexMap[joint.parentIndex];
        }
        joint.transform = node.transforms.first();
        joint.translation = extractTranslation(joint.transform);
        joint.rotation = glmExtractRotation(joint.transform);
        glm::vec3 scale = extractScale(joint.transform);
        joint.postTransform = glm::scale(glm::mat4(), scale);

		joint.parentIndex = parents[nodeIndex];
        globalTransforms[nodeIndex] = joint.transform;
        if (joint.parentIndex != -1) {
            globalTransforms[nodeIndex] = globalTransforms[joint.parentIndex] * globalTransforms[nodeIndex];
            joint.parentIndex = originalToNewNodeIndexMap[joint.parentIndex];
        }

        joint.name = node.name;
        joint.isSkeletonJoint = false;
        hfmModel.joints.push_back(joint);
    }
    hfmModel.shapeVertices.resize(hfmModel.joints.size());


    // get offset transform from mapping
    float unitScaleFactor = 1.0f;
    float offsetScale = mapping.value("scale", 1.0f).toFloat() * unitScaleFactor;
    glm::quat offsetRotation = glm::quat(glm::radians(glm::vec3(mapping.value("rx").toFloat(), mapping.value("ry").toFloat(), mapping.value("rz").toFloat())));
    hfmModel.offset = glm::translate(glm::mat4(), glm::vec3(mapping.value("tx").toFloat(), mapping.value("ty").toFloat(), mapping.value("tz").toFloat())) *
        glm::mat4_cast(offsetRotation) * glm::scale(glm::mat4(), glm::vec3(offsetScale, offsetScale, offsetScale));


    // Build skeleton
    std::vector<glm::mat4> jointInverseBindTransforms;
    std::vector<glm::mat4> globalBindTransforms;
    jointInverseBindTransforms.resize(numNodes);
    globalBindTransforms.resize(numNodes);

    hfmModel.hasSkeletonJoints = !_file.skins.isEmpty();
    if (hfmModel.hasSkeletonJoints) {
        std::vector<std::vector<float>> inverseBindValues;
        getSkinInverseBindMatrices(inverseBindValues);

        for (int jointIndex = 0; jointIndex < numNodes; ++jointIndex) {
            int nodeIndex = sortedNodes[jointIndex];
            auto joint = hfmModel.joints[jointIndex];

            for (int s = 0; s < _file.skins.size(); ++s) {
                const auto& skin = _file.skins[s];
                int matrixIndex = skin.joints.indexOf(nodeIndex);
                joint.isSkeletonJoint = skin.joints.contains(nodeIndex);

                // build inverse bind matrices
                if (joint.isSkeletonJoint) {
                    std::vector<float>& value = inverseBindValues[s];
                    int matrixCount = 16 * matrixIndex;
                    jointInverseBindTransforms[jointIndex] =
                        glm::mat4(value[matrixCount], value[matrixCount + 1], value[matrixCount + 2], value[matrixCount + 3],
                            value[matrixCount + 4], value[matrixCount + 5], value[matrixCount + 6], value[matrixCount + 7],
                            value[matrixCount + 8], value[matrixCount + 9], value[matrixCount + 10], value[matrixCount + 11],
                            value[matrixCount + 12], value[matrixCount + 13], value[matrixCount + 14], value[matrixCount + 15]);
                } else {
                    jointInverseBindTransforms[jointIndex] = glm::mat4();
                }
                globalBindTransforms[jointIndex] = jointInverseBindTransforms[jointIndex];
                if (joint.parentIndex != -1) {
                    globalBindTransforms[jointIndex] = globalBindTransforms[joint.parentIndex] * globalBindTransforms[jointIndex];
                }
                glm::vec3 bindTranslation = extractTranslation(hfmModel.offset * glm::inverse(jointInverseBindTransforms[jointIndex]));
                hfmModel.bindExtents.addPoint(bindTranslation);
            }
            hfmModel.joints[jointIndex] = joint;
        }
    }


    // Build materials
    QVector<QString> materialIDs;
    QString unknown = "Default";
    int ukcount = 0;
    foreach(auto material, _file.materials) {
        if (!material.defined["name"]) {
            QString name = unknown + QString::number(++ukcount);
            material.name = name;
            material.defined.insert("name", true);
        }

        QString mid = material.name;
        materialIDs.push_back(mid);
    }

    for (int i = 0; i < materialIDs.size(); ++i) {
        QString& matid = materialIDs[i];
        hfmModel.materials[matid] = HFMMaterial();
        HFMMaterial& hfmMaterial = hfmModel.materials[matid];
        hfmMaterial._material = std::make_shared<graphics::Material>();
        hfmMaterial.name = hfmMaterial.materialID = matid;
        setHFMMaterial(hfmMaterial, _file.materials[i]);
    }


    // Build meshes
    nodecount = 0;
    hfmModel.meshExtents.reset();
    for (int nodeIndex : sortedNodes) {
        auto& node = _file.nodes[nodeIndex];

        if (node.defined["mesh"]) {

            hfmModel.meshes.append(HFMMesh());
            HFMMesh& mesh = hfmModel.meshes[hfmModel.meshes.size() - 1];
            mesh.modelTransform = globalTransforms[nodeIndex];

            if (!hfmModel.hasSkeletonJoints) {
                HFMCluster cluster;
                cluster.jointIndex = nodecount;
                cluster.inverseBindMatrix = glm::mat4();
                cluster.inverseBindTransform = Transform(cluster.inverseBindMatrix);
                mesh.clusters.append(cluster);
            } else { // skinned model
                for (int j = 0; j < numNodes; ++j) {
                    HFMCluster cluster;
                    cluster.jointIndex = j;
                    cluster.inverseBindMatrix = jointInverseBindTransforms[j];
                    cluster.inverseBindTransform = Transform(cluster.inverseBindMatrix);
                    mesh.clusters.append(cluster);
                }
            }
            HFMCluster root;
            root.jointIndex = 0;
            root.inverseBindMatrix = jointInverseBindTransforms[root.jointIndex];
            root.inverseBindTransform = Transform(root.inverseBindMatrix);
            mesh.clusters.append(root);

            QList<QString> meshAttributes;
            foreach(auto &primitive, _file.meshes[node.mesh].primitives) {
                QList<QString> keys = primitive.attributes.values.keys();
                foreach (auto &key, keys) {
                    if (!meshAttributes.contains(key)) {
                        meshAttributes.push_back(key);
                    }
                }
            }

            foreach(auto &primitive, _file.meshes[node.mesh].primitives) {
                HFMMeshPart part = HFMMeshPart();

                int indicesAccessorIdx = primitive.indices;

                if (indicesAccessorIdx > _file.accessors.size()) {
                    qWarning(modelformat) << "Indices accessor index is out of bounds for model " << _url;
                    continue;
                }

                GLTFAccessor& indicesAccessor = _file.accessors[indicesAccessorIdx];

                // Buffers
                QVector<int> indices;
                QVector<float> vertices;
                int verticesStride = 3;
                QVector<float> normals;
                int normalStride = 3;
                QVector<float> tangents;
                int tangentStride = 4;
                QVector<float> texcoords;
                int texCoordStride = 2;
                QVector<float> texcoords2;
                int texCoord2Stride = 2;
                QVector<float> colors;
                int colorStride = 3;
                QVector<uint16_t> joints;
                int jointStride = 4;
                QVector<float> weights;
                int weightStride = 4;

                bool success = addArrayFromAccessor(indicesAccessor, indices);

                if (!success) {
                    qWarning(modelformat) << "There was a problem reading glTF INDICES data for model " << _url;
                    continue;
                }

                // Increment the triangle indices by the current mesh vertex count so each mesh part can all reference the same buffers within the mesh
                int prevMeshVerticesCount = mesh.vertices.count();

                QList<QString> keys = primitive.attributes.values.keys();
                QVector<uint16_t> clusterJoints;
                QVector<float> clusterWeights;

                foreach(auto &key, keys) {
                    int accessorIdx = primitive.attributes.values[key];

                    if (accessorIdx > _file.accessors.size()) {
                        qWarning(modelformat) << "Accessor index is out of bounds for model " << _url;
                        continue;
                    }

                    GLTFAccessor& accessor = _file.accessors[accessorIdx];

                    if (key == "POSITION") {
                        if (accessor.type != GLTFAccessorType::VEC3) {
                            qWarning(modelformat) << "Invalid accessor type on glTF POSITION data for model " << _url;
                            continue;
                        }

                        success = addArrayFromAccessor(accessor, vertices);
                        if (!success) {
                            qWarning(modelformat) << "There was a problem reading glTF POSITION data for model " << _url;
                            continue;
                        }
                    } else if (key == "NORMAL") {
                        if (accessor.type != GLTFAccessorType::VEC3) {
                            qWarning(modelformat) << "Invalid accessor type on glTF NORMAL data for model " << _url;
                            continue;
                        }

                        success = addArrayFromAccessor(accessor, normals);
                        if (!success) {
                            qWarning(modelformat) << "There was a problem reading glTF NORMAL data for model " << _url;
                            continue;
                        }
                    } else if (key == "TANGENT") {
                        if (accessor.type == GLTFAccessorType::VEC4) {
                            tangentStride = 4;
                        } else if (accessor.type == GLTFAccessorType::VEC3) {
                            tangentStride = 3;
                        } else {
                            qWarning(modelformat) << "Invalid accessor type on glTF TANGENT data for model " << _url;
                            continue;
                        }

                        success = addArrayFromAccessor(accessor, tangents);
                        if (!success) {
                            qWarning(modelformat) << "There was a problem reading glTF TANGENT data for model " << _url;
                            tangentStride = 0;
                            continue;
                        }
                    } else if (key == "TEXCOORD_0") {
                        success = addArrayFromAccessor(accessor, texcoords);
                        if (!success) {
                            qWarning(modelformat) << "There was a problem reading glTF TEXCOORD_0 data for model " << _url;
                            continue;
                        }

                        if (accessor.type != GLTFAccessorType::VEC2) {
                            qWarning(modelformat) << "Invalid accessor type on glTF TEXCOORD_0 data for model " << _url;
                            continue;
                        }
                    } else if (key == "TEXCOORD_1") {
                        success = addArrayFromAccessor(accessor, texcoords2);
                        if (!success) {
                            qWarning(modelformat) << "There was a problem reading glTF TEXCOORD_1 data for model " << _url;
                            continue;
                        }

                        if (accessor.type != GLTFAccessorType::VEC2) {
                            qWarning(modelformat) << "Invalid accessor type on glTF TEXCOORD_1 data for model " << _url;
                            continue;
                        }
                    } else if (key == "COLOR_0") {
                        if (accessor.type == GLTFAccessorType::VEC4) {
                            colorStride = 4;
                        } else if (accessor.type == GLTFAccessorType::VEC3) {
                            colorStride = 3;
                        } else {
                            qWarning(modelformat) << "Invalid accessor type on glTF COLOR_0 data for model " << _url;
                            continue;
                        }

                        success = addArrayFromAccessor(accessor, colors);
                        if (!success) {
                            qWarning(modelformat) << "There was a problem reading glTF COLOR_0 data for model " << _url;
                            continue;
                        }
                    } else if (key == "JOINTS_0") {
                        if (accessor.type == GLTFAccessorType::VEC4) {
                            jointStride = 4;
                        } else if (accessor.type == GLTFAccessorType::VEC3) {
                            jointStride = 3;
                        } else if (accessor.type == GLTFAccessorType::VEC2) {
                            jointStride = 2;
                        } else if (accessor.type == GLTFAccessorType::SCALAR) {
                            jointStride = 1;
                        } else {
                            qWarning(modelformat) << "Invalid accessor type on glTF JOINTS_0 data for model " << _url;
                            continue;
                        }

                        success = addArrayFromAccessor(accessor, joints);
                        if (!success) {
                            qWarning(modelformat) << "There was a problem reading glTF JOINTS_0 data for model " << _url;
                            continue;
                        }
                    } else if (key == "WEIGHTS_0") {
                        if (accessor.type == GLTFAccessorType::VEC4) {
                            weightStride = 4;
                        } else if (accessor.type == GLTFAccessorType::VEC3) {
                            weightStride = 3;
                        } else if (accessor.type == GLTFAccessorType::VEC2) {
                            weightStride = 2;
                        } else if (accessor.type == GLTFAccessorType::SCALAR) {
                            weightStride = 1;
                        } else {
                            qWarning(modelformat) << "Invalid accessor type on glTF WEIGHTS_0 data for model " << _url;
                            continue;
                        }

                        success = addArrayFromAccessor(accessor, weights);
                        if (!success) {
                            qWarning(modelformat) << "There was a problem reading glTF WEIGHTS_0 data for model " << _url;
                            continue;
                        }
                    }
                }

                // Validation stage
                if (indices.count() == 0) {
                    qWarning(modelformat) << "Missing indices for model " << _url;
                    continue;
                }
                if (vertices.count() == 0) {
                    qWarning(modelformat) << "Missing vertices for model " << _url;
                    continue;
                }

                int partVerticesCount = vertices.size() / 3;

                // generate the normals if they don't exist
                if (normals.size() == 0) {
                    QVector<int> newIndices;
                    QVector<float> newVertices;
                    QVector<float> newNormals;
                    QVector<float> newTexcoords;
                    QVector<float> newTexcoords2;
                    QVector<float> newColors;
                    QVector<uint16_t> newJoints;
                    QVector<float> newWeights;

                    for (int n = 0; n + 2 < indices.size(); n = n + 3) {
                        int v1_index = (indices[n + 0] * 3);
                        int v2_index = (indices[n + 1] * 3);
                        int v3_index = (indices[n + 2] * 3);

                        if (v1_index + 2 >= vertices.size() || v2_index + 2 >= vertices.size() || v3_index + 2 >= vertices.size()) {
                            qWarning(modelformat) << "Indices out of range for model " << _url;
                            break;
                        }

                        glm::vec3 v1 = glm::vec3(vertices[v1_index], vertices[v1_index + 1], vertices[v1_index + 2]);
                        glm::vec3 v2 = glm::vec3(vertices[v2_index], vertices[v2_index + 1], vertices[v2_index + 2]);
                        glm::vec3 v3 = glm::vec3(vertices[v3_index], vertices[v3_index + 1], vertices[v3_index + 2]);

                        newVertices.append(v1.x);
                        newVertices.append(v1.y);
                        newVertices.append(v1.z);
                        newVertices.append(v2.x);
                        newVertices.append(v2.y);
                        newVertices.append(v2.z);
                        newVertices.append(v3.x);
                        newVertices.append(v3.y);
                        newVertices.append(v3.z);

                        glm::vec3 norm = glm::normalize(glm::cross(v2 - v1, v3 - v1));

                        newNormals.append(norm.x);
                        newNormals.append(norm.y);
                        newNormals.append(norm.z);
                        newNormals.append(norm.x);
                        newNormals.append(norm.y);
                        newNormals.append(norm.z);
                        newNormals.append(norm.x);
                        newNormals.append(norm.y);
                        newNormals.append(norm.z);

                        if (texcoords.size() == partVerticesCount * texCoordStride) {
                            GLTF_APPEND_ARRAY_2(newTexcoords, texcoords)
                        }

                        if (texcoords2.size() == partVerticesCount * texCoord2Stride) {
                            GLTF_APPEND_ARRAY_2(newTexcoords2, texcoords2)
                        }

                        if (colors.size() == partVerticesCount * colorStride) {
                            if (colorStride == 4) {
                                GLTF_APPEND_ARRAY_4(newColors, colors)
                            } else {
                                GLTF_APPEND_ARRAY_3(newColors, colors)
                            }
                        }

                        if (joints.size() == partVerticesCount * jointStride) {
                            if (jointStride == 4) {
                                GLTF_APPEND_ARRAY_4(newJoints, joints)
                            } else if (jointStride == 3) {
                                GLTF_APPEND_ARRAY_3(newJoints, joints)
                            } else if (jointStride == 2) {
                                GLTF_APPEND_ARRAY_2(newJoints, joints)
                            } else {
                                GLTF_APPEND_ARRAY_1(newJoints, joints)
                            }
                        }

                        if (weights.size() == partVerticesCount * weightStride) {
                            if (weightStride == 4) {
                                GLTF_APPEND_ARRAY_4(newWeights, weights)
                            } else if (weightStride == 3) {
                                GLTF_APPEND_ARRAY_3(newWeights, weights)
                            } else if (weightStride == 2) {
                                GLTF_APPEND_ARRAY_2(newWeights, weights)
                            } else {
                                GLTF_APPEND_ARRAY_1(newWeights, weights)
                            }
                        }
                        newIndices.append(n);
                        newIndices.append(n + 1);
                        newIndices.append(n + 2);
                    }

                    vertices = newVertices;
                    normals = newNormals;
                    tangents = QVector<float>();
                    texcoords = newTexcoords;
                    texcoords2 = newTexcoords2;
                    colors = newColors;
                    joints = newJoints;
                    weights = newWeights;
                    indices = newIndices;

                    partVerticesCount = vertices.size() / 3;
                }

                QVector<int> validatedIndices;
                for (int n = 0; n < indices.count(); ++n) {
                    if (indices[n] < partVerticesCount) {
                        validatedIndices.push_back(indices[n] + prevMeshVerticesCount);
                    } else {
                        validatedIndices = QVector<int>();
                        break;
                    }
                }

                if (validatedIndices.size() == 0) {
                    qWarning(modelformat) << "No valid indices for model " << _url;
                    continue;
                }

                part.triangleIndices.append(validatedIndices);

                for (int n = 0; n + verticesStride - 1 < vertices.size(); n = n + verticesStride) {
                    mesh.vertices.push_back(glm::vec3(vertices[n], vertices[n + 1], vertices[n + 2]));
                }

                for (int n = 0; n + normalStride - 1 < normals.size(); n = n + normalStride) {
                    mesh.normals.push_back(glm::vec3(normals[n], normals[n + 1], normals[n + 2]));
                }

                // TODO: add correct tangent generation
                if (tangents.size() == partVerticesCount * tangentStride) {
                    for (int n = 0; n + tangentStride - 1 < tangents.size(); n += tangentStride) {
                        float tanW = tangentStride == 4 ? tangents[n + 3] : 1;
                        mesh.tangents.push_back(glm::vec3(tanW * tangents[n], tangents[n + 1], tanW * tangents[n + 2]));
                    }
                } else {
                    if (meshAttributes.contains("TANGENT")) {
                        for (int i = 0; i < partVerticesCount; ++i) {
                            mesh.tangents.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
                        }
                    }
                }

                if (texcoords.size() == partVerticesCount * texCoordStride) {
                    for (int n = 0; n + 1 < texcoords.size(); n = n + 2) {
                        mesh.texCoords.push_back(glm::vec2(texcoords[n], texcoords[n + 1]));
                    }
                } else {
                    if (meshAttributes.contains("TEXCOORD_0")) {
                        for (int i = 0; i < partVerticesCount; ++i) {
                            mesh.texCoords.push_back(glm::vec2(0.0f, 0.0f));
                        }
                    }
                }

                if (texcoords2.size() == partVerticesCount * texCoord2Stride) {
                    for (int n = 0; n + 1 < texcoords2.size(); n = n + 2) {
                        mesh.texCoords1.push_back(glm::vec2(texcoords2[n], texcoords2[n + 1]));
                    }
                } else {
                    if (meshAttributes.contains("TEXCOORD_1")) {
                        for (int i = 0; i < partVerticesCount; ++i) {
                            mesh.texCoords1.push_back(glm::vec2(0.0f, 0.0f));
                        }
                    }
                }

                if (colors.size() == partVerticesCount * colorStride) {
                    for (int n = 0; n + 2 < colors.size(); n += colorStride) {
                        mesh.colors.push_back(ColorUtils::tosRGBVec3(glm::vec3(colors[n], colors[n + 1], colors[n + 2])));
                    }
                } else {
                    if (meshAttributes.contains("COLOR_0")) {
                        for (int i = 0; i < partVerticesCount; ++i) {
                            mesh.colors.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
                        }
                    }
                }

                if (joints.size() == partVerticesCount * jointStride) {
                    for (int n = 0; n < joints.size(); n += jointStride) {
                        clusterJoints.push_back(joints[n]);
                        if (jointStride > 1) {
                            clusterJoints.push_back(joints[n + 1]);
                            if (jointStride > 2) {
                                clusterJoints.push_back(joints[n + 2]);
                                if (jointStride > 3) {
                                    clusterJoints.push_back(joints[n + 3]);
                                } else {
                                    clusterJoints.push_back(0);
                                }
                            } else {
                                clusterJoints.push_back(0);
                                clusterJoints.push_back(0);
                            }
                        } else {
                            clusterJoints.push_back(0);
                            clusterJoints.push_back(0);
                            clusterJoints.push_back(0);
                        }
                    }
                } else {
                    if (meshAttributes.contains("JOINTS_0")) {
                        for (int i = 0; i < partVerticesCount; ++i) {
                            for (int j = 0; j < 4; ++j) {
                                clusterJoints.push_back(0);
                            }
                        }
                    }
                }

                if (weights.size() == partVerticesCount * weightStride) {
                    for (int n = 0; n + weightStride - 1 < weights.size(); n += weightStride) {
                        clusterWeights.push_back(weights[n]);
                        if (weightStride > 1) {
                            clusterWeights.push_back(weights[n + 1]);
                            if (weightStride > 2) {
                                clusterWeights.push_back(weights[n + 2]);
                                if (weightStride > 3) {
                                    clusterWeights.push_back(weights[n + 3]);
                                } else {
                                    clusterWeights.push_back(0.0f);
                                }
                            } else {
                                clusterWeights.push_back(0.0f);
                                clusterWeights.push_back(0.0f);
                            }
                        } else {
                            clusterWeights.push_back(0.0f);
                            clusterWeights.push_back(0.0f);
                            clusterWeights.push_back(0.0f);
                        }
                    }
                } else {
                    if (meshAttributes.contains("WEIGHTS_0")) {
                        for (int i = 0; i < partVerticesCount; ++i) {
                            clusterWeights.push_back(1.0f);
                            for (int j = 1; j < 4; ++j) {
                                clusterWeights.push_back(0.0f);
                            }
                        }
                    }
                }

                // Build weights (adapted from FBXSerializer.cpp)
                if (hfmModel.hasSkeletonJoints) {
                    int prevMeshClusterIndexCount = mesh.clusterIndices.count();
                    int prevMeshClusterWeightCount = mesh.clusterWeights.count();
                    const int WEIGHTS_PER_VERTEX = 4;
                    const float ALMOST_HALF = 0.499f;
                    int numVertices = mesh.vertices.size() - prevMeshVerticesCount;

                    // Append new cluster indices and weights for this mesh part
                    for (int i = 0; i < numVertices * WEIGHTS_PER_VERTEX; ++i) {
                        mesh.clusterIndices.push_back(mesh.clusters.size() - 1);
                        mesh.clusterWeights.push_back(0);
                    }

                    for (int c = 0; c < clusterJoints.size(); ++c) {
                        mesh.clusterIndices[prevMeshClusterIndexCount + c] =
                            originalToNewNodeIndexMap[_file.skins[node.skin].joints[clusterJoints[c]]];
                    }

                    // normalize and compress to 16-bits
                    for (int i = 0; i < numVertices; ++i) {
                        int j = i * WEIGHTS_PER_VERTEX;

                        float totalWeight = 0.0f;
                        for (int k = j; k < j + WEIGHTS_PER_VERTEX; ++k) {
                            totalWeight += clusterWeights[k];
                        }
                        if (totalWeight > 0.0f) {
                            float weightScalingFactor = (float)(UINT16_MAX) / totalWeight;
                            for (int k = j; k < j + WEIGHTS_PER_VERTEX; ++k) {
                                mesh.clusterWeights[prevMeshClusterWeightCount + k] = (uint16_t)(weightScalingFactor * clusterWeights[k] + ALMOST_HALF);
                            }
                        } else {
                            mesh.clusterWeights[prevMeshClusterWeightCount + j] = (uint16_t)((float)(UINT16_MAX) + ALMOST_HALF);
                        }
                        for (int clusterIndex = 0; clusterIndex < mesh.clusters.size() - 1; ++clusterIndex) {
                            ShapeVertices& points = hfmModel.shapeVertices.at(clusterIndex);
                            glm::vec3 globalMeshScale = extractScale(globalTransforms[nodeIndex]);
                            const glm::mat4 meshToJoint = glm::scale(glm::mat4(), globalMeshScale) * jointInverseBindTransforms[clusterIndex];

                            const float EXPANSION_WEIGHT_THRESHOLD = 0.25f;
                            if (mesh.clusterWeights[j] >= EXPANSION_WEIGHT_THRESHOLD) {
                                // TODO: fix transformed vertices being pushed back
                                auto& vertex = mesh.vertices[i];
                                const glm::mat4 vertexTransform = meshToJoint * (glm::translate(glm::mat4(), vertex));
                                glm::vec3 transformedVertex = hfmModel.joints[clusterIndex].translation * (extractTranslation(vertexTransform));
                                points.push_back(transformedVertex);
                            }
                        }
                    }
                }

                if (primitive.defined["material"]) {
                    part.materialID = materialIDs[primitive.material];
                }
                mesh.parts.push_back(part);

                // populate the texture coordinates if they don't exist
                if (mesh.texCoords.size() == 0 && !hfmModel.hasSkeletonJoints) {
                    for (int i = 0; i < part.triangleIndices.size(); ++i) {
                        mesh.texCoords.push_back(glm::vec2(0.0, 1.0));
                    }
                }

                // Build morph targets (blend shapes)
                if (!primitive.targets.isEmpty()) {

                    // Build list of blendshapes from FST and model.
                    typedef QPair<int, float> WeightedIndex;
                    hifi::VariantMultiHash blendshapeMappings = mapping.value("bs").toHash();
                    QMultiHash<QString, WeightedIndex> blendshapeIndices;
                    for (int i = 0;; ++i) {
                        auto blendshapeName = QString(BLENDSHAPE_NAMES[i]);
                        if (blendshapeName.isEmpty()) {
                            break;
                        }
                        auto mappings = blendshapeMappings.values(blendshapeName);
                        if (mappings.count() > 0) {
                            // Use blendshape from mapping.
                            foreach(const QVariant& mapping, mappings) {
                                auto blendshapeMapping = mapping.toList();
                                blendshapeIndices.insert(blendshapeMapping.at(0).toString(),
                                    WeightedIndex(i, blendshapeMapping.at(1).toFloat()));
                            }
                        } else {
                            // Use blendshape from model.
                            if (_file.meshes[node.mesh].extras.targetNames.contains(blendshapeName)) {
                                blendshapeIndices.insert(blendshapeName, WeightedIndex(i, 1.0f));
                            }
                        }
                    }

                    // If an FST isn't being used and the model is likely from ReadyPlayerMe, add blendshape synonyms.
                    auto fileTargetNames = _file.meshes[node.mesh].extras.targetNames;
                    bool likelyReadyPlayerMeFile =
                           fileTargetNames.contains("browOuterUpLeft")
                        && fileTargetNames.contains("browInnerUp")
                        && fileTargetNames.contains("browDownLeft")
                        && fileTargetNames.contains("eyeBlinkLeft")
                        && fileTargetNames.contains("eyeWideLeft")
                        && fileTargetNames.contains("mouthLeft")
                        && fileTargetNames.contains("viseme_O")
                        && fileTargetNames.contains("mouthShrugLower");
                    if (blendshapeMappings.count() == 0 && likelyReadyPlayerMeFile) {
                        QHash<QString, QPair<QString, float>>::const_iterator synonym
                            = READYPLAYERME_BLENDSHAPES_MAP.constBegin();
                        while (synonym != READYPLAYERME_BLENDSHAPES_MAP.constEnd()) {
                            if (fileTargetNames.contains(synonym.key())) {
                                auto blendshape = BLENDSHAPE_LOOKUP_MAP.find(synonym.value().first);
                                if (blendshape != BLENDSHAPE_LOOKUP_MAP.end()) {
                                    blendshapeIndices.insert(synonym.key(),
                                        WeightedIndex(blendshape.value(), synonym.value().second));
                                }
                            }
                            ++synonym;
                        }
                    }

                    // Create blendshapes.
                    if (!blendshapeIndices.isEmpty()) {
                        mesh.blendshapes.resize((int)Blendshapes::BlendshapeCount);
                    }
                    auto keys = blendshapeIndices.keys();
                    auto values = blendshapeIndices.values();
                    auto names = _file.meshes[node.mesh].extras.targetNames;

                    for (int weightedIndex = 0; weightedIndex < keys.size(); ++weightedIndex) {
                        float weight = 1.0f;
                        int indexFromMapping = weightedIndex;
                        int targetIndex = weightedIndex;
                        hfmModel.blendshapeChannelNames.push_back("target_" + QString::number(weightedIndex));

                        if (!names.isEmpty()) {
                            targetIndex = names.indexOf(keys[weightedIndex]);
                            if (targetIndex == -1) {
                                continue;  // Ignore blendshape targets not present in glTF file.
                            }
                            indexFromMapping = values[weightedIndex].first;
                            weight = values[weightedIndex].second;
                            hfmModel.blendshapeChannelNames[weightedIndex] = keys[weightedIndex];
                        }

                        HFMBlendshape& blendshape = mesh.blendshapes[indexFromMapping];
                        auto target = primitive.targets[targetIndex];

                        QVector<glm::vec3> normals;
                        QVector<glm::vec3> vertices;

                        if (target.values.contains((QString) "NORMAL")) {
                            generateTargetData(target.values.value((QString) "NORMAL"), weight, normals);
                        }
                        if (target.values.contains((QString) "POSITION")) {
                            generateTargetData(target.values.value((QString) "POSITION"), weight, vertices);
                        }

                        if (blendshape.indices.size() < prevMeshVerticesCount + vertices.size()) {
                            blendshape.indices.resize(prevMeshVerticesCount + vertices.size());
                            blendshape.vertices.resize(prevMeshVerticesCount + vertices.size());
                            blendshape.normals.resize(prevMeshVerticesCount + vertices.size());
                        }
                        for (int i = 0; i < vertices.size(); i++) {
                            blendshape.indices[prevMeshVerticesCount + i] = prevMeshVerticesCount + i;
                            blendshape.vertices[prevMeshVerticesCount + i] += vertices.value(i);
                            blendshape.normals[prevMeshVerticesCount + i] += normals.value(i);
                        }

                    }
                }

                foreach(const glm::vec3& vertex, mesh.vertices) {
                    glm::vec3 transformedVertex = glm::vec3(globalTransforms[nodeIndex] * glm::vec4(vertex, 1.0f));
                    mesh.meshExtents.addPoint(transformedVertex);
                    hfmModel.meshExtents.addPoint(transformedVertex);
                }
            }

            // Mesh extents must be at least a minimum size, in particular for blendshapes to work on planar meshes.
            const float MODEL_MIN_DIMENSION = 0.001f;
            auto delta = glm::max(glm::vec3(MODEL_MIN_DIMENSION) - mesh.meshExtents.size(), glm::vec3(0.0f)) / 2.0f;
            mesh.meshExtents.minimum -= delta;
            mesh.meshExtents.maximum += delta;
            hfmModel.meshExtents.minimum -= delta;
            hfmModel.meshExtents.maximum += delta;

            mesh.meshIndex = hfmModel.meshes.size();
        }
        ++nodecount;
    }

    return true;
}

MediaType GLTFSerializer::getMediaType() const {
    MediaType mediaType("gltf");
    mediaType.extensions.push_back("gltf");
    mediaType.webMediaTypes.push_back("model/gltf+json");

    mediaType.extensions.push_back("glb");
    mediaType.webMediaTypes.push_back("model/gltf-binary");

    return mediaType;
}

std::unique_ptr<hfm::Serializer::Factory> GLTFSerializer::getFactory() const {
    return std::make_unique<hfm::Serializer::SimpleFactory<GLTFSerializer>>();
}

HFMModel::Pointer GLTFSerializer::read(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url) {

    _url = url;

    // Normalize url for local files
    hifi::URL normalizeUrl = DependencyManager::get<ResourceManager>()->normalizeURL(_url);
    if (normalizeUrl.scheme().isEmpty() || (normalizeUrl.scheme() == "file")) {
        QString localFileName = PathUtils::expandToLocalDataAbsolutePath(normalizeUrl).toLocalFile();
        _url = hifi::URL(QFileInfo(localFileName).absoluteFilePath());
    }

    if (parseGLTF(data)) {
        //_file.dump();
        auto hfmModelPtr = std::make_shared<HFMModel>();
        HFMModel& hfmModel = *hfmModelPtr;
        buildGeometry(hfmModel, mapping, _url);

        //hfmModel.debugDump();
        //glTFDebugDump();

        return hfmModelPtr;
    } else {
        qCDebug(modelformat) << "Error parsing GLTF file.";
    }

    return nullptr;
}

bool GLTFSerializer::readBinary(const QString& url, hifi::ByteArray& outdata) {
    bool success;

    if (url.contains("data:application/octet-stream;base64,")) {
        outdata = requestEmbeddedData(url);
        success = !outdata.isEmpty();
    } else {
        hifi::URL binaryUrl = _url.resolved(url);
        std::tie<bool, hifi::ByteArray>(success, outdata) = requestData(binaryUrl);
    }

    return success;
}

bool GLTFSerializer::doesResourceExist(const QString& url) {
    if (_url.isEmpty()) {
        return false;
    }
    hifi::URL candidateUrl = _url.resolved(url);
    return DependencyManager::get<ResourceManager>()->resourceExists(candidateUrl);
}

std::tuple<bool, hifi::ByteArray> GLTFSerializer::requestData(hifi::URL& url) {
    auto request = DependencyManager::get<ResourceManager>()->createResourceRequest(
        nullptr, url, true, -1, "GLTFSerializer::requestData");

    if (!request) {
        return std::make_tuple(false, hifi::ByteArray());
    }

    QEventLoop loop;
    QObject::connect(request, &ResourceRequest::finished, &loop, &QEventLoop::quit);
    request->send();
    loop.exec();

    if (request->getResult() == ResourceRequest::Success) {
        return std::make_tuple(true, request->getData());
    } else {
        return std::make_tuple(false, hifi::ByteArray());
    }
}

hifi::ByteArray GLTFSerializer::requestEmbeddedData(const QString& url) {
    QString binaryUrl = url.split(",")[1];
    return binaryUrl.isEmpty() ? hifi::ByteArray() : QByteArray::fromBase64(binaryUrl.toUtf8());
}


QNetworkReply* GLTFSerializer::request(hifi::URL& url, bool isTest) {
    if (!qApp) {
        return nullptr;
    }
    bool aboutToQuit{ false };
    auto connection = QObject::connect(qApp, &QCoreApplication::aboutToQuit, [&] {
        aboutToQuit = true;
    });
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest netRequest(url);
    netRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply* netReply = isTest ? networkAccessManager.head(netRequest) : networkAccessManager.get(netRequest);
    if (!qApp || aboutToQuit) {
        netReply->deleteLater();
        return nullptr;
    }
    QEventLoop loop; // Create an event loop that will quit when we get the finished signal
    QObject::connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();                    // Nothing is going to happen on this whole run thread until we get this

    QObject::disconnect(connection);
    return netReply;                // trying to sync later on.
}

HFMTexture GLTFSerializer::getHFMTexture(const GLTFTexture& texture) {
    HFMTexture fbxtex = HFMTexture();
    fbxtex.texcoordSet = 0;

    if (texture.defined["source"]) {
        QString url = _file.images[texture.source].uri;

        QString fname = hifi::URL(url).fileName();
        hifi::URL textureUrl = _url.resolved(url);
        fbxtex.name = fname;
        fbxtex.filename = textureUrl.toEncoded();

        if (_url.path().endsWith("glb") && !_glbBinary.isEmpty()) {
            int bufferView = _file.images[texture.source].bufferView;

            GLTFBufferView& imagesBufferview = _file.bufferviews[bufferView];
            int offset = imagesBufferview.byteOffset;
            int length = imagesBufferview.byteLength;

            fbxtex.content = _glbBinary.mid(offset, length);
            fbxtex.filename = textureUrl.toEncoded().append(texture.source);
        }

        if (url.contains("data:image/jpeg;base64,") || url.contains("data:image/png;base64,")) {
            fbxtex.content = requestEmbeddedData(url);
        }
    }
    return fbxtex;
}

void GLTFSerializer::setHFMMaterial(HFMMaterial& hfmMat, const GLTFMaterial& material) {
    if (material.defined["alphaMode"]) {
        hfmMat._material->setOpacityMapMode(material.alphaMode);
    } else {
        hfmMat._material->setOpacityMapMode(graphics::MaterialKey::OPACITY_MAP_OPAQUE); // GLTF defaults to opaque
    }

    if (material.defined["alphaCutoff"]) {
        hfmMat._material->setOpacityCutoff(material.alphaCutoff);
    }

    if (material.defined["doubleSided"] && material.doubleSided) {
        hfmMat._material->setCullFaceMode(graphics::MaterialKey::CullFaceMode::CULL_NONE);
    }

    if (material.defined["emissiveFactor"] && material.emissiveFactor.size() == 3) {
        glm::vec3 emissive = glm::vec3(material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2]);
        hfmMat._material->setEmissive(emissive);
    }

    if (material.defined["emissiveTexture"]) {
        hfmMat.emissiveTexture = getHFMTexture(_file.textures[material.emissiveTexture]);
        hfmMat.useEmissiveMap = true;
    }

    if (material.defined["normalTexture"]) {
        hfmMat.normalTexture = getHFMTexture(_file.textures[material.normalTexture]);
        hfmMat.useNormalMap = true;
    }

    if (material.defined["occlusionTexture"]) {
        hfmMat.occlusionTexture = getHFMTexture(_file.textures[material.occlusionTexture]);
        hfmMat.useOcclusionMap = true;
    }

    if (material.defined["pbrMetallicRoughness"]) {
        hfmMat.isPBSMaterial = true;

        if (material.pbrMetallicRoughness.defined["metallicFactor"]) {
            hfmMat.metallic = material.pbrMetallicRoughness.metallicFactor;
            hfmMat._material->setMetallic(hfmMat.metallic);
        }
        if (material.pbrMetallicRoughness.defined["baseColorTexture"]) {
            hfmMat.opacityTexture = getHFMTexture(_file.textures[material.pbrMetallicRoughness.baseColorTexture]);
            hfmMat.albedoTexture = getHFMTexture(_file.textures[material.pbrMetallicRoughness.baseColorTexture]);
            hfmMat.useAlbedoMap = true;
        }
        if (material.pbrMetallicRoughness.defined["metallicRoughnessTexture"]) {
            hfmMat.roughnessTexture = getHFMTexture(_file.textures[material.pbrMetallicRoughness.metallicRoughnessTexture]);
            hfmMat.roughnessTexture.sourceChannel = image::ColorChannel::GREEN;
            hfmMat.useRoughnessMap = true;
            hfmMat.metallicTexture = getHFMTexture(_file.textures[material.pbrMetallicRoughness.metallicRoughnessTexture]);
            hfmMat.metallicTexture.sourceChannel = image::ColorChannel::BLUE;
            hfmMat.useMetallicMap = true;
        }
        if (material.pbrMetallicRoughness.defined["roughnessFactor"]) {
            hfmMat._material->setRoughness(material.pbrMetallicRoughness.roughnessFactor);
        }
        if (material.pbrMetallicRoughness.defined["baseColorFactor"] &&
            material.pbrMetallicRoughness.baseColorFactor.size() == 4) {
            glm::vec3 dcolor =
                glm::vec3(material.pbrMetallicRoughness.baseColorFactor[0], material.pbrMetallicRoughness.baseColorFactor[1],
                          material.pbrMetallicRoughness.baseColorFactor[2]);
            hfmMat.diffuseColor = dcolor;
            hfmMat._material->setAlbedo(dcolor);
            hfmMat._material->setOpacity(material.pbrMetallicRoughness.baseColorFactor[3]);
        }
    }

}

template<typename T, typename L>
bool GLTFSerializer::readArray(const hifi::ByteArray& bin, int byteOffset, int count,
                           QVector<L>& outarray, int accessorType, bool normalized) {

    QDataStream blobstream(bin);
    blobstream.setByteOrder(QDataStream::LittleEndian);
    blobstream.setVersion(QDataStream::Qt_5_9);
    blobstream.setFloatingPointPrecision(QDataStream::FloatingPointPrecision::SinglePrecision);
    blobstream.skipRawData(byteOffset);

    int bufferCount = 0;
    switch (accessorType) {
    case GLTFAccessorType::SCALAR:
        bufferCount = 1;
        break;
    case GLTFAccessorType::VEC2:
        bufferCount = 2;
        break;
    case GLTFAccessorType::VEC3:
        bufferCount = 3;
        break;
    case GLTFAccessorType::VEC4:
        bufferCount = 4;
        break;
    case GLTFAccessorType::MAT2:
        bufferCount = 4;
        break;
    case GLTFAccessorType::MAT3:
        bufferCount = 9;
        break;
    case GLTFAccessorType::MAT4:
        bufferCount = 16;
        break;
    default:
        qWarning(modelformat) << "Unknown accessorType: " << accessorType;
        blobstream.setDevice(nullptr);
        return false;
    }

    float scale = 1.0f;  // Normalized output values should always be floats.
    if (normalized) {
        scale = (float)(std::numeric_limits<T>::max)();
    }

    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < bufferCount; ++j) {
            if (!blobstream.atEnd()) {
                T value;
                blobstream >> value;
                if (normalized) {
                    outarray.push_back(std::max((float)value / scale, -1.0f));
                } else {
                    outarray.push_back(value);
                }
            } else {
                blobstream.setDevice(nullptr);
                return false;
            }
        }
    }

    blobstream.setDevice(nullptr);
    return true;
}
template<typename T>
bool GLTFSerializer::addArrayOfType(const hifi::ByteArray& bin, int byteOffset, int count,
                                QVector<T>& outarray, int accessorType, int componentType, bool normalized) {

    switch (componentType) {
    case GLTFAccessorComponentType::BYTE: {}
    case GLTFAccessorComponentType::UNSIGNED_BYTE: {
        return readArray<uchar>(bin, byteOffset, count, outarray, accessorType, normalized);
    }
    case GLTFAccessorComponentType::SHORT: {
        return readArray<short>(bin, byteOffset, count, outarray, accessorType, normalized);
    }
    case GLTFAccessorComponentType::UNSIGNED_INT: {
        return readArray<uint>(bin, byteOffset, count, outarray, accessorType, normalized);
    }
    case GLTFAccessorComponentType::UNSIGNED_SHORT: {
        return readArray<ushort>(bin, byteOffset, count, outarray, accessorType, normalized);
    }
    case GLTFAccessorComponentType::FLOAT: {
        return readArray<float>(bin, byteOffset, count, outarray, accessorType, normalized);
    }
    }
    return false;
}

template <typename T>
bool GLTFSerializer::addArrayFromAccessor(GLTFAccessor& accessor, QVector<T>& outarray) {
    bool success = true;

    if (accessor.defined["bufferView"]) {
        GLTFBufferView& bufferview = _file.bufferviews[accessor.bufferView];
        GLTFBuffer& buffer = _file.buffers[bufferview.buffer];

        int accBoffset = accessor.defined["byteOffset"] ? accessor.byteOffset : 0;

        success = addArrayOfType(buffer.blob, bufferview.byteOffset + accBoffset, accessor.count, outarray, accessor.type,
                                 accessor.componentType, accessor.normalized);
    } else {
        for (int i = 0; i < accessor.count; ++i) {
            T value;
            memset(&value, 0, sizeof(T));  // Make sure the dummy array is initialized to zero.
            outarray.push_back(value);
        }
    }

    if (success) {
        if (accessor.defined["sparse"]) {
            QVector<int> out_sparse_indices_array;

            GLTFBufferView& sparseIndicesBufferview = _file.bufferviews[accessor.sparse.indices.bufferView];
            GLTFBuffer& sparseIndicesBuffer = _file.buffers[sparseIndicesBufferview.buffer];

            int accSIBoffset = accessor.sparse.indices.defined["byteOffset"] ? accessor.sparse.indices.byteOffset : 0;

            success = addArrayOfType(sparseIndicesBuffer.blob, sparseIndicesBufferview.byteOffset + accSIBoffset,
                                     accessor.sparse.count, out_sparse_indices_array, GLTFAccessorType::SCALAR,
                                     accessor.sparse.indices.componentType, false);
            if (success) {
                QVector<T> out_sparse_values_array;

                GLTFBufferView& sparseValuesBufferview = _file.bufferviews[accessor.sparse.values.bufferView];
                GLTFBuffer& sparseValuesBuffer = _file.buffers[sparseValuesBufferview.buffer];

                int accSVBoffset = accessor.sparse.values.defined["byteOffset"] ? accessor.sparse.values.byteOffset : 0;

                success = addArrayOfType(sparseValuesBuffer.blob, sparseValuesBufferview.byteOffset + accSVBoffset,
                                         accessor.sparse.count, out_sparse_values_array, accessor.type, accessor.componentType,
                                         accessor.normalized);

                if (success) {
                    for (int i = 0; i < accessor.sparse.count; ++i) {
                        if ((i * 3) + 2 < out_sparse_values_array.size()) {
                            if ((out_sparse_indices_array[i] * 3) + 2 < outarray.length()) {
                                for (int j = 0; j < 3; ++j) {
                                    outarray[(out_sparse_indices_array[i] * 3) + j] = out_sparse_values_array[(i * 3) + j];
                                }
                            } else {
                                success = false;
                                break;
                            }
                        } else {
                            success = false;
                            break;
                        }
                    }
                }
            }
        }
    }

    return success;
}

void GLTFSerializer::retriangulate(const QVector<int>& inIndices, const QVector<glm::vec3>& in_vertices,
                               const QVector<glm::vec3>& in_normals, QVector<int>& outIndices,
                               QVector<glm::vec3>& out_vertices, QVector<glm::vec3>& out_normals) {
    for (int i = 0; i + 2 < inIndices.size(); i = i + 3) {

        int idx1 = inIndices[i];
        int idx2 = inIndices[i+1];
        int idx3 = inIndices[i+2];

        out_vertices.push_back(in_vertices[idx1]);
        out_vertices.push_back(in_vertices[idx2]);
        out_vertices.push_back(in_vertices[idx3]);

        out_normals.push_back(in_normals[idx1]);
        out_normals.push_back(in_normals[idx2]);
        out_normals.push_back(in_normals[idx3]);

        outIndices.push_back(i);
        outIndices.push_back(i+1);
        outIndices.push_back(i+2);
    }
}

void GLTFSerializer::glTFDebugDump() {
    qCDebug(modelformat) << "\n";
    qCDebug(modelformat) << "---------------- GLTF Model ----------------";

    qCDebug(modelformat) << "---------------- Nodes ----------------";
    for (GLTFNode node : _file.nodes) {
        if (node.defined["mesh"]) {
            qCDebug(modelformat) << "    node_transforms" << node.transforms;
        }
    }

    qCDebug(modelformat) << "---------------- Accessors ----------------";
    for (GLTFAccessor accessor : _file.accessors) {
        qCDebug(modelformat) << "count: " << accessor.count;
        qCDebug(modelformat) << "byteOffset: " << accessor.byteOffset;
    }

    qCDebug(modelformat) << "---------------- Textures ----------------";
    for (GLTFTexture texture : _file.textures) {
        if (texture.defined["source"]) {
            QString url = _file.images[texture.source].uri;
            QString fname = hifi::URL(url).fileName();
            qCDebug(modelformat) << "fname: " << fname;
        }
    }
}
