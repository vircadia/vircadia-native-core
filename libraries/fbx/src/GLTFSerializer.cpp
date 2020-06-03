//
//  GLTFSerializer.cpp
//  libraries/fbx/src
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

#include <unordered_set>
#include <map>
#include <qfile.h>
#include <qfileinfo.h>

#include <glm/gtc/type_ptr.hpp>

#include <shared/NsightHelpers.h>
#include <NetworkAccessManager.h>
#include <ResourceManager.h>
#include <PathUtils.h>
#include <image/ColorChannel.h>
#include <BlendshapeConstants.h>

#include "FBXSerializer.h"

#define GLTF_GET_INDICIES(accCount)           \
    int index1 = (indices[n + 0] * accCount); \
    int index2 = (indices[n + 1] * accCount); \
    int index3 = (indices[n + 2] * accCount);

#define GLTF_APPEND_ARRAY_1(newArray, oldArray) \
    GLTF_GET_INDICIES(1)                        \
    newArray.append(oldArray[index1]);          \
    newArray.append(oldArray[index2]);          \
    newArray.append(oldArray[index3]);

#define GLTF_APPEND_ARRAY_2(newArray, oldArray) \
    GLTF_GET_INDICIES(2)                        \
    newArray.append(oldArray[index1]);          \
    newArray.append(oldArray[index1 + 1]);      \
    newArray.append(oldArray[index2]);          \
    newArray.append(oldArray[index2 + 1]);      \
    newArray.append(oldArray[index3]);          \
    newArray.append(oldArray[index3 + 1]);

#define GLTF_APPEND_ARRAY_3(newArray, oldArray) \
    GLTF_GET_INDICIES(3)                        \
    newArray.append(oldArray[index1]);          \
    newArray.append(oldArray[index1 + 1]);      \
    newArray.append(oldArray[index1 + 2]);      \
    newArray.append(oldArray[index2]);          \
    newArray.append(oldArray[index2 + 1]);      \
    newArray.append(oldArray[index2 + 2]);      \
    newArray.append(oldArray[index3]);          \
    newArray.append(oldArray[index3 + 1]);      \
    newArray.append(oldArray[index3 + 2]);

#define GLTF_APPEND_ARRAY_4(newArray, oldArray) \
    GLTF_GET_INDICIES(4)                        \
    newArray.append(oldArray[index1]);          \
    newArray.append(oldArray[index1 + 1]);      \
    newArray.append(oldArray[index1 + 2]);      \
    newArray.append(oldArray[index1 + 3]);      \
    newArray.append(oldArray[index2]);          \
    newArray.append(oldArray[index2 + 1]);      \
    newArray.append(oldArray[index2 + 2]);      \
    newArray.append(oldArray[index2 + 3]);      \
    newArray.append(oldArray[index3]);          \
    newArray.append(oldArray[index3 + 1]);      \
    newArray.append(oldArray[index3 + 2]);      \
    newArray.append(oldArray[index3 + 3]);

bool GLTFSerializer::getStringVal(const QJsonObject& object,
                                  const QString& fieldname,
                                  QString& value,
                                  QMap<QString, bool>& defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isString());
    if (_defined) {
        value = object[fieldname].toString();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getBoolVal(const QJsonObject& object,
                                const QString& fieldname,
                                bool& value,
                                QMap<QString, bool>& defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isBool());
    if (_defined) {
        value = object[fieldname].toBool();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getIntVal(const QJsonObject& object, const QString& fieldname, int& value, QMap<QString, bool>& defined) {
    bool _defined = (object.contains(fieldname) && !object[fieldname].isNull());
    if (_defined) {
        value = object[fieldname].toInt();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getDoubleVal(const QJsonObject& object,
                                  const QString& fieldname,
                                  double& value,
                                  QMap<QString, bool>& defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isDouble());
    if (_defined) {
        value = object[fieldname].toDouble();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}
bool GLTFSerializer::getObjectVal(const QJsonObject& object,
                                  const QString& fieldname,
                                  QJsonObject& value,
                                  QMap<QString, bool>& defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isObject());
    if (_defined) {
        value = object[fieldname].toObject();
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getIntArrayVal(const QJsonObject& object,
                                    const QString& fieldname,
                                    QVector<int>& values,
                                    QMap<QString, bool>& defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isArray());
    if (_defined) {
        QJsonArray arr = object[fieldname].toArray();
        for (const QJsonValue& v : arr) {
            if (!v.isNull()) {
                values.push_back(v.toInt());
            }
        }
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getDoubleArrayVal(const QJsonObject& object,
                                       const QString& fieldname,
                                       QVector<double>& values,
                                       QMap<QString, bool>& defined) {
    bool _defined = (object.contains(fieldname) && object[fieldname].isArray());
    if (_defined) {
        QJsonArray arr = object[fieldname].toArray();
        for (const QJsonValue& v : arr) {
            if (v.isDouble()) {
                values.push_back(v.toDouble());
            }
        }
    }
    defined.insert(fieldname, _defined);
    return _defined;
}

bool GLTFSerializer::getObjectArrayVal(const QJsonObject& object,
                                       const QString& fieldname,
                                       QJsonArray& objects,
                                       QMap<QString, bool>& defined) {
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

int GLTFSerializer::getMeshPrimitiveRenderingMode(const QString& type) {
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

int GLTFSerializer::getAccessorType(const QString& type) {
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

int GLTFSerializer::getCameraType(const QString& type) {
    if (type == "orthographic") {
        return GLTFCameraTypes::ORTHOGRAPHIC;
    }
    if (type == "perspective") {
        return GLTFCameraTypes::PERSPECTIVE;
    }
    return GLTFCameraTypes::PERSPECTIVE;
}

int GLTFSerializer::getImageMimeType(const QString& mime) {
    if (mime == "image/jpeg") {
        return GLTFImageMimetype::JPEG;
    }
    if (mime == "image/png") {
        return GLTFImageMimetype::PNG;
    }
    return GLTFImageMimetype::JPEG;
}

int GLTFSerializer::getAnimationSamplerInterpolation(const QString& interpolation) {
    if (interpolation == "LINEAR") {
        return GLTFAnimationSamplerInterpolation::LINEAR;
    }
    return GLTFAnimationSamplerInterpolation::LINEAR;
}

bool GLTFSerializer::setAsset(const QJsonObject& object) {
    QJsonObject jsAsset;
    bool isAssetDefined = getObjectVal(object, "asset", jsAsset, _file.defined);
    if (isAssetDefined) {
        if (!getStringVal(jsAsset, "version", _file.asset.version, _file.asset.defined) || _file.asset.version != "2.0") {
            return false;
        }
        getStringVal(jsAsset, "generator", _file.asset.generator, _file.asset.defined);
        getStringVal(jsAsset, "copyright", _file.asset.copyright, _file.asset.defined);
    }
    return isAssetDefined;
}

GLTFAccessor::GLTFAccessorSparse::GLTFAccessorSparseIndices GLTFSerializer::createAccessorSparseIndices(
    const QJsonObject& object) {
    GLTFAccessor::GLTFAccessorSparse::GLTFAccessorSparseIndices accessorSparseIndices;

    getIntVal(object, "bufferView", accessorSparseIndices.bufferView, accessorSparseIndices.defined);
    getIntVal(object, "byteOffset", accessorSparseIndices.byteOffset, accessorSparseIndices.defined);
    getIntVal(object, "componentType", accessorSparseIndices.componentType, accessorSparseIndices.defined);

    return accessorSparseIndices;
}

GLTFAccessor::GLTFAccessorSparse::GLTFAccessorSparseValues GLTFSerializer::createAccessorSparseValues(
    const QJsonObject& object) {
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
        for (const QJsonValue& v : channels) {
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
        for (const QJsonValue& v : samplers) {
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

bool GLTFSerializer::getIndexFromObject(const QJsonObject& object,
                                        const QString& field,
                                        int& outidx,
                                        QMap<QString, bool>& defined) {
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
        getDoubleArrayVal(jsMetallicRoughness, "baseColorFactor", material.pbrMetallicRoughness.baseColorFactor,
                          material.pbrMetallicRoughness.defined);
        getIndexFromObject(jsMetallicRoughness, "baseColorTexture", material.pbrMetallicRoughness.baseColorTexture,
                           material.pbrMetallicRoughness.defined);
        getDoubleVal(jsMetallicRoughness, "metallicFactor", material.pbrMetallicRoughness.metallicFactor,
                     material.pbrMetallicRoughness.defined);
        getDoubleVal(jsMetallicRoughness, "roughnessFactor", material.pbrMetallicRoughness.roughnessFactor,
                     material.pbrMetallicRoughness.defined);
        getIndexFromObject(jsMetallicRoughness, "metallicRoughnessTexture",
                           material.pbrMetallicRoughness.metallicRoughnessTexture, material.pbrMetallicRoughness.defined);
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
        for (const QJsonValue& prim : jsPrimitives) {
            if (prim.isObject()) {
                GLTFMeshPrimitive primitive;
                QJsonObject jsPrimitive = prim.toObject();
                getIntVal(jsPrimitive, "mode", primitive.mode, primitive.defined);
                getIntVal(jsPrimitive, "indices", primitive.indices, primitive.defined);
                getIntVal(jsPrimitive, "material", primitive.material, primitive.defined);

                QJsonObject jsAttributes;
                if (getObjectVal(jsPrimitive, "attributes", jsAttributes, primitive.defined)) {
                    QStringList attrKeys = jsAttributes.keys();
                    for (const QString& attrKey : attrKeys) {
                        int attrVal;
                        getIntVal(jsAttributes, attrKey, attrVal, primitive.attributes.defined);
                        primitive.attributes.values.insert(attrKey, attrVal);
                    }
                }

                QJsonArray jsTargets;
                if (getObjectArrayVal(jsPrimitive, "targets", jsTargets, primitive.defined)) {
                    for (const QJsonValue& tar : jsTargets) {
                        if (tar.isObject()) {
                            QJsonObject jsTarget = tar.toObject();
                            QStringList tarKeys = jsTarget.keys();
                            GLTFMeshPrimitiveAttr target;
                            for (const QString& tarKey : tarKeys) {
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
            foreach (const QJsonValue& tarName, jsTargetNames) { extras.targetNames.push_back(tarName.toString()); }
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
            for (const QJsonValue& accVal : accessors) {
                if (accVal.isObject()) {
                    success = success && addAccessor(accVal.toObject());
                }
            }
        }

        QJsonArray animations;
        if (getObjectArrayVal(jsFile, "animations", animations, _file.defined)) {
            for (const QJsonValue& animVal : accessors) {
                if (animVal.isObject()) {
                    success = success && addAnimation(animVal.toObject());
                }
            }
        }

        QJsonArray bufferViews;
        if (getObjectArrayVal(jsFile, "bufferViews", bufferViews, _file.defined)) {
            for (const QJsonValue& bufviewVal : bufferViews) {
                if (bufviewVal.isObject()) {
                    success = success && addBufferView(bufviewVal.toObject());
                }
            }
        }

        QJsonArray buffers;
        if (getObjectArrayVal(jsFile, "buffers", buffers, _file.defined)) {
            for (const QJsonValue& bufVal : buffers) {
                if (bufVal.isObject()) {
                    success = success && addBuffer(bufVal.toObject());
                }
            }
        }

        QJsonArray cameras;
        if (getObjectArrayVal(jsFile, "cameras", cameras, _file.defined)) {
            for (const QJsonValue& camVal : cameras) {
                if (camVal.isObject()) {
                    success = success && addCamera(camVal.toObject());
                }
            }
        }

        QJsonArray images;
        if (getObjectArrayVal(jsFile, "images", images, _file.defined)) {
            for (const QJsonValue& imgVal : images) {
                if (imgVal.isObject()) {
                    success = success && addImage(imgVal.toObject());
                }
            }
        }

        QJsonArray materials;
        if (getObjectArrayVal(jsFile, "materials", materials, _file.defined)) {
            for (const QJsonValue& matVal : materials) {
                if (matVal.isObject()) {
                    success = success && addMaterial(matVal.toObject());
                }
            }
        }

        QJsonArray meshes;
        if (getObjectArrayVal(jsFile, "meshes", meshes, _file.defined)) {
            for (const QJsonValue& meshVal : meshes) {
                if (meshVal.isObject()) {
                    success = success && addMesh(meshVal.toObject());
                }
            }
        }

        QJsonArray nodes;
        if (getObjectArrayVal(jsFile, "nodes", nodes, _file.defined)) {
            for (const QJsonValue& nodeVal : nodes) {
                if (nodeVal.isObject()) {
                    success = success && addNode(nodeVal.toObject());
                }
            }
        }

        QJsonArray samplers;
        if (getObjectArrayVal(jsFile, "samplers", samplers, _file.defined)) {
            for (const QJsonValue& samVal : samplers) {
                if (samVal.isObject()) {
                    success = success && addSampler(samVal.toObject());
                }
            }
        }

        QJsonArray scenes;
        if (getObjectArrayVal(jsFile, "scenes", scenes, _file.defined)) {
            for (const QJsonValue& sceneVal : scenes) {
                if (sceneVal.isObject()) {
                    success = success && addScene(sceneVal.toObject());
                }
            }
        }

        QJsonArray skins;
        if (getObjectArrayVal(jsFile, "skins", skins, _file.defined)) {
            for (const QJsonValue& skinVal : skins) {
                if (skinVal.isObject()) {
                    success = success && addSkin(skinVal.toObject());
                }
            }
        }

        QJsonArray textures;
        if (getObjectArrayVal(jsFile, "textures", textures, _file.defined)) {
            for (const QJsonValue& texVal : textures) {
                if (texVal.isObject()) {
                    success = success && addTexture(texVal.toObject());
                }
            }
        }
    }
    return success;
}

const glm::mat4& GLTFSerializer::getModelTransform(const GLTFNode& node) {
    return node.transform;
}

void GLTFSerializer::getSkinInverseBindMatrices(std::vector<std::vector<float>>& inverseBindMatrixValues) {
    for (auto& skin : _file.skins) {
        GLTFAccessor& indicesAccessor = _file.accessors[skin.inverseBindMatrices];
        QVector<float> matrices;
        addArrayFromAccessor(indicesAccessor, matrices);
        inverseBindMatrixValues.push_back(matrices.toStdVector());
    }
}

void GLTFSerializer::generateTargetData(int index, float weight, QVector<glm::vec3>& returnVector) {
    GLTFAccessor& accessor = _file.accessors[index];
    QVector<float> storedValues;
    addArrayFromAccessor(accessor, storedValues);
    for (int n = 0; n < storedValues.size(); n = n + 3) {
        returnVector.push_back(glm::vec3(weight * storedValues[n], weight * storedValues[n + 1], weight * storedValues[n + 2]));
    }
}

namespace gltf {

using ParentIndexMap = std::unordered_map<int, int>;
ParentIndexMap findParentIndices(const QVector<GLTFNode>& nodes) {
    ParentIndexMap parentIndices;
    int numNodes = nodes.size();
    for (int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex) {
        auto& gltfNode = nodes[nodeIndex];
        for (const auto& childIndex : gltfNode.children) {
            parentIndices[childIndex] = nodeIndex;
        }
    }
    return parentIndices;
}

bool requiresNodeReordering(const ParentIndexMap& map) {
    for (const auto& entry : map) {
        if (entry.first < entry.second) {
            return true;
        }
    }
    return false;
}

int findEdgeCount(const ParentIndexMap& parentIndices, int nodeIndex) {
    auto parentsEnd = parentIndices.end();
    ParentIndexMap::const_iterator itr;
    int result = 0;
    while (parentsEnd != (itr = parentIndices.find(nodeIndex))) {
        nodeIndex = itr->second;
        ++result;
    }
    return result;
}

using IndexBag = std::unordered_set<int>;
using EdgeCountMap = std::map<int, IndexBag>;
EdgeCountMap findEdgeCounts(int numNodes, const ParentIndexMap& map) {
    EdgeCountMap edgeCounts;
    // For each item, determine how many tranversals to a root node
    for (int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex) {
        // How many steps between this node and a root node?
        int edgeCount = findEdgeCount(map, nodeIndex);
        // Populate the result map
        edgeCounts[edgeCount].insert(nodeIndex);
    }
    return edgeCounts;
}

using ReorderMap = std::unordered_map<int, int>;
ReorderMap buildReorderMap(const EdgeCountMap& map) {
    ReorderMap result;
    int newIndex = 0;
    for (const auto& entry : map) {
        const IndexBag& oldIndices = entry.second;
        for (const auto& oldIndex : oldIndices) {
            result.insert({ oldIndex, newIndex });
            ++newIndex;
        }
    }
    return result;
}

void reorderNodeIndices(QVector<int>& indices, const ReorderMap& oldToNewIndexMap) {
    for (auto& index : indices) {
        index = oldToNewIndexMap.at(index);
    }
}

}  // namespace gltf

void GLTFFile::populateMaterialNames() {
    // Build material names
    QSet<QString> usedNames;
    for (const auto& material : materials) {
        if (!material.name.isEmpty()) {
            usedNames.insert(material.name);
        }
    }

    int ukcount = 0;
    const QString unknown{ "Default_%1" };
    for (auto& material : materials) {
        QString generatedName = unknown.arg(ukcount++);
        while (usedNames.contains(generatedName)) {
            generatedName = unknown.arg(ukcount++);
        }
        material.name = generatedName;
        material.defined.insert("name", true);
        usedNames.insert(generatedName);
    }
}

void GLTFFile::reorderNodes(const std::unordered_map<int, int>& oldToNewIndexMap) {
    int numNodes = nodes.size();
    assert(numNodes == oldToNewIndexMap.size());
    QVector<GLTFNode> newNodes;
    newNodes.resize(numNodes);
    for (int oldIndex = 0; oldIndex < numNodes; ++oldIndex) {
        const auto& oldNode = nodes[oldIndex];
        int newIndex = oldToNewIndexMap.at(oldIndex);
        auto& newNode = newNodes[newIndex];
        // Write the new node
        newNode = oldNode;
        // Fixup the child indices
        gltf::reorderNodeIndices(newNode.children, oldToNewIndexMap);
    }
    newNodes.swap(nodes);

    for (auto& subScene : scenes) {
        gltf::reorderNodeIndices(subScene.nodes, oldToNewIndexMap);
    }
}

// Ensure that the GLTF nodes are ordered so
void GLTFFile::sortNodes() {
    // Find all the parents
    auto parentIndices = gltf::findParentIndices(nodes);
    // If the nodes are already in a good order, we're done
    if (!gltf::requiresNodeReordering(parentIndices)) {
        return;
    }

    auto edgeCounts = gltf::findEdgeCounts(nodes.size(), parentIndices);
    auto oldToNewIndexMap = gltf::buildReorderMap(edgeCounts);
    reorderNodes(oldToNewIndexMap);
    assert(!gltf::requiresNodeReordering(gltf::findParentIndices(nodes)));
}

void GLTFNode::normalizeTransform() {
    if (defined["matrix"] && matrix.size() == 16) {
        transform = glm::make_mat4(matrix.constData());
    } else {
        transform = glm::mat4(1.0);
        if (defined["scale"] && scale.size() == 3) {
            glm::vec3 scaleVec = glm::make_vec3(scale.data());
            transform = glm::scale(transform, scaleVec);
        }

        if (defined["rotation"] && rotation.size() == 4) {
            glm::quat rotQ = glm::make_quat(rotation.data());
            transform = glm::mat4_cast(rotQ) * transform;
        }

        if (defined["translation"] && translation.size() == 3) {
            glm::vec3 transV = glm::make_vec3(translation.data());
            transform = glm::translate(glm::mat4(1.0), transV) * transform;
        }
    }
}

void GLTFFile::normalizeNodeTransforms() {
    for (auto& node : nodes) {
        node.normalizeTransform();
    }
}

bool GLTFSerializer::buildGeometry(HFMModel& hfmModel, const hifi::VariantHash& mapping, const hifi::URL& url) {
    int numNodes = _file.nodes.size();
    
    // Build joints
    hfmModel.jointIndices["x"] = numNodes;
    auto parentIndices = gltf::findParentIndices(_file.nodes);
    const auto parentsEnd = parentIndices.end();
    for (int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex) {
        HFMJoint joint;
        auto& node = _file.nodes[nodeIndex];
        auto parentItr = parentIndices.find(nodeIndex);
        if (parentsEnd == parentItr) {
            joint.parentIndex = -1;
        } else {
            joint.parentIndex = parentItr->second;
        }

        joint.transform = getModelTransform(node);
        joint.translation = extractTranslation(joint.transform);
        joint.rotation = glmExtractRotation(joint.transform);
        glm::vec3 scale = extractScale(joint.transform);
        joint.postTransform = glm::scale(glm::mat4(), scale);

        joint.globalTransform = joint.transform;
        // Nodes are sorted, so we can apply the full transform just by getting the global transform of the already defined parent
        if (joint.parentIndex != -1 && joint.parentIndex != nodeIndex) {
            const auto& parentJoint = hfmModel.joints[(size_t)joint.parentIndex];
            joint.transform = parentJoint.transform * joint.transform;
            joint.globalTransform = joint.globalTransform * parentJoint.globalTransform;
        } else {
            joint.globalTransform = hfmModel.offset * joint.globalTransform;
        }

        joint.name = node.name;
        joint.isSkeletonJoint = false;
        hfmModel.joints.push_back(joint);
    }

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
    // Lookup between the GLTF mesh and the skin
    std::vector<int> gltfMeshToSkin;
    gltfMeshToSkin.resize(_file.meshes.size(), -1);

    hfmModel.hasSkeletonJoints = !_file.skins.isEmpty();
    if (hfmModel.hasSkeletonJoints) {
        std::vector<std::vector<float>> inverseBindValues;
        getSkinInverseBindMatrices(inverseBindValues);

        for (int jointIndex = 0; jointIndex < numNodes; ++jointIndex) {
            int nodeIndex = jointIndex;
            auto& joint = hfmModel.joints[jointIndex];

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
        }

        std::vector<int> skinToRootJoint;
        skinToRootJoint.resize(_file.skins.size(), 0);
        for (int jointIndex = 0; jointIndex < numNodes; ++jointIndex) {
            const auto& node = _file.nodes[jointIndex];
            if (node.skin != -1) {
                skinToRootJoint[node.skin] = jointIndex;
                if (node.mesh != -1) {
                    gltfMeshToSkin[node.mesh] = node.skin;
                }
            }
        }

        for (int skinIndex = 0; skinIndex < _file.skins.size(); ++skinIndex) {
            const auto& skin = _file.skins[skinIndex];
            hfmModel.skinDeformers.emplace_back();
            auto& skinDeformer = hfmModel.skinDeformers.back();

            // Add the nodes being referred to for skinning
            for (int skinJointIndex : skin.joints) {
                hfm::Cluster cluster;
                cluster.jointIndex = skinJointIndex;
                cluster.inverseBindMatrix = jointInverseBindTransforms[skinJointIndex];
                cluster.inverseBindTransform = Transform(cluster.inverseBindMatrix);
                skinDeformer.clusters.push_back(cluster);
            }
            
            // Always append a cluster referring to the root joint at the end
            int rootJointIndex = skinToRootJoint[skinIndex];
            hfm::Cluster root;
            root.jointIndex = rootJointIndex;
            root.inverseBindMatrix = jointInverseBindTransforms[root.jointIndex];
            root.inverseBindTransform = Transform(root.inverseBindMatrix);
            skinDeformer.clusters.push_back(root);
        }
    }

    for (const auto& material : _file.materials) {
        const QString& matid = material.name;
        hfmModel.materials.emplace_back();
        HFMMaterial& hfmMaterial = hfmModel.materials.back();
        hfmMaterial._material = std::make_shared<graphics::Material>();
        hfmMaterial.materialID =  matid;
        setHFMMaterial(hfmMaterial, material);
    }


    int gltfMeshCount = _file.meshes.size();
    hfmModel.meshExtents.reset();
    std::vector<std::vector<hfm::Shape>> templateShapePerPrimPerGLTFMesh;
    for (int gltfMeshIndex = 0; gltfMeshIndex < gltfMeshCount; ++gltfMeshIndex) {
        const auto& gltfMesh = _file.meshes[gltfMeshIndex];
        hfmModel.meshes.emplace_back();
        // NOTE: The number of hfm meshes may be greater than the number of gltf meshes, if a gltf mesh has primitives with different vertex attributes. In that case, this mesh reference may be reassigned.
        hfm::Mesh* meshPtr = &hfmModel.meshes.back();
        const size_t firstMeshIndexForGLTFMesh = hfmModel.meshes.size() - 1;
        meshPtr->meshIndex = gltfMeshIndex;
        templateShapePerPrimPerGLTFMesh.emplace_back();
        std::vector<hfm::Shape>& templateShapePerPrim = templateShapePerPrimPerGLTFMesh.back();

        QSet<QString> primitiveAttributes;
        if (!gltfMesh.primitives.empty()) {
            for (const auto& attribute : gltfMesh.primitives[0].attributes.values.keys()) {
                primitiveAttributes.insert(attribute);
            }
        }
        std::vector<QSet<QString>> primitiveAttributeVariants;

        int primCount = (int)gltfMesh.primitives.size();
        size_t hfmMeshIndex = firstMeshIndexForGLTFMesh;
        for(int primIndex = 0; primIndex < primCount; ++primIndex) {
            auto& primitive = gltfMesh.primitives[primIndex];

            QList<QString> keys = primitive.attributes.values.keys();
            QSet<QString> newPrimitiveAttributes;
            for (const auto& key : keys) {
                newPrimitiveAttributes.insert(key);
            }
            if (newPrimitiveAttributes != primitiveAttributes) {
                assert(primIndex != 0);

                // We need to use a different mesh because the vertex attributes are different
                auto attributeVariantIt = std::find(primitiveAttributeVariants.cbegin(), primitiveAttributeVariants.cend(), newPrimitiveAttributes);
                if (attributeVariantIt == primitiveAttributeVariants.cend()) {
                    // Need to allocate a new mesh
                    hfmModel.meshes.emplace_back();
                    meshPtr = &hfmModel.meshes.back();
                    hfmMeshIndex = hfmModel.meshes.size() - 1;
                    meshPtr->meshIndex = gltfMeshIndex;
                    primitiveAttributeVariants.push_back(newPrimitiveAttributes);
                } else {
                    // An hfm mesh already exists for this gltf mesh with the same vertex attributes. Use it again.
                    auto variantIndex = (size_t)(attributeVariantIt - primitiveAttributeVariants.cbegin());
                    hfmMeshIndex = firstMeshIndexForGLTFMesh + variantIndex;
                    meshPtr = &hfmModel.meshes[hfmMeshIndex];
                }
                primitiveAttributes = newPrimitiveAttributes;
            }
            // Now, allocate the part for the correct mesh...
            hfm::Mesh& mesh = *meshPtr;
            mesh.parts.emplace_back();
            hfm::MeshPart& part = mesh.parts.back();
            // ...and keep track of the relationship between the gltf mesh/primitive and the hfm mesh/part
            templateShapePerPrim.emplace_back();
            hfm::Shape& templateShape = templateShapePerPrim.back();
            templateShape.mesh = (uint32_t)hfmMeshIndex;
            templateShape.meshPart = (uint32_t)(mesh.parts.size() - 1);
            templateShape.material = primitive.material;

            int indicesAccessorIdx = primitive.indices;

            GLTFAccessor& indicesAccessor = _file.accessors[indicesAccessorIdx];

            // Buffers
            constexpr int VERTEX_STRIDE = 3;
            constexpr int NORMAL_STRIDE = 3;
            constexpr int TEX_COORD_STRIDE = 2;

            QVector<int> indices;
            QVector<float> vertices;
            QVector<float> normals;
            QVector<float> tangents;
            QVector<float> texcoords;
            QVector<float> texcoords2;
            QVector<float> colors;
            QVector<uint16_t> joints;
            QVector<float> weights;

            static int tangentStride = 4;
            static int colorStride = 3;
            static int jointStride = 4;
            static int weightStride = 4;

            bool success = addArrayFromAccessor(indicesAccessor, indices);

            if (!success) {
                qWarning(modelformat) << "There was a problem reading glTF INDICES data for model " << _url;
                continue;
            }

            // Increment the triangle indices by the current mesh vertex count so each mesh part can all reference the same buffers within the mesh
            int prevMeshVerticesCount = mesh.vertices.count();
            QVector<uint16_t> clusterJoints;
            QVector<float> clusterWeights;

            for(auto &key : keys) {
                int accessorIdx = primitive.attributes.values[key];
                GLTFAccessor& accessor = _file.accessors[accessorIdx];
                const auto vertexAttribute = GLTFVertexAttribute::fromString(key);
                switch (vertexAttribute) {
                    case GLTFVertexAttribute::POSITION:
                        success = addArrayFromAttribute(vertexAttribute, accessor, vertices);
                        break;

                    case GLTFVertexAttribute::NORMAL:
                        success = addArrayFromAttribute(vertexAttribute, accessor, normals);
                        break;

                    case GLTFVertexAttribute::TANGENT:
                        success = addArrayFromAttribute(vertexAttribute, accessor, tangents);
                        tangentStride = GLTFAccessorType::count((GLTFAccessorType::Value)accessor.type);
                        break;

                    case GLTFVertexAttribute::TEXCOORD_0:
                        success = addArrayFromAttribute(vertexAttribute, accessor, texcoords);
                        break;

                    case GLTFVertexAttribute::TEXCOORD_1:
                        success = addArrayFromAttribute(vertexAttribute, accessor, texcoords2);
                        break;

                    case GLTFVertexAttribute::COLOR_0:
                        success = addArrayFromAttribute(vertexAttribute, accessor, colors);
                        colorStride = GLTFAccessorType::count((GLTFAccessorType::Value)accessor.type);
                        break;

                    case GLTFVertexAttribute::JOINTS_0:
                        success = addArrayFromAttribute(vertexAttribute, accessor, joints);
                        jointStride = GLTFAccessorType::count((GLTFAccessorType::Value)accessor.type);
                        break;

                    case GLTFVertexAttribute::WEIGHTS_0:
                        success = addArrayFromAttribute(vertexAttribute, accessor, weights);
                        weightStride = GLTFAccessorType::count((GLTFAccessorType::Value)accessor.type);
                        break;

                    default:
                        success = false;
                        break;
                }
                if (!success) {
                    continue;
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
                qWarning(modelformat) << "Indices out of range for model " << _url;
                continue;
            }

            part.triangleIndices.append(validatedIndices);

            mesh.vertices.reserve(mesh.vertices.size() + partVerticesCount);
            for (int n = 0; n < vertices.size(); n = n + VERTEX_STRIDE) {
                mesh.vertices.push_back(glm::vec3(vertices[n], vertices[n + 1], vertices[n + 2]));
            }

            mesh.normals.reserve(mesh.normals.size() + partVerticesCount);
            for (int n = 0; n < normals.size(); n = n + NORMAL_STRIDE) {
                mesh.normals.push_back(glm::vec3(normals[n], normals[n + 1], normals[n + 2]));
            }

            if (tangents.size() == partVerticesCount * tangentStride) {
                mesh.tangents.reserve(mesh.tangents.size() + partVerticesCount);
                for (int n = 0; n < tangents.size(); n += tangentStride) {
                    float tanW = tangentStride == 4 ? tangents[n + 3] : 1;
                    mesh.tangents.push_back(glm::vec3(tanW * tangents[n], tangents[n + 1], tanW * tangents[n + 2]));
                }
            }

            if (texcoords.size() == partVerticesCount * TEX_COORD_STRIDE) {
                mesh.texCoords.reserve(mesh.texCoords.size() + partVerticesCount);
                for (int n = 0; n < texcoords.size(); n = n + 2) {
                    mesh.texCoords.push_back(glm::vec2(texcoords[n], texcoords[n + 1]));
                }
            } else if (primitiveAttributes.contains("TEXCOORD_0")) {
                mesh.texCoords.resize(mesh.texCoords.size() + partVerticesCount);
            }

            if (texcoords2.size() == partVerticesCount * TEX_COORD_STRIDE) {
                mesh.texCoords1.reserve(mesh.texCoords1.size() + partVerticesCount);
                for (int n = 0; n < texcoords2.size(); n = n + 2) {
                    mesh.texCoords1.push_back(glm::vec2(texcoords2[n], texcoords2[n + 1]));
                }
            } else if (primitiveAttributes.contains("TEXCOORD_1")) {
                mesh.texCoords1.resize(mesh.texCoords1.size() + partVerticesCount);
            }

            if (colors.size() == partVerticesCount * colorStride) {
                mesh.colors.reserve(mesh.colors.size() + partVerticesCount);
                for (int n = 0; n < colors.size(); n += colorStride) {
                    mesh.colors.push_back(glm::vec3(colors[n], colors[n + 1], colors[n + 2]));
                }
            } else if (primitiveAttributes.contains("COLOR_0")) {
                mesh.colors.reserve(mesh.colors.size() + partVerticesCount);
                for (int i = 0; i < partVerticesCount; ++i) {
                    mesh.colors.push_back(glm::vec3(1.0f));
                }
            }

            const int WEIGHTS_PER_VERTEX = 4;

            if (weights.size() == partVerticesCount * weightStride) {
                for (int n = 0; n < weights.size(); n += weightStride) {
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
            } else if (primitiveAttributes.contains("WEIGHTS_0")) {
                for (int i = 0; i < partVerticesCount; ++i) {
                    clusterWeights.push_back(1.0f);
                    for (int j = 0; j < WEIGHTS_PER_VERTEX; ++j) {
                        clusterWeights.push_back(0.0f);
                    }
                }
            }

            // Compress floating point weights to uint16_t for graphics runtime
            // TODO: If the GLTF skinning weights are already in integer format, we should just copy the data
            if (!clusterWeights.empty()) {
                size_t numWeights = 4 * (mesh.vertices.size() - (uint32_t)prevMeshVerticesCount);
                size_t newWeightsStart = mesh.clusterWeights.size();
                size_t newWeightsEnd = newWeightsStart + numWeights;
                mesh.clusterWeights.reserve(newWeightsEnd);
                for (int weightIndex = 0; weightIndex < clusterWeights.size(); ++weightIndex) {
                    // Per the GLTF specification
                    uint16_t weight = std::round(clusterWeights[weightIndex] * 65535.0f);
                    mesh.clusterWeights.push_back(weight);
                }
                mesh.clusterWeightsPerVertex = WEIGHTS_PER_VERTEX;
            }

            if (joints.size() == partVerticesCount * jointStride) {
                for (int n = 0; n < joints.size(); n += jointStride) {
                    mesh.clusterIndices.push_back(joints[n]);
                    if (jointStride > 1) {
                        mesh.clusterIndices.push_back(joints[n + 1]);
                        if (jointStride > 2) {
                            mesh.clusterIndices.push_back(joints[n + 2]);
                            if (jointStride > 3) {
                                mesh.clusterIndices.push_back(joints[n + 3]);
                            } else {
                                mesh.clusterIndices.push_back(0);
                            }
                        } else {
                            mesh.clusterIndices.push_back(0);
                            mesh.clusterIndices.push_back(0);
                        }
                    } else {
                        mesh.clusterIndices.push_back(0);
                        mesh.clusterIndices.push_back(0);
                        mesh.clusterIndices.push_back(0);
                    }
                }
            } else if (primitiveAttributes.contains("JOINTS_0")) {
                for (int i = 0; i < partVerticesCount; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        mesh.clusterIndices.push_back(0);
                    }
                }
            }

            if (!mesh.clusterIndices.empty()) {
                int skinIndex = gltfMeshToSkin[gltfMeshIndex];
                if (skinIndex != -1) {
                    const auto& deformer = hfmModel.skinDeformers[(size_t)skinIndex];
                    std::vector<uint16_t> oldToNew;
                    oldToNew.resize(_file.nodes.size());
                    for (uint16_t clusterIndex = 0; clusterIndex < deformer.clusters.size() - 1; ++clusterIndex) {
                        const auto& cluster = deformer.clusters[clusterIndex];
                        oldToNew[(size_t)cluster.jointIndex] = clusterIndex;
                    }
                }
            }

            // populate the texture coordinates if they don't exist
            if (mesh.texCoords.size() == 0 && !hfmModel.hasSkeletonJoints) {
                for (int i = 0; i < part.triangleIndices.size(); ++i) { mesh.texCoords.push_back(glm::vec2(0.0, 1.0)); }
            }

            // Build morph targets (blend shapes)
            if (!primitive.targets.isEmpty()) {

                // Build list of blendshapes from FST
                typedef QPair<int, float> WeightedIndex;
                hifi::VariantHash blendshapeMappings = mapping.value("bs").toHash();
                QMultiHash<QString, WeightedIndex> blendshapeIndices;

                for (int i = 0;; ++i) {
                    hifi::ByteArray blendshapeName = FACESHIFT_BLENDSHAPES[i];
                    if (blendshapeName.isEmpty()) {
                        break;
                    }
                    QList<QVariant> mappings = blendshapeMappings.values(blendshapeName);
                    foreach (const QVariant& mapping, mappings) {
                        QVariantList blendshapeMapping = mapping.toList();
                        blendshapeIndices.insert(blendshapeMapping.at(0).toByteArray(), WeightedIndex(i, blendshapeMapping.at(1).toFloat()));
                    }
                }

                // glTF morph targets may or may not have names. if they are labeled, add them based on
                // the corresponding names from the FST. otherwise, just add them in the order they are given
                mesh.blendshapes.resize(blendshapeMappings.size());
                auto values = blendshapeIndices.values();
                auto keys = blendshapeIndices.keys();
                auto names = gltfMesh.extras.targetNames;
                QVector<double> weights = gltfMesh.weights;

                for (int weightedIndex = 0; weightedIndex < values.size(); ++weightedIndex) {
                    float weight = 0.1f;
                    int indexFromMapping = weightedIndex;
                    int targetIndex = weightedIndex;
                    hfmModel.blendshapeChannelNames.push_back("target_" + QString::number(weightedIndex));

                    if (!names.isEmpty()) {
                        targetIndex = names.indexOf(keys[weightedIndex]);
                        indexFromMapping = values[weightedIndex].first;
                        weight = weight * values[weightedIndex].second;
                        hfmModel.blendshapeChannelNames[weightedIndex] = keys[weightedIndex];
                    }
                    HFMBlendshape& blendshape = mesh.blendshapes[indexFromMapping];
                    blendshape.indices = part.triangleIndices;
                    auto target = primitive.targets[targetIndex];

                    QVector<glm::vec3> normals;
                    QVector<glm::vec3> vertices;

                    if (weights.size() == primitive.targets.size()) {
                        int targetWeight = weights[targetIndex];
                        if (targetWeight != 0) {
                            weight = weight * targetWeight;
                        }
                    }

                    if (target.values.contains((QString) "NORMAL")) {
                        generateTargetData(target.values.value((QString) "NORMAL"), weight, normals);
                    }
                    if (target.values.contains((QString) "POSITION")) {
                        generateTargetData(target.values.value((QString) "POSITION"), weight, vertices);
                    }
                    bool isNewBlendshape = blendshape.vertices.size() < vertices.size();
                    int count = 0;
                    for (int i : blendshape.indices) {
                        if (isNewBlendshape) {
                            blendshape.vertices.push_back(vertices[i]);
                            blendshape.normals.push_back(normals[i]);
                        } else {
                            blendshape.vertices[count] = blendshape.vertices[count] + vertices[i];
                            blendshape.normals[count] = blendshape.normals[count] + normals[i];
                            ++count;
                        }
                    }
                }
            }         
        }
    }


    // Create the instance shapes for each transform node
    for (int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex) {
        const auto& node = _file.nodes[nodeIndex];
        if (-1 == node.mesh) {
            continue;
        }

        const auto& gltfMesh = _file.meshes[node.mesh];
        const auto& templateShapePerPrim = templateShapePerPrimPerGLTFMesh[node.mesh];
        int primCount = (int)gltfMesh.primitives.size();
        for (int primIndex = 0; primIndex < primCount; ++primIndex) {
            const auto& templateShape = templateShapePerPrim[primIndex];
            hfmModel.shapes.push_back(templateShape);
            auto& hfmShape = hfmModel.shapes.back();
            // Everything else is already defined (mesh, meshPart, material), so just define the new transform and deformer if present
            hfmShape.joint = nodeIndex;
            hfmShape.skinDeformer = node.skin != -1 ? node.skin : hfm::UNDEFINED_KEY;
        }
    }

    // TODO: Fix skinning and remove this workaround which disables skinning
    // TODO: Restore after testing
    {
        std::vector<int> meshToRootJoint;
        meshToRootJoint.resize(hfmModel.meshes.size(), -1);
        std::vector<uint16_t> meshToClusterSize;
        meshToClusterSize.resize(hfmModel.meshes.size());
        for (auto& shape : hfmModel.shapes) {
            shape.skinDeformer = hfm::UNDEFINED_KEY;
        }

        for (auto& mesh : hfmModel.meshes) {
            mesh.clusterWeights.clear();
            mesh.clusterIndices.clear();
            mesh.clusterWeightsPerVertex = 0;
        }

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
        _file.sortNodes();
        _file.populateMaterialNames();
        _file.normalizeNodeTransforms();
        auto hfmModelPtr = std::make_shared<HFMModel>();
        HFMModel& hfmModel = *hfmModelPtr;
        buildGeometry(hfmModel, mapping, _url);

        //hfmDebugDump(data);
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
    auto request =
        DependencyManager::get<ResourceManager>()->createResourceRequest(nullptr, url, true, -1, "GLTFSerializer::requestData");

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
    auto connection = QObject::connect(qApp, &QCoreApplication::aboutToQuit, [&] { aboutToQuit = true; });
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest netRequest(url);
    netRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply* netReply = isTest ? networkAccessManager.head(netRequest) : networkAccessManager.get(netRequest);
    if (!qApp || aboutToQuit) {
        netReply->deleteLater();
        return nullptr;
    }
    QEventLoop loop;  // Create an event loop that will quit when we get the finished signal
    QObject::connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();  // Nothing is going to happen on this whole run thread until we get this

    QObject::disconnect(connection);
    return netReply;  // trying to sync later on.
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

template <typename T, typename L>
bool GLTFSerializer::readArray(const hifi::ByteArray& bin, int byteOffset, int count, QVector<L>& outarray, int accessorType) {
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
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < bufferCount; ++j) {
            if (!blobstream.atEnd()) {
                T value;
                blobstream >> value;
                outarray.push_back(value);
            } else {
                blobstream.setDevice(nullptr);
                return false;
            }
        }
    }

    blobstream.setDevice(nullptr);
    return true;
}
template <typename T>
bool GLTFSerializer::addArrayOfType(const hifi::ByteArray& bin,
                                    int byteOffset,
                                    int count,
                                    QVector<T>& outarray,
                                    int accessorType,
                                    int componentType) {
    switch (componentType) {
        case GLTFAccessorComponentType::BYTE: {}
        case GLTFAccessorComponentType::UNSIGNED_BYTE: {
            return readArray<uchar>(bin, byteOffset, count, outarray, accessorType);
        }
        case GLTFAccessorComponentType::SHORT: {
            return readArray<short>(bin, byteOffset, count, outarray, accessorType);
        }
        case GLTFAccessorComponentType::UNSIGNED_INT: {
            return readArray<uint>(bin, byteOffset, count, outarray, accessorType);
        }
        case GLTFAccessorComponentType::UNSIGNED_SHORT: {
            return readArray<ushort>(bin, byteOffset, count, outarray, accessorType);
        }
        case GLTFAccessorComponentType::FLOAT: {
            return readArray<float>(bin, byteOffset, count, outarray, accessorType);
        }
    }
    return false;
}


template <typename T>
bool GLTFSerializer::addArrayFromAttribute(GLTFVertexAttribute::Value vertexAttribute, GLTFAccessor& accessor, QVector<T>& outarray) {
    switch (vertexAttribute) {
    case GLTFVertexAttribute::POSITION:
        if (accessor.type != GLTFAccessorType::VEC3) {
            qWarning(modelformat) << "Invalid accessor type on glTF POSITION data for model " << _url;
            return false;
        }

        if (!addArrayFromAccessor(accessor, outarray)) {
            qWarning(modelformat) << "There was a problem reading glTF POSITION data for model " << _url;
            return false;
        }
        break;

    case GLTFVertexAttribute::NORMAL:
        if (accessor.type != GLTFAccessorType::VEC3) {
            qWarning(modelformat) << "Invalid accessor type on glTF NORMAL data for model " << _url;
            return false;
        }

        if (!addArrayFromAccessor(accessor, outarray)) {
            qWarning(modelformat) << "There was a problem reading glTF NORMAL data for model " << _url;
            return false;
        }
        break;

    case GLTFVertexAttribute::TANGENT:
        if (accessor.type != GLTFAccessorType::VEC4 && accessor.type != GLTFAccessorType::VEC3) {
            qWarning(modelformat) << "Invalid accessor type on glTF TANGENT data for model " << _url;
            return false;
        }

        if (!addArrayFromAccessor(accessor, outarray)) {
            qWarning(modelformat) << "There was a problem reading glTF TANGENT data for model " << _url;
            return false;
        }
        break;

    case GLTFVertexAttribute::TEXCOORD_0:
        if (accessor.type != GLTFAccessorType::VEC2) {
            qWarning(modelformat) << "Invalid accessor type on glTF TEXCOORD_0 data for model " << _url;
            return false;
        }

        if (!addArrayFromAccessor(accessor, outarray)) {
            qWarning(modelformat) << "There was a problem reading glTF TEXCOORD_0 data for model " << _url;
            return false;
        }
        break;

    case GLTFVertexAttribute::TEXCOORD_1:
        if (accessor.type != GLTFAccessorType::VEC2) {
            qWarning(modelformat) << "Invalid accessor type on glTF TEXCOORD_1 data for model " << _url;
            return false;
        }
        if (!addArrayFromAccessor(accessor, outarray)) {
            qWarning(modelformat) << "There was a problem reading glTF TEXCOORD_1 data for model " << _url;
            return false;
        }
        break;

    case GLTFVertexAttribute::COLOR_0:
        if (accessor.type != GLTFAccessorType::VEC4 && accessor.type != GLTFAccessorType::VEC3) {
            qWarning(modelformat) << "Invalid accessor type on glTF COLOR_0 data for model " << _url;
            return false;
        }

        if (!addArrayFromAccessor(accessor, outarray)) {
            qWarning(modelformat) << "There was a problem reading glTF COLOR_0 data for model " << _url;
            return false;
        }
        break;

    case GLTFVertexAttribute::JOINTS_0:
        if (accessor.type < GLTFAccessorType::SCALAR || accessor.type > GLTFAccessorType::VEC4) {
            qWarning(modelformat) << "Invalid accessor type on glTF JOINTS_0 data for model " << _url;
            return false;
        }

        if (!addArrayFromAccessor(accessor, outarray)) {
            qWarning(modelformat) << "There was a problem reading glTF JOINTS_0 data for model " << _url;
            return false;
        }
        break;

    case GLTFVertexAttribute::WEIGHTS_0:
        if (accessor.type < GLTFAccessorType::SCALAR || accessor.type > GLTFAccessorType::VEC4) {
            qWarning(modelformat) << "Invalid accessor type on glTF WEIGHTS_0 data for model " << _url;
            return false;
        }

        if (!addArrayFromAccessor(accessor, outarray)) {
            qWarning(modelformat) << "There was a problem reading glTF WEIGHTS_0 data for model " << _url;
            return false;
        }
        break;

    default:
        qWarning(modelformat) << "Unexpected attribute type" << _url;
        return false;
    }

    
    return true;
}

template <typename T>
bool GLTFSerializer::addArrayFromAccessor(GLTFAccessor& accessor, QVector<T>& outarray) {
    bool success = true;

    if (accessor.defined["bufferView"]) {
        GLTFBufferView& bufferview = _file.bufferviews[accessor.bufferView];
        GLTFBuffer& buffer = _file.buffers[bufferview.buffer];

        int accBoffset = accessor.defined["byteOffset"] ? accessor.byteOffset : 0;

        success = addArrayOfType(buffer.blob, bufferview.byteOffset + accBoffset, accessor.count, outarray, accessor.type,
                                 accessor.componentType);
    } else {
        for (int i = 0; i < accessor.count; ++i) {
            T value;
            memset(&value, 0, sizeof(T));  // Make sure the dummy array is initalised to zero.
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
                                     accessor.sparse.indices.componentType);
            if (success) {
                QVector<T> out_sparse_values_array;

                GLTFBufferView& sparseValuesBufferview = _file.bufferviews[accessor.sparse.values.bufferView];
                GLTFBuffer& sparseValuesBuffer = _file.buffers[sparseValuesBufferview.buffer];

                int accSVBoffset = accessor.sparse.values.defined["byteOffset"] ? accessor.sparse.values.byteOffset : 0;

                success = addArrayOfType(sparseValuesBuffer.blob, sparseValuesBufferview.byteOffset + accSVBoffset,
                                         accessor.sparse.count, out_sparse_values_array, accessor.type, accessor.componentType);

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

void GLTFSerializer::retriangulate(const QVector<int>& inIndices,
                                   const QVector<glm::vec3>& in_vertices,
                                   const QVector<glm::vec3>& in_normals,
                                   QVector<int>& outIndices,
                                   QVector<glm::vec3>& out_vertices,
                                   QVector<glm::vec3>& out_normals) {
    for (int i = 0; i < inIndices.size(); i = i + 3) {
        int idx1 = inIndices[i];
        int idx2 = inIndices[i + 1];
        int idx3 = inIndices[i + 2];

        out_vertices.push_back(in_vertices[idx1]);
        out_vertices.push_back(in_vertices[idx2]);
        out_vertices.push_back(in_vertices[idx3]);

        out_normals.push_back(in_normals[idx1]);
        out_normals.push_back(in_normals[idx2]);
        out_normals.push_back(in_normals[idx3]);

        outIndices.push_back(i);
        outIndices.push_back(i + 1);
        outIndices.push_back(i + 2);
    }
}

void GLTFSerializer::glTFDebugDump() {
    qCDebug(modelformat) << "---------------- Nodes ----------------";
    for (GLTFNode node : _file.nodes) {
        if (node.defined["mesh"]) {
            qCDebug(modelformat) << "\n";
            qCDebug(modelformat) << "    node_transform" << node.transform;
            qCDebug(modelformat) << "\n";
        }
    }

    qCDebug(modelformat) << "---------------- Accessors ----------------";
    for (GLTFAccessor accessor : _file.accessors) {
        qCDebug(modelformat) << "\n";
        qCDebug(modelformat) << "count: " << accessor.count;
        qCDebug(modelformat) << "byteOffset: " << accessor.byteOffset;
        qCDebug(modelformat) << "\n";
    }

    qCDebug(modelformat) << "---------------- Textures ----------------";
    for (GLTFTexture texture : _file.textures) {
        if (texture.defined["source"]) {
            qCDebug(modelformat) << "\n";
            QString url = _file.images[texture.source].uri;
            QString fname = hifi::URL(url).fileName();
            qCDebug(modelformat) << "fname: " << fname;
            qCDebug(modelformat) << "\n";
        }
    }

    qCDebug(modelformat) << "\n";
}

void GLTFSerializer::hfmDebugDump(const HFMModel& hfmModel) {
    hfmModel.debugDump();

    qCDebug(modelformat) << "---------------- GLTF Model ----------------";
    glTFDebugDump();

    qCDebug(modelformat) << "\n";
}
