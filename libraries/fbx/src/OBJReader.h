

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
    void skipLine() { _device->readLine(); }
    void pushBackToken(int token) { _pushedBackToken = token; }
    void ungetChar(char ch) { _device->ungetChar(ch); }
    const QString getComment() const { return _comment; }
    glm::vec3 getVec3();
    glm::vec2 getVec2();
    
private:
    float getFloat() { return std::stof((nextToken() != OBJTokenizer::DATUM_TOKEN) ? nullptr : getDatum().data()); }
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
    //materialName groupName  // FIXME
    // Add one more set of vertex data. Answers true if successful
    bool add(QByteArray vertexIndex, QByteArray textureIndex = nullptr, QByteArray normalIndex = nullptr);
    // Return a set of one or more OBJFaces from this one, in which each is just a triangle.
    // Even though FBXMeshPart can handle quads, it would be messy to try to keep track of mixed-size faces, so we treat everything as triangles.
    QVector<OBJFace> triangulate();
private:
    void addFrom(const OBJFace* face, int index);
};

class OBJReader {
public:
    typedef QVector<OBJFace> FaceGroup;
    QVector<glm::vec3> vertices;  // all that we ever encounter while reading
    QVector<glm::vec2> textureUVs;
    QVector<glm::vec3> normals;
    QVector<FaceGroup> faceGroups;
    FBXGeometry readOBJ(const QByteArray& model, const QVariantHash& mapping);
    FBXGeometry readOBJ(QIODevice* device, const QVariantHash& mapping);
    void fbxDebugDump(const FBXGeometry& fbxgeo);
    bool parseOBJGroup(OBJTokenizer& tokenizer, const QVariantHash& mapping, FBXGeometry& geometry, float& scaleGuess);
};
