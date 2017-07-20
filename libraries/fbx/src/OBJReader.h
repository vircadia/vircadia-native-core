
#include <QtNetwork/QNetworkReply>
#include "FBXReader.h"

class OBJTokenizer {
public:
    OBJTokenizer(QIODevice* device);
    enum SpecialToken {
        NO_TOKEN = -1,
        NO_PUSHBACKED_TOKEN = -1,
        DATUM_TOKEN = 0x100,
        COMMENT_TOKEN = 0x101
    };
    int nextToken();
    const QByteArray& getDatum() const { return _datum; }
    bool isNextTokenFloat();
    const QByteArray getLineAsDatum(); // some "filenames" have spaces in them
    void skipLine() { _device->readLine(); }
    void pushBackToken(int token) { _pushedBackToken = token; }
    void ungetChar(char ch) { _device->ungetChar(ch); }
    const QString getComment() const { return _comment; }
    glm::vec3 getVec3();
    bool getVertex(glm::vec3& vertex, glm::vec3& vertexColor);
    glm::vec2 getVec2();
    float getFloat() { return std::stof((nextToken() != OBJTokenizer::DATUM_TOKEN) ? nullptr : getDatum().data()); }

private:
    QIODevice* _device;
    QByteArray _datum;
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
    bool add(const QByteArray& vertexIndex, const QByteArray& textureIndex, const QByteArray& normalIndex,
             const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& vertexColors);
    // Return a set of one or more OBJFaces from this one, in which each is just a triangle.
    // Even though FBXMeshPart can handle quads, it would be messy to try to keep track of mixed-size faces, so we treat everything as triangles.
    QVector<OBJFace> triangulate();
private:
    void addFrom(const OBJFace* face, int index);
};

// Materials and references to material names can come in any order, and different mesh parts can refer to the same material.
// Therefore it would get pretty hacky to try to use FBXMeshPart to store these as we traverse the files.
class OBJMaterial {
public:
    float shininess;
    float opacity;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    QByteArray diffuseTextureFilename;
    QByteArray specularTextureFilename;
    bool used { false };
    bool userSpecifiesUV { false };
    OBJMaterial() : shininess(0.0f), opacity(1.0f), diffuseColor(0.9f), specularColor(0.9f) {}
};

class OBJReader: public QObject { // QObject so we can make network requests.
    Q_OBJECT
public:
    typedef QVector<OBJFace> FaceGroup;
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> vertexColors;
    QVector<glm::vec2> textureUVs;
    QVector<glm::vec3> normals;
    QVector<FaceGroup> faceGroups;
    QString currentMaterialName;
    QHash<QString, OBJMaterial> materials;

    FBXGeometry* readOBJ(QByteArray& model, const QVariantHash& mapping, bool combineParts, const QUrl& url = QUrl());

private:
    QUrl _url;

    QHash<QByteArray, bool> librariesSeen;
    bool parseOBJGroup(OBJTokenizer& tokenizer, const QVariantHash& mapping, FBXGeometry& geometry,
                       float& scaleGuess, bool combineParts);
    void parseMaterialLibrary(QIODevice* device);
    bool isValidTexture(const QByteArray &filename); // true if the file exists. TODO?: check content-type header and that it is a supported format.

    int _partCounter { 0 };
};

// What are these utilities doing here? One is used by fbx loading code in VHACD Utils, and the other a general debugging utility.
void setMeshPartDefaults(FBXMeshPart& meshPart, QString materialID);
void fbxDebugDump(const FBXGeometry& fbxgeo);
