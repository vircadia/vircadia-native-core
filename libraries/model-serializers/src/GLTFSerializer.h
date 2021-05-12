//
//  GLTFSerializer.h
//  libraries/model-serializers/src
//
//  Created by Luis Cuenca on 8/30/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GLTFSerializer_h
#define hifi_GLTFSerializer_h

#include <memory.h>
#include <QtNetwork/QNetworkReply>
#include <hfm/ModelFormatLogging.h>
#include <hfm/HFMSerializer.h>


struct GLTFAsset {
    QString generator;
    QString version; //required
    QString copyright;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["generator"]) {
            qCDebug(modelformat) << "generator: " << generator;
        }
        if (defined["version"]) {
            qCDebug(modelformat) << "version: " << version;
        }
        if (defined["copyright"]) {
            qCDebug(modelformat) << "copyright: " << copyright;
        }
    }
};

struct GLTFNode {
    QString name;
    int camera;
    int mesh;
    QVector<int> children;
    QVector<double> translation;
    QVector<double> rotation;
    QVector<double> scale;
    QVector<double> matrix;
    QVector<glm::mat4> transforms;
    int skin;
    QVector<int> skeletons;
    QString jointName;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["name"]) {
            qCDebug(modelformat) << "name: " << name;
        }
        if (defined["camera"]) {
            qCDebug(modelformat) << "camera: " << camera;
        }
        if (defined["mesh"]) {
            qCDebug(modelformat) << "mesh: " << mesh;
        }
        if (defined["skin"]) {
            qCDebug(modelformat) << "skin: " << skin;
        }
        if (defined["jointName"]) {
            qCDebug(modelformat) << "jointName: " << jointName;
        }
        if (defined["children"]) {
            qCDebug(modelformat) << "children: " << children;
        }
        if (defined["translation"]) {
            qCDebug(modelformat) << "translation: " << translation;
        }
        if (defined["rotation"]) {
            qCDebug(modelformat) << "rotation: " << rotation;
        }
        if (defined["scale"]) {
            qCDebug(modelformat) << "scale: " << scale;
        }
        if (defined["matrix"]) {
            qCDebug(modelformat) << "matrix: " << matrix;
        }
        if (defined["skeletons"]) {
            qCDebug(modelformat) << "skeletons: " << skeletons;
        }
    }
};

// Meshes

struct GLTFMeshPrimitivesTarget {
    int normal;
    int position;
    int tangent;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["normal"]) {
            qCDebug(modelformat) << "normal: " << normal;
        }
        if (defined["position"]) {
            qCDebug(modelformat) << "position: " << position;
        }
        if (defined["tangent"]) {
            qCDebug(modelformat) << "tangent: " << tangent;
        }
    }
};

namespace GLTFMeshPrimitivesRenderingMode {
    enum Values {
        POINTS = 0,
        LINES,
        LINE_LOOP,
        LINE_STRIP,
        TRIANGLES,
        TRIANGLE_STRIP,
        TRIANGLE_FAN
    };
}

struct GLTFMeshPrimitiveAttr {
    QMap<QString, int> values;
    QMap<QString, bool> defined;
    void dump() {
        QList<QString> keys = values.keys();
        qCDebug(modelformat) << "values: ";
        foreach(auto k, keys) {
            qCDebug(modelformat) << k << ": " << values[k];
        }
    }
};

struct GLTFMeshPrimitive {
    GLTFMeshPrimitiveAttr attributes;
    int indices;
    int material;
    int mode{ GLTFMeshPrimitivesRenderingMode::TRIANGLES };
    QVector<GLTFMeshPrimitiveAttr> targets;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["attributes"]) {
            qCDebug(modelformat) << "attributes: ";
            attributes.dump();
        }
        if (defined["indices"]) {
            qCDebug(modelformat) << "indices: " << indices;
        }
        if (defined["material"]) {
            qCDebug(modelformat) << "material: " << material;
        }
        if (defined["mode"]) {
            qCDebug(modelformat) << "mode: " << mode;
        }
        if (defined["targets"]) {
            qCDebug(modelformat) << "targets: ";
            foreach(auto t, targets) t.dump();
        }
    }
};

struct GLTFMeshExtra {
    QVector<QString> targetNames;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["targetNames"]) {
            qCDebug(modelformat) << "targetNames: " << targetNames;
        }
    }
};

struct GLTFMesh {
    QString name;
    QVector<GLTFMeshPrimitive> primitives;
    GLTFMeshExtra extras;
    QVector<double> weights;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["name"]) {
            qCDebug(modelformat) << "name: " << name;
        }
        if (defined["primitives"]) {
            qCDebug(modelformat) << "primitives: ";
            foreach(auto prim, primitives) prim.dump();
        }
        if (defined["extras"]) {
            qCDebug(modelformat) << "extras: ";
            extras.dump();
        }
        if (defined["weights"]) {
            qCDebug(modelformat) << "weights: " << weights;
        }
    }
};

// BufferViews

namespace GLTFBufferViewTarget {
    enum Values {
        ARRAY_BUFFER = 34962,
        ELEMENT_ARRAY_BUFFER = 34963
    };
}

struct GLTFBufferView {
    int buffer; //required
    int byteLength; //required
    int byteOffset { 0 };
    int target;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["buffer"]) {
            qCDebug(modelformat) << "buffer: " << buffer;
        }
        if (defined["byteLength"]) {
            qCDebug(modelformat) << "byteLength: " << byteLength;
        }
        if (defined["byteOffset"]) {
            qCDebug(modelformat) << "byteOffset: " << byteOffset;
        }
        if (defined["target"]) {
            qCDebug(modelformat) << "target: " << target;
        }
    }
};

// Buffers

struct GLTFBuffer {
    int byteLength; //required
    QString uri;
    hifi::ByteArray blob;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["byteLength"]) {
            qCDebug(modelformat) << "byteLength: " << byteLength;
        }
        if (defined["uri"]) {
            qCDebug(modelformat) << "uri: " << uri;
        }
        if (defined["blob"]) {
            qCDebug(modelformat) << "blob: " << "DEFINED";
        }
    }
};

// Samplers
namespace GLTFSamplerFilterType {
    enum Values {
        NEAREST = 9728,
        LINEAR = 9729,
        NEAREST_MIPMAP_NEAREST = 9984,
        LINEAR_MIPMAP_NEAREST = 9985,
        NEAREST_MIPMAP_LINEAR = 9986,
        LINEAR_MIPMAP_LINEAR = 9987
    };
}

namespace GLTFSamplerWrapType {
    enum Values {
        CLAMP_TO_EDGE = 33071,
        MIRRORED_REPEAT = 33648,
        REPEAT = 10497
    };
}

struct GLTFSampler {
    int magFilter;
    int minFilter;
    int wrapS;
    int wrapT;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["magFilter"]) {
            qCDebug(modelformat) << "magFilter: " << magFilter;
        }
        if (defined["minFilter"]) {
            qCDebug(modelformat) << "minFilter: " << minFilter;
        }
        if (defined["wrapS"]) {
            qCDebug(modelformat) << "wrapS: " << wrapS;
        }
        if (defined["wrapT"]) {
            qCDebug(modelformat) << "wrapT: " << wrapT;
        }
    }
};

// Cameras

struct GLTFCameraPerspective {
    double aspectRatio;
    double yfov; //required
    double zfar;
    double znear; //required
    QMap<QString, bool> defined;
    void dump() {
        if (defined["zfar"]) {
            qCDebug(modelformat) << "zfar: " << zfar;
        }
        if (defined["znear"]) {
            qCDebug(modelformat) << "znear: " << znear;
        }
        if (defined["aspectRatio"]) {
            qCDebug(modelformat) << "aspectRatio: " << aspectRatio;
        }
        if (defined["yfov"]) {
            qCDebug(modelformat) << "yfov: " << yfov;
        }
    }
};

struct GLTFCameraOrthographic {
    double zfar; //required
    double znear; //required
    double xmag; //required
    double ymag; //required
    QMap<QString, bool> defined;
    void dump() {
        if (defined["zfar"]) {
            qCDebug(modelformat) << "zfar: " << zfar;
        }
        if (defined["znear"]) {
            qCDebug(modelformat) << "znear: " << znear;
        }
        if (defined["xmag"]) {
            qCDebug(modelformat) << "xmag: " << xmag;
        }
        if (defined["ymag"]) {
            qCDebug(modelformat) << "ymag: " << ymag;
        }
    }
};

namespace GLTFCameraTypes {
    enum Values {
        ORTHOGRAPHIC = 0,
        PERSPECTIVE
    };
}

struct GLTFCamera {
    QString name;
    GLTFCameraPerspective perspective;  //required (or)
    GLTFCameraOrthographic orthographic;  //required (or)
    int type;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["name"]) {
            qCDebug(modelformat) << "name: " << name;
        }
        if (defined["type"]) {
            qCDebug(modelformat) << "type: " << type;
        }
        if (defined["perspective"]) {
            perspective.dump();
        }
        if (defined["orthographic"]) {
            orthographic.dump();
        }
    }
};

// Images

namespace GLTFImageMimetype {
    enum Values {
        JPEG = 0,
        PNG
    };
};

struct GLTFImage {
    QString uri;  //required (or)
    int mimeType;
    int bufferView;   //required (or)
    QMap<QString, bool> defined;
    void dump() {
        if (defined["mimeType"]) {
            qCDebug(modelformat) << "mimeType: " << mimeType;
        }
        if (defined["bufferView"]) {
            qCDebug(modelformat) << "bufferView: " << bufferView;
        }
    }
};

// Materials

struct GLTFpbrMetallicRoughness {
    QVector<double> baseColorFactor;
    int baseColorTexture;
    int metallicRoughnessTexture;
    double metallicFactor;
    double roughnessFactor;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["baseColorFactor"]) {
            qCDebug(modelformat) << "baseColorFactor: " << baseColorFactor;
        }
        if (defined["baseColorTexture"]) {
            qCDebug(modelformat) << "baseColorTexture: " << baseColorTexture;
        }
        if (defined["metallicRoughnessTexture"]) {
            qCDebug(modelformat) << "metallicRoughnessTexture: " << metallicRoughnessTexture;
        }
        if (defined["metallicFactor"]) {
            qCDebug(modelformat) << "metallicFactor: " << metallicFactor;
        }
        if (defined["roughnessFactor"]) {
            qCDebug(modelformat) << "roughnessFactor: " << roughnessFactor;
        }
        if (defined["baseColorFactor"]) {
            qCDebug(modelformat) << "baseColorFactor: " << baseColorFactor;
        }
    }
};

struct GLTFMaterial {
    QString name;
    QVector<double> emissiveFactor;
    int emissiveTexture;
    int normalTexture;
    int occlusionTexture;
    graphics::MaterialKey::OpacityMapMode alphaMode;
    double alphaCutoff;
    bool doubleSided;
    GLTFpbrMetallicRoughness pbrMetallicRoughness;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["name"]) {
            qCDebug(modelformat) << "name: " << name;
        }
        if (defined["emissiveTexture"]) {
            qCDebug(modelformat) << "emissiveTexture: " << emissiveTexture;
        }
        if (defined["normalTexture"]) {
            qCDebug(modelformat) << "normalTexture: " << normalTexture;
        }
        if (defined["occlusionTexture"]) {
            qCDebug(modelformat) << "occlusionTexture: " << occlusionTexture;
        }
        if (defined["emissiveFactor"]) {
            qCDebug(modelformat) << "emissiveFactor: " << emissiveFactor;
        }
        if (defined["alphaMode"]) {
            qCDebug(modelformat) << "alphaMode: " << alphaMode;
        }
        if (defined["alphaCutoff"]) {
            qCDebug(modelformat) << "alphaCutoff: " << alphaCutoff;
        }
        if (defined["pbrMetallicRoughness"]) {
            pbrMetallicRoughness.dump();
        }
    }
};

// Accesors

namespace GLTFAccessorType {
    enum Values {
        SCALAR = 0,
        VEC2,
        VEC3,
        VEC4,
        MAT2,
        MAT3,
        MAT4
    };
}
namespace GLTFAccessorComponentType {
    enum Values {
        BYTE = 5120,
        UNSIGNED_BYTE = 5121,
        SHORT = 5122,
        UNSIGNED_SHORT = 5123,
        UNSIGNED_INT = 5125,
        FLOAT = 5126
    };
}
struct GLTFAccessor {
    struct GLTFAccessorSparse {
        struct GLTFAccessorSparseIndices {
            int bufferView;
            int byteOffset{ 0 };
            int componentType;

            QMap<QString, bool> defined;
            void dump() {
                if (defined["bufferView"]) {
                    qCDebug(modelformat) << "bufferView: " << bufferView;
                }
                if (defined["byteOffset"]) {
                    qCDebug(modelformat) << "byteOffset: " << byteOffset;
                }
                if (defined["componentType"]) {
                    qCDebug(modelformat) << "componentType: " << componentType;
                }
            }
        };
        struct GLTFAccessorSparseValues {
            int bufferView;
            int byteOffset{ 0 };

            QMap<QString, bool> defined;
            void dump() {
                if (defined["bufferView"]) {
                    qCDebug(modelformat) << "bufferView: " << bufferView;
                }
                if (defined["byteOffset"]) {
                    qCDebug(modelformat) << "byteOffset: " << byteOffset;
                }
            }
        };

        int count;
        GLTFAccessorSparseIndices indices;
        GLTFAccessorSparseValues values;

        QMap<QString, bool> defined;
        void dump() {

        }
    };
    int bufferView;
    int byteOffset { 0 };
    int componentType; //required
    int count; //required
    int type; //required
    bool normalized { false };
    QVector<double> max;
    QVector<double> min;
    GLTFAccessorSparse sparse;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["bufferView"]) {
            qCDebug(modelformat) << "bufferView: " << bufferView;
        }
        if (defined["byteOffset"]) {
            qCDebug(modelformat) << "byteOffset: " << byteOffset;
        }
        if (defined["componentType"]) {
            qCDebug(modelformat) << "componentType: " << componentType;
        }
        if (defined["count"]) {
            qCDebug(modelformat) << "count: " << count;
        }
        if (defined["type"]) {
            qCDebug(modelformat) << "type: " << type;
        }
        if (defined["normalized"]) {
            qCDebug(modelformat) << "normalized: " << (normalized ? "TRUE" : "FALSE");
        }
        if (defined["max"]) {
            qCDebug(modelformat) << "max: ";
            foreach(float m, max) {
                qCDebug(modelformat) << m;
            }
        }
        if (defined["min"]) {
            qCDebug(modelformat) << "min: ";
            foreach(float m, min) {
                qCDebug(modelformat) << m;
            }
        }
        if (defined["sparse"]) {
            qCDebug(modelformat) << "sparse: ";
            sparse.dump();
        }
    }
};

// Animation

namespace GLTFChannelTargetPath {
    enum Values {
        TRANSLATION = 0,
        ROTATION,
        SCALE
    };
}

struct GLTFChannelTarget {
    int node;
    int path;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["node"]) {
            qCDebug(modelformat) << "node: " << node;
        }
        if (defined["path"]) {
            qCDebug(modelformat) << "path: " << path;
        }
    }
};

struct GLTFChannel {
    int sampler;
    GLTFChannelTarget target;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["sampler"]) {
            qCDebug(modelformat) << "sampler: " << sampler;
        }
        if (defined["target"]) {
            target.dump();
        }
    }
};

namespace GLTFAnimationSamplerInterpolation {
    enum Values{
        LINEAR = 0
    };
}

struct GLTFAnimationSampler {
    int input;
    int output;
    int interpolation;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["input"]) {
            qCDebug(modelformat) << "input: " << input;
        }
        if (defined["output"]) {
            qCDebug(modelformat) << "output: " << output;
        }
        if (defined["interpolation"]) {
            qCDebug(modelformat) << "interpolation: " << interpolation;
        }
    }
};

struct GLTFAnimation {
    QVector<GLTFChannel> channels;
    QVector<GLTFAnimationSampler> samplers;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["channels"]) {
            foreach(auto channel, channels) channel.dump();
        }
        if (defined["samplers"]) {
            foreach(auto sampler, samplers) sampler.dump();
        }
    }
};

struct GLTFScene {
    QString name;
    QVector<int> nodes;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["name"]) {
            qCDebug(modelformat) << "name: " << name;
        }
        if (defined["nodes"]) {
            qCDebug(modelformat) << "nodes: ";
            foreach(int node, nodes) qCDebug(modelformat) << node;
        }
    }
};

struct GLTFSkin {
    int inverseBindMatrices;
    QVector<int> joints;
    int skeleton;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["inverseBindMatrices"]) {
            qCDebug(modelformat) << "inverseBindMatrices: " << inverseBindMatrices;
        }
        if (defined["skeleton"]) {
            qCDebug(modelformat) << "skeleton: " << skeleton;
        }
        if (defined["joints"]) {
            qCDebug(modelformat) << "joints: ";
            foreach(int joint, joints) qCDebug(modelformat) << joint;
        }
    }
};

struct GLTFTexture {
    int sampler;
    int source;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["sampler"]) {
            qCDebug(modelformat) << "sampler: " << sampler;
        }
        if (defined["source"]) {
            qCDebug(modelformat) << "source: " << sampler;
        }
    }
};

struct GLTFFile {
    GLTFAsset asset;
    int scene = 0;
    QVector<GLTFAccessor> accessors;
    QVector<GLTFAnimation> animations;
    QVector<GLTFBufferView> bufferviews;
    QVector<GLTFBuffer> buffers;
    QVector<GLTFCamera> cameras;
    QVector<GLTFImage> images;
    QVector<GLTFMaterial> materials;
    QVector<GLTFMesh> meshes;
    QVector<GLTFNode> nodes;
    QVector<GLTFSampler> samplers;
    QVector<GLTFScene> scenes;
    QVector<GLTFSkin> skins;
    QVector<GLTFTexture> textures;
    QMap<QString, bool> defined;
    void dump() {
        if (defined["asset"]) {
            asset.dump();
        }
        if (defined["scene"]) {
            qCDebug(modelformat) << "scene: " << scene;
        }
        if (defined["accessors"]) {
            foreach(auto acc, accessors) acc.dump();
        }
        if (defined["animations"]) {
            foreach(auto ani, animations) ani.dump();
        }
        if (defined["bufferviews"]) {
            foreach(auto bv, bufferviews) bv.dump();
        }
        if (defined["buffers"]) {
            foreach(auto b, buffers) b.dump();
        }
        if (defined["cameras"]) {
            foreach(auto c, cameras) c.dump();
        }
        if (defined["images"]) {
            foreach(auto i, images) i.dump();
        }
        if (defined["materials"]) {
            foreach(auto mat, materials) mat.dump();
        }
        if (defined["meshes"]) {
            foreach(auto mes, meshes) mes.dump();
        }
        if (defined["nodes"]) {
            foreach(auto nod, nodes) nod.dump();
        }
        if (defined["samplers"]) {
            foreach(auto sa, samplers) sa.dump();
        }
        if (defined["scenes"]) {
            foreach(auto sc, scenes) sc.dump();
        }
        if (defined["skins"]) {
            foreach(auto sk, nodes) sk.dump();
        }
        if (defined["textures"]) {
            foreach(auto tex, textures) tex.dump();
        }
    }
};

class GLTFSerializer : public QObject, public HFMSerializer {
    Q_OBJECT
public:
    MediaType getMediaType() const override;
    std::unique_ptr<hfm::Serializer::Factory> getFactory() const override;

    HFMModel::Pointer read(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url = hifi::URL()) override;
private:
    GLTFFile _file;
    hifi::URL _url;
    hifi::ByteArray _glbBinary;

    glm::mat4 getModelTransform(const GLTFNode& node);
    void getSkinInverseBindMatrices(std::vector<std::vector<float>>& inverseBindMatrixValues);
    void generateTargetData(int index, float weight, QVector<glm::vec3>& returnVector);

    bool buildGeometry(HFMModel& hfmModel, const hifi::VariantHash& mapping, const hifi::URL& url);
    bool parseGLTF(const hifi::ByteArray& data);

    bool getStringVal(const QJsonObject& object, const QString& fieldname,
                      QString& value, QMap<QString, bool>&  defined);
    bool getBoolVal(const QJsonObject& object, const QString& fieldname,
                    bool& value, QMap<QString, bool>&  defined);
    bool getIntVal(const QJsonObject& object, const QString& fieldname,
                   int& value, QMap<QString, bool>&  defined);
    bool getDoubleVal(const QJsonObject& object, const QString& fieldname,
                      double& value, QMap<QString, bool>&  defined);
    bool getObjectVal(const QJsonObject& object, const QString& fieldname,
                      QJsonObject& value, QMap<QString, bool>&  defined);
    bool getIntArrayVal(const QJsonObject& object, const QString& fieldname,
                        QVector<int>& values, QMap<QString, bool>&  defined);
    bool getDoubleArrayVal(const QJsonObject& object, const QString& fieldname,
                           QVector<double>& values, QMap<QString, bool>&  defined);
    bool getObjectArrayVal(const QJsonObject& object, const QString& fieldname,
                           QJsonArray& objects, QMap<QString, bool>& defined);

    hifi::ByteArray setGLBChunks(const hifi::ByteArray& data);

    graphics::MaterialKey::OpacityMapMode getMaterialAlphaMode(const QString& type);
    int getAccessorType(const QString& type);
    int getAnimationSamplerInterpolation(const QString& interpolation);
    int getCameraType(const QString& type);
    int getImageMimeType(const QString& mime);
    int getMeshPrimitiveRenderingMode(const QString& type);

    bool getIndexFromObject(const QJsonObject& object, const QString& field,
                            int& outidx, QMap<QString, bool>& defined);

    bool setAsset(const QJsonObject& object);

    GLTFAccessor::GLTFAccessorSparse::GLTFAccessorSparseIndices createAccessorSparseIndices(const QJsonObject& object);
    GLTFAccessor::GLTFAccessorSparse::GLTFAccessorSparseValues createAccessorSparseValues(const QJsonObject& object);
    GLTFAccessor::GLTFAccessorSparse createAccessorSparse(const QJsonObject& object);

    bool addAccessor(const QJsonObject& object);
    bool addAnimation(const QJsonObject& object);
    bool addBufferView(const QJsonObject& object);
    bool addBuffer(const QJsonObject& object);
    bool addCamera(const QJsonObject& object);
    bool addImage(const QJsonObject& object);
    bool addMaterial(const QJsonObject& object);
    bool addMesh(const QJsonObject& object);
    bool addNode(const QJsonObject& object);
    bool addSampler(const QJsonObject& object);
    bool addScene(const QJsonObject& object);
    bool addSkin(const QJsonObject& object);
    bool addTexture(const QJsonObject& object);

    bool readBinary(const QString& url, hifi::ByteArray& outdata);

    template<typename T, typename L>
    bool readArray(const hifi::ByteArray& bin, int byteOffset, int count,
                   QVector<L>& outarray, int accessorType, bool normalized);

    template<typename T>
    bool addArrayOfType(const hifi::ByteArray& bin, int byteOffset, int count,
                        QVector<T>& outarray, int accessorType, int componentType, bool normalized);

    template <typename T>
    bool addArrayFromAccessor(GLTFAccessor& accessor, QVector<T>& outarray);

    void retriangulate(const QVector<int>& in_indices, const QVector<glm::vec3>& in_vertices,
                       const QVector<glm::vec3>& in_normals, QVector<int>& out_indices,
                       QVector<glm::vec3>& out_vertices, QVector<glm::vec3>& out_normals);

    std::tuple<bool, hifi::ByteArray> requestData(hifi::URL& url);
    hifi::ByteArray requestEmbeddedData(const QString& url);

    QNetworkReply* request(hifi::URL& url, bool isTest);
    bool doesResourceExist(const QString& url);

    void setHFMMaterial(HFMMaterial& hfmMat, const GLTFMaterial& material);
    HFMTexture getHFMTexture(const GLTFTexture& texture);

    void glTFDebugDump();
};

#endif // hifi_GLTFSerializer_h
