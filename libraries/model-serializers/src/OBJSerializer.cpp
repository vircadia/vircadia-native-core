//
//  OBJSerializer.cpp
//  libraries/model-serializers/src
//
//  Created by Seth Alves on 3/7/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// http://en.wikipedia.org/wiki/Wavefront_.obj_file
// http://www.scratchapixel.com/old/lessons/3d-advanced-lessons/obj-file-format/obj-file-format/
// http://paulbourke.net/dataformats/obj/

#include "OBJSerializer.h"

#include <ctype.h>  // .obj files are not locale-specific. The C/ASCII charset applies.
#include <sstream>

#include <QtCore/QBuffer>
#include <QtCore/QIODevice>
#include <QtCore/QEventLoop>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include <shared/NsightHelpers.h>
#include <NetworkAccessManager.h>
#include <ResourceManager.h>

#include "FBXSerializer.h"
#include <hfm/ModelFormatLogging.h>
#include <shared/PlatformHacks.h>

QHash<QString, float> COMMENT_SCALE_HINTS = {{"This file uses centimeters as units", 1.0f / 100.0f},
                                             {"This file uses millimeters as units", 1.0f / 1000.0f}};

const QString SMART_DEFAULT_MATERIAL_NAME = "High Fidelity smart default material name";

const float ILLUMINATION_MODEL_MIN_OPACITY = 0.1f;
const float ILLUMINATION_MODEL_APPLY_SHININESS = 0.5f;
const float ILLUMINATION_MODEL_APPLY_ROUGHNESS = 1.0f;
const float ILLUMINATION_MODEL_APPLY_NON_METALLIC = 0.0f;

namespace {
template<class T>
T& checked_at(QVector<T>& vector, int i) {
    if (i < 0 || i >= vector.size()) {
        throw std::out_of_range("index " + std::to_string(i) + "is out of range");
    }
    return vector[i];
}
}

OBJTokenizer::OBJTokenizer(QIODevice* device) : _device(device), _pushedBackToken(-1) {
}

const hifi::ByteArray OBJTokenizer::getLineAsDatum() {
    return _device->readLine().trimmed();
}

float OBJTokenizer::getFloat() {
    return std::stof((nextToken() != OBJTokenizer::DATUM_TOKEN) ? nullptr : getDatum().data());
}

int OBJTokenizer::nextToken(bool allowSpaceChar /*= false*/) {
    if (_pushedBackToken != NO_PUSHBACKED_TOKEN) {
        int token = _pushedBackToken;
        _pushedBackToken = NO_PUSHBACKED_TOKEN;
        return token;
    }

    char ch;
    while (_device->getChar(&ch)) {
        if (QChar(ch).isSpace()) {
            continue; // skip whitespace
        }
        switch (ch) {
            case '#': {
                _datum = "";
                _comment = _device->readLine(); // stash comment for a future call to getComment
                return COMMENT_TOKEN;
            }

            case '\"':
                _datum = "";
                while (_device->getChar(&ch)) {
                    if (ch == '\"') { // end on closing quote
                        break;
                    }
                    if (ch == '\\') { // handle escaped quotes
                        if (_device->getChar(&ch) && ch != '\"') {
                            _datum.append('\\');
                        }
                    }
                    _datum.append(ch);
                }
                return DATUM_TOKEN;

            default:
                _datum = "";
                _datum.append(ch);
                while (_device->getChar(&ch)) {
                    if ((QChar(ch).isSpace() || ch == '\"') && (!allowSpaceChar || ch != ' ')) {
                        ungetChar(ch); // read until we encounter a special character, then replace it
                        break;
                    }
                    _datum.append(ch);
                }

                return DATUM_TOKEN;
        }
    }
    return NO_TOKEN;
}

bool OBJTokenizer::isNextTokenFloat() {
    if (nextToken() != OBJTokenizer::DATUM_TOKEN) {
        return false;
    }
    hifi::ByteArray token = getDatum();
    pushBackToken(OBJTokenizer::DATUM_TOKEN);
    bool ok;
    token.toFloat(&ok);
    return ok;
}

glm::vec3 OBJTokenizer::getVec3() {
    auto x = getFloat(); // N.B.: getFloat() has side-effect
    auto y = getFloat(); // And order of arguments is different on Windows/Linux.
    auto z = getFloat();
    auto v = glm::vec3(x, y, z);
    while (isNextTokenFloat()) {
        // ignore any following floats
        nextToken();
    }
    return v;
}
bool OBJTokenizer::getVertex(glm::vec3& vertex, glm::vec3& vertexColor) {
    // Used for vertices which may also have a vertex color (RGB [0,1]) to follow.
    //    NOTE: Returns true if there is a vertex color.
    auto x = getFloat(); // N.B.: getFloat() has side-effect
    auto y = getFloat(); // And order of arguments is different on Windows/Linux.
    auto z = getFloat();
    vertex = glm::vec3(x, y, z);

    auto r = 1.0f, g = 1.0f, b = 1.0f;
    bool hasVertexColor = false;
    if (isNextTokenFloat()) {
        // If there's another float it's one of two things: a W component or an R component. The standard OBJ spec
        // doesn't output a W component, so we're making the assumption that if a float follows (unless it's
        // only a single value) that it's a vertex color.
        r = getFloat();
        if (isNextTokenFloat()) {
            // Safe to assume the following values are the green/blue components.
            g = getFloat();
            b = getFloat();

            hasVertexColor = true;
        }

        vertexColor = glm::vec3(r, g, b);
    }

    return hasVertexColor;
}
glm::vec2 OBJTokenizer::getVec2() {
    float uCoord = getFloat();
    float vCoord = 1.0f - getFloat();
    auto v = glm::vec2(uCoord, vCoord);
    while (isNextTokenFloat()) {
        // there can be a w, but we don't handle that
        nextToken();
    }
    return v;
}


void setMeshPartDefaults(HFMMeshPart& meshPart, QString materialID) {
    meshPart.materialID = materialID;
}

// OBJFace
//    NOTE (trent, 7/20/17): The vertexColors vector being passed-in isn't necessary here, but I'm just
//                         pairing it with the vertices vector for consistency.
bool OBJFace::add(const hifi::ByteArray& vertexIndex, const hifi::ByteArray& textureIndex, const hifi::ByteArray& normalIndex, const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& vertexColors) {
    bool ok;
    int index = vertexIndex.toInt(&ok);
    if (!ok) {
        return false;
    }
    vertexIndices.append(index - 1);
    if (!textureIndex.isEmpty()) {
        index = textureIndex.toInt(&ok);
        if (!ok) {
            return false;
        }
        if (index < 0) { // Count backwards from the last one added.
            index = vertices.count() + 1 + index;
        }
        textureUVIndices.append(index - 1);
    }
    if (!normalIndex.isEmpty()) {
        index = normalIndex.toInt(&ok);
        if (!ok) {
            return false;
        }
        normalIndices.append(index - 1);
    }
    return true;
}

QVector<OBJFace> OBJFace::triangulate() {
    QVector<OBJFace> newFaces;
    const int nVerticesInATriangle = 3;
    if (vertexIndices.count() == nVerticesInATriangle) {
        newFaces.append(*this);
    } else {
        for (int i = 1; i < vertexIndices.count() - 1; i++) {
            OBJFace newFace;
            newFace.addFrom(this, 0);
            newFace.addFrom(this, i);
            newFace.addFrom(this, i + 1);
            newFace.groupName = groupName;
            newFace.materialName = materialName;
            newFaces.append(newFace);
        }
    }
    return newFaces;
}

void OBJFace::addFrom(const OBJFace* face, int index) { // add using data from f at index i
    vertexIndices.append(face->vertexIndices[index]);
    if (face->textureUVIndices.count() > 0) { // Any at all. Runtime error if not consistent.
        textureUVIndices.append(face->textureUVIndices[index]);
    }
    if (face->normalIndices.count() > 0) {
        normalIndices.append(face->normalIndices[index]);
    }
}

bool OBJSerializer::isValidTexture(const hifi::ByteArray &filename) {
    if (_url.isEmpty()) {
        return false;
    }
    hifi::URL candidateUrl = _url.resolved(hifi::URL(filename));

    return DependencyManager::get<ResourceManager>()->resourceExists(candidateUrl);
}

void OBJSerializer::parseMaterialLibrary(QIODevice* device) {
    OBJTokenizer tokenizer(device);
    QString matName = SMART_DEFAULT_MATERIAL_NAME;
    OBJMaterial& currentMaterial = materials[matName];
    while (true) {
        switch (tokenizer.nextToken()) {
            case OBJTokenizer::COMMENT_TOKEN:
                #ifdef WANT_DEBUG
                qCDebug(modelformat) << "OBJSerializer MTLLIB comment:" << tokenizer.getComment();
                #endif
                break;
            case OBJTokenizer::DATUM_TOKEN:
                break;
            default:
                materials[matName] = currentMaterial;
                #ifdef WANT_DEBUG
                qCDebug(modelformat) <<
                                     "OBJSerializer Last material illumination model:" << currentMaterial.illuminationModel <<
                                     " shininess:" << currentMaterial.shininess <<
                                     " opacity:" << currentMaterial.opacity <<
                                     " diffuse color:" << currentMaterial.diffuseColor <<
                                     " specular color:" << currentMaterial.specularColor <<
                                     " emissive color:" << currentMaterial.emissiveColor <<
                                     " diffuse texture:" << currentMaterial.diffuseTextureFilename <<
                                     " specular texture:" << currentMaterial.specularTextureFilename <<
                                     " emissive texture:" << currentMaterial.emissiveTextureFilename <<
                                     " bump texture:" << currentMaterial.bumpTextureFilename <<
                                     " opacity texture:" << currentMaterial.opacityTextureFilename;
#endif
                return;
        }
        hifi::ByteArray token = tokenizer.getDatum();
        if (token == "newmtl") {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                return;
            }
            materials[matName] = currentMaterial;
            matName = tokenizer.getDatum();
            currentMaterial = materials[matName];
            #ifdef WANT_DEBUG
            qCDebug(modelformat) << "OBJSerializer Starting new material definition " << matName;
            #endif
            currentMaterial.diffuseTextureFilename = "";
            currentMaterial.emissiveTextureFilename = "";
            currentMaterial.specularTextureFilename = "";
            currentMaterial.bumpTextureFilename = "";
            currentMaterial.opacityTextureFilename = "";

        } else if (token == "Ns") {
            currentMaterial.shininess = tokenizer.getFloat();
        } else if (token == "Ni") {
            #ifdef WANT_DEBUG
            qCDebug(modelformat) << "OBJSerializer Ignoring material Ni " << tokenizer.getFloat();
            #else
            tokenizer.getFloat();
            #endif
        } else if (token == "d") {
            currentMaterial.opacity = tokenizer.getFloat();
        } else if (token == "Tr") {
            currentMaterial.opacity = 1.0f - tokenizer.getFloat();
        } else if (token == "illum") {
            currentMaterial.illuminationModel = tokenizer.getFloat();
        } else if (token == "Tf") {
            #ifdef WANT_DEBUG
            qCDebug(modelformat) << "OBJSerializer Ignoring material Tf " << tokenizer.getVec3();
            #else
            tokenizer.getVec3();
            #endif
        } else if (token == "Ka") {
            #ifdef WANT_DEBUG
            qCDebug(modelformat) << "OBJSerializer Ignoring material Ka " << tokenizer.getVec3();;
            #else
            tokenizer.getVec3();
            #endif
        } else if (token == "Kd") {
            currentMaterial.diffuseColor = tokenizer.getVec3();
        } else if (token == "Ke") {
            currentMaterial.emissiveColor = tokenizer.getVec3();
        } else if (token == "Ks") {
            currentMaterial.specularColor = tokenizer.getVec3();
        } else if ((token == "map_Kd") || (token == "map_Ke") || (token == "map_Ks") || (token == "map_bump") || (token == "bump") || (token == "map_d")) {
            const hifi::ByteArray textureLine = tokenizer.getLineAsDatum();
            hifi::ByteArray filename;
            OBJMaterialTextureOptions textureOptions;
            parseTextureLine(textureLine, filename, textureOptions);
            if (filename.endsWith(".tga")) {
                #ifdef WANT_DEBUG
                qCDebug(modelformat) << "OBJSerializer WARNING: currently ignoring tga texture " << filename << " in " << _url;
                #endif
                break;
            }
            if (token == "map_Kd") {
                currentMaterial.diffuseTextureFilename = filename;
            } else if (token == "map_Ke") {
                currentMaterial.emissiveTextureFilename = filename;
            } else if (token == "map_Ks" ) {
                currentMaterial.specularTextureFilename = filename;
            } else if ((token == "map_bump") || (token == "bump")) {
                currentMaterial.bumpTextureFilename = filename;
                currentMaterial.bumpTextureOptions = textureOptions;
            } else if (token == "map_d") {
                currentMaterial.opacityTextureFilename = filename;
            }
        }
    }
}

void OBJSerializer::parseTextureLine(const hifi::ByteArray& textureLine, hifi::ByteArray& filename, OBJMaterialTextureOptions& textureOptions) {
    // Texture options reference http://paulbourke.net/dataformats/mtl/
    // and https://wikivisually.com/wiki/Material_Template_Library

    std::istringstream iss(textureLine.toStdString());
    const std::vector<std::string> parser(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());

    uint i = 0;
    while (i < parser.size()) {
        if (i + 1 < parser.size() && parser[i][0] == '-') {
            const std::string& option = parser[i++];
            if (option == "-blendu" || option == "-blendv") {
                #ifdef WANT_DEBUG
                const std::string& onoff = parser[i++];
                qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring texture option" << option.c_str() << onoff.c_str();
                #endif
            } else if (option == "-bm") {
                const std::string& bm = parser[i++];
                textureOptions.bumpMultiplier = std::stof(bm);
            } else if (option == "-boost") {
                #ifdef WANT_DEBUG
                const std::string& boost = parser[i++];
                float boostFloat = std::stof(boost);
                qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring texture option" << option.c_str() << boost.c_str();
                #endif
            } else if (option == "-cc") {
                #ifdef WANT_DEBUG
                const std::string& onoff = parser[i++];
                qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring texture option" << option.c_str() << onoff.c_str();
                #endif
            } else if (option == "-clamp") {
                #ifdef WANT_DEBUG
                const std::string& onoff = parser[i++];
                qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring texture option" << option.c_str() << onoff.c_str();
                #endif
            } else if (option == "-imfchan") {
                #ifdef WANT_DEBUG
                const std::string& imfchan = parser[i++];
                qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring texture option" << option.c_str() << imfchan.c_str();
                #endif
            } else if (option == "-mm") {
                if (i + 1 < parser.size()) {
                    #ifdef WANT_DEBUG
                    const std::string& mmBase = parser[i++];
                    const std::string& mmGain = parser[i++];
                    float mmBaseFloat = std::stof(mmBase);
                    float mmGainFloat = std::stof(mmGain);
                    qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring texture option" << option.c_str() << mmBase.c_str() << mmGain.c_str();
                    #endif
                }
            } else if (option == "-o" || option == "-s" || option == "-t") {
                if (i + 2 < parser.size()) {
                    #ifdef WANT_DEBUG
                    const std::string& u = parser[i++];
                    const std::string& v = parser[i++];
                    const std::string& w = parser[i++];
                    float uFloat = std::stof(u);
                    float vFloat = std::stof(v);
                    float wFloat = std::stof(w);
                    qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring texture option" << option.c_str() << u.c_str() << v.c_str() << w.c_str();
                    #endif
                }
            } else if (option == "-texres") {
                #ifdef WANT_DEBUG
                const std::string& texres = parser[i++];
                float texresFloat = std::stof(texres);
                qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring texture option" << option.c_str() << texres.c_str();
                #endif
            } else if (option == "-type") {
                #ifdef WANT_DEBUG
                const std::string& type = parser[i++];
                qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring texture option" << option.c_str() << type.c_str();
                #endif
            } else if (option[0] == '-') {
                #ifdef WANT_DEBUG
                qCDebug(modelformat) << "OBJSerializer WARNING: Ignoring unsupported texture option" << option.c_str();
                #endif
            }
        } else { // assume filename at end when no more options
            std::string filenameString = parser[i++];
            while (i < parser.size()) { // filename has space in it
                filenameString += " " + parser[i++];
            }
            filename = filenameString.c_str();
        }
    }
}

std::tuple<bool, hifi::ByteArray> requestData(hifi::URL& url) {
    auto request = DependencyManager::get<ResourceManager>()->createResourceRequest(
        nullptr, url, true, -1, "(OBJSerializer) requestData");

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


QNetworkReply* request(hifi::URL& url, bool isTest) {
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


bool OBJSerializer::parseOBJGroup(OBJTokenizer& tokenizer, const hifi::VariantHash& mapping, HFMModel& hfmModel,
                              float& scaleGuess, bool combineParts) {
    FaceGroup faces;
    HFMMesh& mesh = hfmModel.meshes[0];
    mesh.parts.append(HFMMeshPart());
    HFMMeshPart& meshPart = mesh.parts.last();
    bool sawG = false;
    bool result = true;
    int originalFaceCountForDebugging = 0;
    QString currentGroup;
    bool anyVertexColor { false };
    int vertexCount { 0 };

    setMeshPartDefaults(meshPart, QString("dontknow") + QString::number(mesh.parts.count()));

    while (true) {
        int tokenType = tokenizer.nextToken();
        if (tokenType == OBJTokenizer::COMMENT_TOKEN) {
            // loop through the list of known comments which suggest a scaling factor.
            // if we find one, save the scaling hint into scaleGuess
            QString comment = tokenizer.getComment();
            QHashIterator<QString, float> i(COMMENT_SCALE_HINTS);
            while (i.hasNext()) {
                i.next();
                if (comment.contains(i.key())) {
                    scaleGuess = i.value();
                }
            }
            continue;
        }
        if (tokenType != OBJTokenizer::DATUM_TOKEN) {
            result = false;
            break;
        }
        hifi::ByteArray token = tokenizer.getDatum();
        //qCDebug(modelformat) << token;
        // we don't support separate objects in the same file, so treat "o" the same as "g".
        if (token == "g" || token == "o") {
            if (sawG) {
                // we've encountered the beginning of the next group.
                tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                break;
            }
            sawG = true;
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            hifi::ByteArray groupName = tokenizer.getDatum();
            currentGroup = groupName;
            if (!combineParts) {
                currentMaterialName = QString("part-") + QString::number(_partCounter++);
            }
        } else if (token == "mtllib" && !_url.isEmpty()) {
            if (tokenizer.nextToken(true) != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            hifi::ByteArray libraryName = tokenizer.getDatum();
            librariesSeen[libraryName] = true;
            // We'll read it later only if we actually need it.
        } else if (token == "usemtl") {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            QString nextName = tokenizer.getDatum();
            if (nextName != currentMaterialName) {
                if (combineParts) {
                    currentMaterialName = nextName;
                }
                #ifdef WANT_DEBUG
                qCDebug(modelformat) << "OBJSerializer new current material:" << currentMaterialName;
                #endif
            }
        } else if (token == "v") {
            glm::vec3 vertex;
            glm::vec3 vertexColor { glm::vec3(1.0f) };

            bool hasVertexColor = tokenizer.getVertex(vertex, vertexColor);
            vertices.append(vertex);

            // if any vertex has color, they all need to.
            if (hasVertexColor && !anyVertexColor) {
                // we've had a gap of zero or more vertices without color, followed
                // by one that has color.  catch up:
                for (int i = 0; i < vertexCount; i++) {
                    vertexColors.append(glm::vec3(1.0f));
                }
                anyVertexColor = true;
            }
            if (anyVertexColor) {
                vertexColors.append(vertexColor);
            }
            vertexCount++;
        } else if (token == "vn") {
            normals.append(tokenizer.getVec3());
        } else if (token == "vt") {
            textureUVs.append(tokenizer.getVec2());
        } else if (token == "f") {
            OBJFace face;
            while (true) {
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                    if (face.vertexIndices.count() == 0) {
                        // nonsense, bail out.
                        goto done;
                    }
                    break;
                }
                // faces can be:
                //   vertex-index
                //   vertex-index/texture-index
                //   vertex-index/texture-index/surface-normal-index
                hifi::ByteArray token = tokenizer.getDatum();
                auto firstChar = token[0];
                // Tokenizer treats line endings as whitespace. Non-digit and non-negative sign indicates done;
                if (!isdigit(firstChar) && firstChar != '-') {
                    tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                    break;
                }
                QList<hifi::ByteArray> parts = token.split('/');
                assert(parts.count() >= 1);
                assert(parts.count() <= 3);
                // If indices are negative relative indices then adjust them to absolute indices based on current vector sizes
                // Also add 1 to each index as 1 will be subtracted later on from each index in OBJFace::add
                for (int i = 0; i < parts.count(); ++i) {
                    int part = parts[i].toInt();
                    if (part < 0) {
                        switch (i) {
                            case 0:
                                parts[i].setNum(vertices.size() + part + 1);
                                break;
                            case 1:
                                parts[i].setNum(textureUVs.size() + part + 1);
                                break;
                            case 2:
                                parts[i].setNum(normals.size() + part + 1);
                                break;
                        }
                    }
                }
                const hifi::ByteArray noData {};
                face.add(parts[0], (parts.count() > 1) ? parts[1] : noData, (parts.count() > 2) ? parts[2] : noData,
                         vertices, vertexColors);
                face.groupName = currentGroup;
                face.materialName = currentMaterialName;
            }
            originalFaceCountForDebugging++;
            foreach(OBJFace face, face.triangulate()) {
                faces.append(face);
            }
        } else {
            // something we don't (yet) care about
            // qCDebug(modelformat) << "OBJ parser is skipping a line with" << token;
            tokenizer.skipLine();
        }
    }
done:
    if (faces.count() == 0) { // empty mesh
        mesh.parts.pop_back();
    } else {
        faceGroups.append(faces); // We're done with this group. Add the faces.
    }
    return result;
}

MediaType OBJSerializer::getMediaType() const {
    MediaType mediaType("obj");
    mediaType.extensions.push_back("obj");
    return mediaType;
}

std::unique_ptr<hfm::Serializer::Factory> OBJSerializer::getFactory() const {
    return std::make_unique<hfm::Serializer::SimpleFactory<OBJSerializer>>();
}

HFMModel::Pointer OBJSerializer::read(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url) {
    PROFILE_RANGE_EX(resource_parse, __FUNCTION__, 0xffff0000, nullptr);
    QBuffer buffer { const_cast<hifi::ByteArray*>(&data) };
    buffer.open(QIODevice::ReadOnly);

    auto hfmModelPtr = std::make_shared<HFMModel>();
    HFMModel& hfmModel { *hfmModelPtr };
    OBJTokenizer tokenizer { &buffer };
    float scaleGuess = 1.0f;

    bool needsMaterialLibrary = false;

    _url = url;
    bool combineParts = mapping.value("combineParts").toBool();
    hfmModel.meshExtents.reset();
    hfmModel.meshes.append(HFMMesh());

    try {
        // call parseOBJGroup as long as it's returning true.  Each successful call will
        // add a new meshPart to the model's single mesh.
        while (parseOBJGroup(tokenizer, mapping, hfmModel, scaleGuess, combineParts)) {}

        HFMMesh& mesh = hfmModel.meshes[0];
        mesh.meshIndex = 0;

        hfmModel.joints.resize(1);
        hfmModel.joints[0].parentIndex = -1;
        hfmModel.joints[0].distanceToParent = 0;
        hfmModel.joints[0].translation = glm::vec3(0, 0, 0);
        hfmModel.joints[0].rotationMin = glm::vec3(0, 0, 0);
        hfmModel.joints[0].rotationMax = glm::vec3(0, 0, 0);
        hfmModel.joints[0].name = "OBJ";
        hfmModel.joints[0].isSkeletonJoint = true;

        hfmModel.jointIndices["x"] = 1;

        HFMCluster cluster;
        cluster.jointIndex = 0;
        cluster.inverseBindMatrix = glm::mat4(1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1);
        mesh.clusters.append(cluster);

        QMap<QString, int> materialMeshIdMap;
        QVector<HFMMeshPart> hfmMeshParts;
        for (int i = 0, meshPartCount = 0; i < mesh.parts.count(); i++, meshPartCount++) {
            HFMMeshPart& meshPart = mesh.parts[i];
            FaceGroup faceGroup = faceGroups[meshPartCount];
            bool specifiesUV = false;
            foreach(OBJFace face, faceGroup) {
                // Go through all of the OBJ faces and determine the number of different materials necessary (each different material will be a unique mesh).
                // NOTE (trent/mittens 3/30/17): this seems hardcore wasteful and is slowed down a bit by iterating through the face group twice, but it's the best way I've thought of to hack multi-material support in an OBJ into this pipeline.
                if (!materialMeshIdMap.contains(face.materialName)) {
                    // Create a new HFMMesh for this material mapping.
                    materialMeshIdMap.insert(face.materialName, materialMeshIdMap.count());

                    hfmMeshParts.append(HFMMeshPart());
                    HFMMeshPart& meshPartNew = hfmMeshParts.last();
                    meshPartNew.quadIndices = QVector<int>(meshPart.quadIndices);                    // Copy over quad indices [NOTE (trent/mittens, 4/3/17): Likely unnecessary since they go unused anyway].
                    meshPartNew.quadTrianglesIndices = QVector<int>(meshPart.quadTrianglesIndices); // Copy over quad triangulated indices [NOTE (trent/mittens, 4/3/17): Likely unnecessary since they go unused anyway].
                    meshPartNew.triangleIndices = QVector<int>(meshPart.triangleIndices);            // Copy over triangle indices.

                    // Do some of the material logic (which previously lived below) now.
                    // All the faces in the same group will have the same name and material.
                    QString groupMaterialName = face.materialName;
                    if (groupMaterialName.isEmpty() && specifiesUV) {
#ifdef WANT_DEBUG
                        qCDebug(modelformat) << "OBJSerializer WARNING: " << url
                            << " needs a texture that isn't specified. Using default mechanism.";
#endif
                        groupMaterialName = SMART_DEFAULT_MATERIAL_NAME;
                    }
                    if (!groupMaterialName.isEmpty()) {
                        OBJMaterial& material = materials[groupMaterialName];
                        if (specifiesUV) {
                            material.userSpecifiesUV = true; // Note might not be true in a later usage.
                        }
                        if (specifiesUV || (groupMaterialName.compare("none", Qt::CaseInsensitive) != 0)) {
                            // Blender has a convention that a material named "None" isn't really used (or defined).
                            material.used = true;
                            needsMaterialLibrary = groupMaterialName != SMART_DEFAULT_MATERIAL_NAME;
                        }
                        materials[groupMaterialName] = material;
                        meshPartNew.materialID = groupMaterialName;
                    }
                }
            }
        }

        // clean up old mesh parts.
        int unmodifiedMeshPartCount = mesh.parts.count();
        mesh.parts.clear();
        mesh.parts = QVector<HFMMeshPart>(hfmMeshParts);

        for (int i = 0, meshPartCount = 0; i < unmodifiedMeshPartCount; i++, meshPartCount++) {
            FaceGroup faceGroup = faceGroups[meshPartCount];

            // Now that each mesh has been created with its own unique material mappings, fill them with data (vertex data is duplicated, face data is not).
            foreach(OBJFace face, faceGroup) {
                HFMMeshPart& meshPart = mesh.parts[materialMeshIdMap[face.materialName]];

                glm::vec3 v0 = checked_at(vertices, face.vertexIndices[0]);
                glm::vec3 v1 = checked_at(vertices, face.vertexIndices[1]);
                glm::vec3 v2 = checked_at(vertices, face.vertexIndices[2]);

                glm::vec3 vc0, vc1, vc2;
                bool hasVertexColors = (vertexColors.size() > 0);
                if (hasVertexColors) {
                    // If there are any vertex colors, it's safe to assume all meshes had them exported.
                    vc0 = checked_at(vertexColors, face.vertexIndices[0]);
                    vc1 = checked_at(vertexColors, face.vertexIndices[1]);
                    vc2 = checked_at(vertexColors, face.vertexIndices[2]);
                }

                // Scale the vertices if the OBJ file scale is specified as non-one.
                if (scaleGuess != 1.0f) {
                    v0 *= scaleGuess;
                    v1 *= scaleGuess;
                    v2 *= scaleGuess;
                }

                // Add the vertices.
                meshPart.triangleIndices.append(mesh.vertices.count()); // not face.vertexIndices into vertices
                mesh.vertices << v0;
                meshPart.triangleIndices.append(mesh.vertices.count());
                mesh.vertices << v1;
                meshPart.triangleIndices.append(mesh.vertices.count());
                mesh.vertices << v2;

                if (hasVertexColors) {
                    // Add vertex colors.
                    mesh.colors << vc0;
                    mesh.colors << vc1;
                    mesh.colors << vc2;
                }

                glm::vec3 n0, n1, n2;
                if (face.normalIndices.count()) {
                    n0 = checked_at(normals, face.normalIndices[0]);
                    n1 = checked_at(normals, face.normalIndices[1]);
                    n2 = checked_at(normals, face.normalIndices[2]);
                } else {
                    // generate normals from triangle plane if not provided
                    n0 = n1 = n2 = glm::cross(v1 - v0, v2 - v0);
                }

                mesh.normals.append(n0);
                mesh.normals.append(n1);
                mesh.normals.append(n2);

                if (face.textureUVIndices.count()) {
                    mesh.texCoords <<
                        checked_at(textureUVs, face.textureUVIndices[0]) <<
                        checked_at(textureUVs, face.textureUVIndices[1]) <<
                        checked_at(textureUVs, face.textureUVIndices[2]);
                } else {
                    glm::vec2 corner(0.0f, 1.0f);
                    mesh.texCoords << corner << corner << corner;
                }
            }
        }

        mesh.meshExtents.reset();
        foreach(const glm::vec3& vertex, mesh.vertices) {
            mesh.meshExtents.addPoint(vertex);
            hfmModel.meshExtents.addPoint(vertex);
        }

        // hfmDebugDump(hfmModel);
    } catch(const std::exception& e) {
        qCDebug(modelformat) << "OBJSerializer fail: " << e.what();
    }

    QString queryPart = _url.query();
    bool suppressMaterialsHack = queryPart.contains("hifiusemat"); // If this appears in query string, don't fetch mtl even if used.
    OBJMaterial& preDefinedMaterial = materials[SMART_DEFAULT_MATERIAL_NAME];
    preDefinedMaterial.used = true;
    if (suppressMaterialsHack) {
        needsMaterialLibrary = preDefinedMaterial.userSpecifiesUV = false; // I said it was a hack...
    }
    // Some .obj files use the convention that a group with uv coordinates that doesn't define a material, should use
    // a texture with the same basename as the .obj file.
    if (preDefinedMaterial.userSpecifiesUV && !_url.isEmpty()) {
        QString filename = _url.fileName();
        int extIndex = filename.lastIndexOf('.'); // by construction, this does not fail
        QString basename = filename.remove(extIndex + 1, sizeof("obj"));
        preDefinedMaterial.diffuseColor = glm::vec3(1.0f);
        QVector<hifi::ByteArray> extensions = { "jpg", "jpeg", "png", "tga" };
        hifi::ByteArray base = basename.toUtf8(), textName = "";
        qCDebug(modelformat) << "OBJSerializer looking for default texture";
        for (int i = 0; i < extensions.count(); i++) {
            hifi::ByteArray candidateString = base + extensions[i];
            if (isValidTexture(candidateString)) {
                textName = candidateString;
                break;
            }
        }

        if (!textName.isEmpty()) {
            #ifdef WANT_DEBUG
            qCDebug(modelformat) << "OBJSerializer found a default texture: " << textName;
            #endif
            preDefinedMaterial.diffuseTextureFilename = textName;
        }
        materials[SMART_DEFAULT_MATERIAL_NAME] = preDefinedMaterial;
    }
    if (needsMaterialLibrary) {
        foreach (QString libraryName, librariesSeen.keys()) {
            // Throw away any path part of libraryName, and merge against original url.
            hifi::URL libraryUrl = _url.resolved(hifi::URL(libraryName).fileName());
            qCDebug(modelformat) << "OBJSerializer material library" << libraryName;
            bool success;
            hifi::ByteArray data;
            std::tie<bool, hifi::ByteArray>(success, data) = requestData(libraryUrl);
            if (success) {
                QBuffer buffer { &data };
                buffer.open(QIODevice::ReadOnly);
                parseMaterialLibrary(&buffer);
            } else {
                qCDebug(modelformat) << "OBJSerializer WARNING:" << libraryName << "did not answer";
            }
        }
    }

    foreach (QString materialID, materials.keys()) {
        OBJMaterial& objMaterial = materials[materialID];
        if (!objMaterial.used) {
            continue;
        }

        HFMMaterial& hfmMaterial = hfmModel.materials[materialID] = HFMMaterial(objMaterial.diffuseColor,
                                                                                objMaterial.specularColor,
                                                                                objMaterial.emissiveColor,
                                                                                objMaterial.shininess,
                                                                                objMaterial.opacity);

        hfmMaterial.name = materialID;
        hfmMaterial.materialID = materialID;
        hfmMaterial._material = std::make_shared<graphics::Material>();
        graphics::MaterialPointer modelMaterial = hfmMaterial._material;

        if (!objMaterial.diffuseTextureFilename.isEmpty()) {
            hfmMaterial.albedoTexture.filename = objMaterial.diffuseTextureFilename;
        }
        if (!objMaterial.specularTextureFilename.isEmpty()) {
            hfmMaterial.specularTexture.filename = objMaterial.specularTextureFilename;
        }
        if (!objMaterial.emissiveTextureFilename.isEmpty()) {
            hfmMaterial.emissiveTexture.filename = objMaterial.emissiveTextureFilename;
        }
        if (!objMaterial.bumpTextureFilename.isEmpty()) {
            hfmMaterial.normalTexture.filename = objMaterial.bumpTextureFilename;
            hfmMaterial.normalTexture.isBumpmap = true;
            hfmMaterial.bumpMultiplier = objMaterial.bumpTextureOptions.bumpMultiplier;
        }
        if (!objMaterial.opacityTextureFilename.isEmpty()) {
            hfmMaterial.opacityTexture.filename = objMaterial.opacityTextureFilename;
        }

        modelMaterial->setEmissive(hfmMaterial.emissiveColor);
        modelMaterial->setAlbedo(hfmMaterial.diffuseColor);
        modelMaterial->setMetallic(glm::length(hfmMaterial.specularColor));
        modelMaterial->setRoughness(graphics::Material::shininessToRoughness(hfmMaterial.shininess));

        bool applyTransparency = false;
        bool applyShininess = false;
        bool applyRoughness = false;
        bool applyNonMetallic = false;
        bool fresnelOn = false;

        // Illumination model reference http://paulbourke.net/dataformats/mtl/
        switch (objMaterial.illuminationModel) {
            case 0: // Color on and Ambient off
                // We don't support ambient = do nothing?
                break;
            case 1: // Color on and Ambient on
                // We don't support ambient = do nothing?
                break;
            case 2: // Highlight on
                // Change specular intensity = do nothing for now?
                break;
            case 3: // Reflection on and Ray trace on
                applyShininess = true;
                break;
            case 4: // Transparency: Glass on and Reflection: Ray trace on
                applyTransparency = true;
                applyShininess = true;
                break;
            case 5: // Reflection: Fresnel on and Ray trace on
                applyShininess = true;
                fresnelOn = true;
                break;
            case 6: // Transparency: Refraction on and Reflection: Fresnel off and Ray trace on
                applyTransparency = true;
                applyNonMetallic = true;
                applyShininess = true;
                break;
            case 7: // Transparency: Refraction on and Reflection: Fresnel on and Ray trace on
                applyTransparency = true;
                applyNonMetallic = true;
                applyShininess = true;
                fresnelOn = true;
                break;
            case 8: // Reflection on and Ray trace off
                applyShininess = true;
                break;
            case 9: // Transparency: Glass on and Reflection: Ray trace off
                applyTransparency = true;
                applyNonMetallic = true;
                applyRoughness = true;
                break;
            case 10: // Casts shadows onto invisible surfaces
                // Do nothing?
                break;
        }

        if (applyTransparency) {
            hfmMaterial.opacity = std::max(hfmMaterial.opacity, ILLUMINATION_MODEL_MIN_OPACITY);
        }
        if (applyShininess) {
            modelMaterial->setRoughness(ILLUMINATION_MODEL_APPLY_SHININESS);
        } else if (applyRoughness) {
            modelMaterial->setRoughness(ILLUMINATION_MODEL_APPLY_ROUGHNESS);
        }
        if (applyNonMetallic) {
            modelMaterial->setMetallic(ILLUMINATION_MODEL_APPLY_NON_METALLIC);
        }
        if (fresnelOn) {
            // TODO: how to turn fresnel on?
        }

        modelMaterial->setOpacity(hfmMaterial.opacity);
    }

    return hfmModelPtr;
}

void hfmDebugDump(const HFMModel& hfmModel) {
    qCDebug(modelformat) << "---------------- hfmModel ----------------";
    qCDebug(modelformat) << "  hasSkeletonJoints =" << hfmModel.hasSkeletonJoints;
    qCDebug(modelformat) << "  offset =" << hfmModel.offset;
    qCDebug(modelformat) << "  meshes.count() =" << hfmModel.meshes.count();
    foreach (HFMMesh mesh, hfmModel.meshes) {
        qCDebug(modelformat) << "    vertices.count() =" << mesh.vertices.count();
        qCDebug(modelformat) << "    colors.count() =" << mesh.colors.count();
        qCDebug(modelformat) << "    normals.count() =" << mesh.normals.count();
        /*if (mesh.normals.count() == mesh.vertices.count()) {
            for (int i = 0; i < mesh.normals.count(); i++) {
                qCDebug(modelformat) << "        " << mesh.vertices[ i ] << mesh.normals[ i ];
            }
        }*/
        qCDebug(modelformat) << "    tangents.count() =" << mesh.tangents.count();
        qCDebug(modelformat) << "    colors.count() =" << mesh.colors.count();
        qCDebug(modelformat) << "    texCoords.count() =" << mesh.texCoords.count();
        qCDebug(modelformat) << "    texCoords1.count() =" << mesh.texCoords1.count();
        qCDebug(modelformat) << "    clusterIndices.count() =" << mesh.clusterIndices.count();
        qCDebug(modelformat) << "    clusterWeights.count() =" << mesh.clusterWeights.count();
        qCDebug(modelformat) << "    meshExtents =" << mesh.meshExtents;
        qCDebug(modelformat) << "    modelTransform =" << mesh.modelTransform;
        qCDebug(modelformat) << "    parts.count() =" << mesh.parts.count();
        foreach (HFMMeshPart meshPart, mesh.parts) {
            qCDebug(modelformat) << "        quadIndices.count() =" << meshPart.quadIndices.count();
            qCDebug(modelformat) << "        triangleIndices.count() =" << meshPart.triangleIndices.count();
   /*
            qCDebug(modelformat) << "        diffuseColor =" << meshPart.diffuseColor << "mat =" << meshPart._material->getDiffuse();
            qCDebug(modelformat) << "        specularColor =" << meshPart.specularColor << "mat =" << meshPart._material->getMetallic();
            qCDebug(modelformat) << "        emissiveColor =" << meshPart.emissiveColor << "mat =" << meshPart._material->getEmissive();
            qCDebug(modelformat) << "        emissiveParams =" << meshPart.emissiveParams;
            qCDebug(modelformat) << "        gloss =" << meshPart.shininess << "mat =" << meshPart._material->getRoughness();
            qCDebug(modelformat) << "        opacity =" << meshPart.opacity << "mat =" << meshPart._material->getOpacity();
            */
            qCDebug(modelformat) << "        materialID =" << meshPart.materialID;
      /*      qCDebug(modelformat) << "        diffuse texture =" << meshPart.diffuseTexture.filename;
            qCDebug(modelformat) << "        specular texture =" << meshPart.specularTexture.filename;
            */
        }
        qCDebug(modelformat) << "    clusters.count() =" << mesh.clusters.count();
        foreach (HFMCluster cluster, mesh.clusters) {
            qCDebug(modelformat) << "        jointIndex =" << cluster.jointIndex;
            qCDebug(modelformat) << "        inverseBindMatrix =" << cluster.inverseBindMatrix;
        }
    }

    qCDebug(modelformat) << "  jointIndices =" << hfmModel.jointIndices;
    qCDebug(modelformat) << "  joints.count() =" << hfmModel.joints.count();

    foreach (HFMJoint joint, hfmModel.joints) {

        qCDebug(modelformat) << "    parentIndex" << joint.parentIndex;
        qCDebug(modelformat) << "    distanceToParent" << joint.distanceToParent;
        qCDebug(modelformat) << "    translation" << joint.translation;
        qCDebug(modelformat) << "    preTransform" << joint.preTransform;
        qCDebug(modelformat) << "    preRotation" << joint.preRotation;
        qCDebug(modelformat) << "    rotation" << joint.rotation;
        qCDebug(modelformat) << "    postRotation" << joint.postRotation;
        qCDebug(modelformat) << "    postTransform" << joint.postTransform;
        qCDebug(modelformat) << "    transform" << joint.transform;
        qCDebug(modelformat) << "    rotationMin" << joint.rotationMin;
        qCDebug(modelformat) << "    rotationMax" << joint.rotationMax;
        qCDebug(modelformat) << "    inverseDefaultRotation" << joint.inverseDefaultRotation;
        qCDebug(modelformat) << "    inverseBindRotation" << joint.inverseBindRotation;
        qCDebug(modelformat) << "    bindTransform" << joint.bindTransform;
        qCDebug(modelformat) << "    name" << joint.name;
        qCDebug(modelformat) << "    isSkeletonJoint" << joint.isSkeletonJoint;
    }

    qCDebug(modelformat) << "\n";
}
