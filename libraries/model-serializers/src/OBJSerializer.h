//
//  OBJSerializer.h
//  libraries/model-serializers/src
//
//  Created by Seth Alves on 3/6/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OBJSerializer_h
#define hifi_OBJSerializer_h

#include <QtNetwork/QNetworkReply>
#include <hfm/HFMSerializer.h>

class OBJTokenizer {
public:
    OBJTokenizer(QIODevice* device);
    enum SpecialToken {
        NO_TOKEN = -1,
        NO_PUSHBACKED_TOKEN = -1,
        DATUM_TOKEN = 0x100,
        COMMENT_TOKEN = 0x101
    };
    int nextToken(bool allowSpaceChar = false);
    const hifi::ByteArray& getDatum() const { return _datum; }
    bool isNextTokenFloat();
    const hifi::ByteArray getLineAsDatum(); // some "filenames" have spaces in them
    void skipLine() { _device->readLine(); }
    void pushBackToken(int token) { _pushedBackToken = token; }
    void ungetChar(char ch) { _device->ungetChar(ch); }
    const QString getComment() const { return _comment; }
    glm::vec3 getVec3();
    bool getVertex(glm::vec3& vertex, glm::vec3& vertexColor);
    glm::vec2 getVec2();
    float getFloat();

private:
    QIODevice* _device;
    hifi::ByteArray _datum;
    int _pushedBackToken;
    QString _comment;
};

class OBJFace { // A single face, with three or more planar vertices. But see triangulate().
public:
    QVector<int> vertexIndices;
    QVector<int> textureUVIndices;
    QVector<int> normalIndices;
    QString groupName; // We don't make use of hierarchical structure, but it can be preserved for debugging and future use.
    QString materialName;
    // Add one more set of vertex data. Answers true if successful
    bool add(const hifi::ByteArray& vertexIndex, const hifi::ByteArray& textureIndex, const hifi::ByteArray& normalIndex,
             const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& vertexColors);
    // Return a set of one or more OBJFaces from this one, in which each is just a triangle.
    // Even though HFMMeshPart can handle quads, it would be messy to try to keep track of mixed-size faces, so we treat everything as triangles.
    QVector<OBJFace> triangulate();
private:
    void addFrom(const OBJFace* face, int index);
};

class OBJMaterialTextureOptions {
public:
    float bumpMultiplier { 1.0f };
}
;
// Materials and references to material names can come in any order, and different mesh parts can refer to the same material.
// Therefore it would get pretty hacky to try to use HFMMeshPart to store these as we traverse the files.
class OBJMaterial {
public:
    float shininess;
    float opacity;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    glm::vec3 emissiveColor;
    hifi::ByteArray diffuseTextureFilename;
    hifi::ByteArray specularTextureFilename;
    hifi::ByteArray emissiveTextureFilename;
    hifi::ByteArray bumpTextureFilename;
    hifi::ByteArray opacityTextureFilename;

    OBJMaterialTextureOptions bumpTextureOptions;
    int illuminationModel;
    bool used { false };
    bool userSpecifiesUV { false };
    OBJMaterial() : shininess(0.0f), opacity(1.0f), diffuseColor(0.9f), specularColor(0.9f), emissiveColor(0.0f), illuminationModel(-1) {}
};

class OBJSerializer: public QObject, public HFMSerializer { // QObject so we can make network requests.
    Q_OBJECT
public:
    MediaType getMediaType() const override;
    std::unique_ptr<hfm::Serializer::Factory> getFactory() const override;

    typedef QVector<OBJFace> FaceGroup;
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> vertexColors;
    QVector<glm::vec2> textureUVs;
    QVector<glm::vec3> normals;
    QVector<FaceGroup> faceGroups;
    QString currentMaterialName;
    QHash<QString, OBJMaterial> materials;

    HFMModel::Pointer read(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url = hifi::URL()) override;

private:
    hifi::URL _url;

    QHash<hifi::ByteArray, bool> librariesSeen;
    bool parseOBJGroup(OBJTokenizer& tokenizer, const hifi::VariantHash& mapping, HFMModel& hfmModel,
                       float& scaleGuess, bool combineParts);
    void parseMaterialLibrary(QIODevice* device);
    void parseTextureLine(const hifi::ByteArray& textureLine, hifi::ByteArray& filename, OBJMaterialTextureOptions& textureOptions);
    bool isValidTexture(const hifi::ByteArray &filename); // true if the file exists. TODO?: check content-type header and that it is a supported format.

    int _partCounter { 0 };
};

// What are these utilities doing here? One is used by fbx loading code in VHACD Utils, and the other a general debugging utility.
void setMeshPartDefaults(HFMMeshPart& meshPart, QString materialID);
void hfmDebugDump(const HFMModel& hfmModel);

#endif // hifi_OBJSerializer_h
