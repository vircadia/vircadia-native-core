//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestFbx.h"

#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QFile>

#include <FBXReader.h>

struct MyVertex {
    vec3 position;
    vec2 texCoords;
    vec3 normal;
    uint32_t color;

    static gpu::Stream::FormatPointer getVertexFormat() {
        static const gpu::Element POSITION_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
        static const gpu::Element TEXTURE_ELEMENT { gpu::VEC2, gpu::FLOAT, gpu::UV };
        static const gpu::Element NORMAL_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
        static const gpu::Element COLOR_ELEMENT { gpu::VEC4, gpu::NUINT8, gpu::RGBA };
        gpu::Stream::FormatPointer vertexFormat { std::make_shared<gpu::Stream::Format>() };
        vertexFormat->setAttribute(gpu::Stream::POSITION, 0, POSITION_ELEMENT, offsetof(MyVertex, position));
        vertexFormat->setAttribute(gpu::Stream::TEXCOORD, 0, TEXTURE_ELEMENT, offsetof(MyVertex, texCoords));
        vertexFormat->setAttribute(gpu::Stream::COLOR, 0, COLOR_ELEMENT, offsetof(MyVertex, color));
        vertexFormat->setAttribute(gpu::Stream::NORMAL, 0, NORMAL_ELEMENT, offsetof(MyVertex, normal));
        return vertexFormat;
    }

};

struct Part {
    size_t baseVertex;
    size_t baseIndex;
    size_t materialId;
};

struct DrawElementsIndirectCommand {
    uint  count { 0 };
    uint  instanceCount { 1 };
    uint  firstIndex { 0 };
    uint  baseVertex { 0 };
    uint  baseInstance { 0 };
};


class FileDownloader : public QObject {
    Q_OBJECT
public:
    explicit FileDownloader(QUrl imageUrl, QObject *parent = 0) : QObject(parent) {
        connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileDownloaded(QNetworkReply*)));
        QNetworkRequest request(imageUrl);
        m_WebCtrl.get(request);
    }

    virtual ~FileDownloader() {}

    const QByteArray& downloadedData() const {
        return m_DownloadedData;
    }

signals:
    void downloaded();

private slots:
    void fileDownloaded(QNetworkReply* pReply) {
        m_DownloadedData = pReply->readAll();
        pReply->deleteLater();
        emit downloaded();
    }

private:
    QNetworkAccessManager m_WebCtrl;
    QByteArray m_DownloadedData;
};


static const QUrl TEST_ASSET = QString("https://s3.amazonaws.com/DreamingContent/assets/models/tardis/console.fbx");
static const mat4 TEST_ASSET_TRANSFORM = glm::translate(mat4(), vec3(0, -1.5f, 0)) * glm::scale(mat4(), vec3(0.01f));
//static const QUrl TEST_ASSET = QString("https://s3.amazonaws.com/DreamingContent/assets/simple/SimpleMilitary/Models/Vehicles/tank_02_c.fbx");
//static const mat4 TEST_ASSET_TRANSFORM = glm::translate(mat4(), vec3(0, -0.5f, 0)) * glm::scale(mat4(), vec3(0.1f));

TestFbx::TestFbx(const render::ShapePlumberPointer& shapePlumber) : _shapePlumber(shapePlumber) {
    FileDownloader* downloader = new FileDownloader(TEST_ASSET, qApp);
    QObject::connect(downloader, &FileDownloader::downloaded, [this, downloader] {
        parseFbx(downloader->downloadedData());
    });
}

bool TestFbx::isReady() const {
    return _partCount != 0;
}

void TestFbx::parseFbx(const QByteArray& fbxData) {
    QVariantHash mapping;
    FBXGeometry* fbx = readFBX(fbxData, mapping);
    size_t totalVertexCount = 0;
    size_t totalIndexCount = 0;
    size_t totalPartCount = 0;
    size_t highestIndex = 0;
    for (const auto& mesh : fbx->meshes) {
        size_t vertexCount = mesh.vertices.size();
        totalVertexCount += mesh.vertices.size();
        highestIndex = std::max(highestIndex, vertexCount);
        totalPartCount += mesh.parts.size();
        for (const auto& part : mesh.parts) {
            totalIndexCount += part.quadTrianglesIndices.size();
            totalIndexCount += part.triangleIndices.size();
        }
    }
    size_t baseVertex = 0;
    std::vector<MyVertex> vertices;
    vertices.reserve(totalVertexCount);
    std::vector<uint16_t> indices;
    indices.reserve(totalIndexCount);
    std::vector<DrawElementsIndirectCommand> parts;
    parts.reserve(totalPartCount);
    _partCount = totalPartCount;
    for (const auto& mesh : fbx->meshes) {
        baseVertex = vertices.size();

        vec3 color;
        for (const auto& part : mesh.parts) {
            DrawElementsIndirectCommand partIndirect;
            partIndirect.baseVertex = (uint)baseVertex;
            partIndirect.firstIndex = (uint)indices.size();
            partIndirect.baseInstance = (uint)parts.size();
            _partTransforms.push_back(mesh.modelTransform);
            auto material = fbx->materials[part.materialID];
            color = material.diffuseColor;
            for (auto index : part.quadTrianglesIndices) {
                indices.push_back(index);
            }
            for (auto index : part.triangleIndices) {
                indices.push_back(index);
            }
            size_t triangles = (indices.size() - partIndirect.firstIndex);
            Q_ASSERT(0 == (triangles % 3));
            //triangles /= 3;
            partIndirect.count = (uint)triangles;
            parts.push_back(partIndirect);
        }

        size_t vertexCount = mesh.vertices.size();
        for (size_t i = 0; i < vertexCount; ++i) {
            MyVertex vertex;
            vertex.position = mesh.vertices[(int)i];
            vec3 n = mesh.normals[(int)i];
            vertex.normal = n; 
            vertex.texCoords = mesh.texCoords[(int)i];
            vertex.color = toCompactColor(vec4(color, 1));
            vertices.push_back(vertex);
        }
    }

    _vertexBuffer->append(vertices);
    _indexBuffer->append(indices);
    _indirectBuffer->append(parts);
    delete fbx;
}

void TestFbx::renderTest(size_t testId, RenderArgs* args) {
    gpu::Batch& batch = *(args->_batch);
    //pipeline->pipeline
    if (_partCount) {
        for (size_t i = 0; i < _partCount; ++i) {
            batch.setModelTransform(TEST_ASSET_TRANSFORM * _partTransforms[i]);
            batch.setupNamedCalls(__FUNCTION__, [this](gpu::Batch& batch, gpu::Batch::NamedBatchData&) {
                RenderArgs args; args._batch = &batch;
                _shapePlumber->pickPipeline(&args, render::ShapeKey());
                batch.setInputBuffer(0, _vertexBuffer, 0, sizeof(MyVertex));
                batch.setIndexBuffer(gpu::UINT16, _indexBuffer, 0);
                batch.setInputFormat(MyVertex::getVertexFormat());
                batch.setIndirectBuffer(_indirectBuffer, 0);
                batch.multiDrawIndexedIndirect((uint)_partCount, gpu::TRIANGLES);
            });
        }
    }
}

#include "TestFbx.moc"
