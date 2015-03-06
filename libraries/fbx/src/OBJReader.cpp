

#include <QBuffer>
#include <QIODevice>


#include "FBXReader.h"
#include "OBJReader.h"



class OBJTokenizer {
public:
    OBJTokenizer(QIODevice* device) : _device(device), _pushedBackToken(-1) { }
    enum SpecialToken { DATUM_TOKEN = 0x100 };
    int nextToken();
    const QByteArray& getDatum() const { return _datum; }
    void skipLine() { _device->readLine(); }
    void pushBackToken(int token) { _pushedBackToken = token; }
    void ungetChar(char ch) { _device->ungetChar(ch); }

private:
    QIODevice* _device;
    QByteArray _datum;
    int _pushedBackToken;
};

int OBJTokenizer::nextToken() {
    if (_pushedBackToken != -1) {
        int token = _pushedBackToken;
        _pushedBackToken = -1;
        return token;
    }

    char ch;
    while (_device->getChar(&ch)) {
        if (QChar(ch).isSpace()) {
            continue; // skip whitespace
        }
        switch (ch) {
            case '#':
                _device->readLine(); // skip the comment
                break;

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
                    if (QChar(ch).isSpace() || ch == '\"') {
                        ungetChar(ch); // read until we encounter a special character, then replace it
                        break;
                    }
                    _datum.append(ch);
                }

                return DATUM_TOKEN;
        }
    }
    return -1;
}



bool parseOBJMesh(OBJTokenizer &tokenizer, const QVariantHash& mapping, FBXGeometry &geometry, int& indexStart) {
    FBXMesh mesh;
    FBXMeshPart meshPart;
    bool sawG = false;
    bool meshHasData = true;
    bool result = true;

    try { // XXX move this out/up
        while (true) {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                result = false;
                break;
            }
            QByteArray token = tokenizer.getDatum();
            if (token == "g") {
                if (sawG) {
                    // we've encountered the beginning of the next group.
                    tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                    break;
                }
                sawG = true;
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                QByteArray groupName = tokenizer.getDatum();
                meshPart.materialID = groupName;
                meshHasData = true;
            } else if (token == "v") {
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                float x = std::stof(tokenizer.getDatum().data());
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                float y = std::stof(tokenizer.getDatum().data());
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                float z = std::stof(tokenizer.getDatum().data());
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }

                // float w = 1.0;
                try{
                    // w =
                    std::stof(tokenizer.getDatum().data());
                }
                catch(const std::exception& e){
                    // next token wasn't a number (the w field is optional), push it back
                    tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                }

                mesh.vertices.append(glm::vec3(x, y, z));
                meshHasData = true;
            } else if (token == "f") {
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                int p0 = std::stoi(tokenizer.getDatum().data());
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                int p1 = std::stoi(tokenizer.getDatum().data());
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                int p2 = std::stoi(tokenizer.getDatum().data());

                // obj indexes is 1 based and index over the entire file.  mesh indexes are 0 based
                // and index within the mesh.
                meshPart.triangleIndices.append(p0 - 1 - indexStart); // obj index is 1 based
                meshPart.triangleIndices.append(p1 - 1 - indexStart);
                meshPart.triangleIndices.append(p2 - 1 - indexStart);
                meshHasData = true;
            } else {
                // something we don't (yet) care about
                qDebug() << "skipping line with" << token;
                tokenizer.skipLine();
            }
        }
    }
    catch(const std::exception& e) {
        // something went wrong, stop here.
        qDebug() << "something went wrong";
        return false;
    }

    if (meshHasData) {
        mesh.parts.append(meshPart);
        geometry.meshes.append(mesh);
    }

    indexStart += mesh.vertices.count();

    return result;
}



FBXGeometry extractOBJGeometry(const FBXNode& node, const QVariantHash& mapping) {
    FBXGeometry geometry;
    return geometry;
}



FBXGeometry readOBJ(const QByteArray& model, const QVariantHash& mapping) {
    QBuffer buffer(const_cast<QByteArray*>(&model));
    buffer.open(QIODevice::ReadOnly);
    return readOBJ(&buffer, mapping);
}


FBXGeometry readOBJ(QIODevice* device, const QVariantHash& mapping) {
    FBXGeometry geometry;
    OBJTokenizer tokenizer(device);
    int indexStart = 0;

    while (true) {
        if (!parseOBJMesh(tokenizer, mapping, geometry, indexStart)) {
            break;
        }
    }

    return geometry;
}
