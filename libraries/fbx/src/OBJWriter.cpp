//
//  OBJWriter.cpp
//  libraries/fbx/src/
//
//  Created by Seth Alves on 2017-1-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QFile>
#include <QFileInfo>
#include "model/Geometry.h"
#include "OBJWriter.h"
#include "ModelFormatLogging.h"

static QString formatFloat(double n) {
    // limit precision to 6, but don't output trailing zeros.
    QString s = QString::number(n, 'f', 6);
    while (s.endsWith("0")) {
        s.remove(s.size() - 1, 1);
    }
    if (s.endsWith(".")) {
        s.remove(s.size() - 1, 1);
    }

    // check for non-numbers.  if we get NaN or inf or scientific notation, just return 0
    for (int i = 0; i < s.length(); i++) {
        auto c = s.at(i).toLatin1();
        if (c != '-' &&
            c != '.' &&
            (c < '0' || c > '9')) {
            qCDebug(modelformat) << "OBJWriter zeroing bad vertex coordinate:" << s << "because of" << c;
            return QString("0");
        }
    }

    return s;
}

bool writeOBJToTextStream(QTextStream& out, QList<MeshPointer> meshes) {
    // each mesh's vertices are numbered from zero.  We're combining all their vertices into one list here,
    // so keep track of the start index for each mesh.
    QList<int> meshVertexStartOffset;
    QList<int> meshNormalStartOffset;
    int currentVertexStartOffset = 0;
    int currentNormalStartOffset = 0;

    // write out vertices (and maybe colors)
    foreach (const MeshPointer& mesh, meshes) {
        meshVertexStartOffset.append(currentVertexStartOffset);
        const gpu::BufferView& vertexBuffer = mesh->getVertexBuffer();

        const gpu::BufferView& colorsBufferView = mesh->getAttributeBuffer(gpu::Stream::COLOR);
        gpu::BufferView::Index numColors = (gpu::BufferView::Index)colorsBufferView.getNumElements();
        gpu::BufferView::Index colorIndex = 0;

        int vertexCount = 0;
        gpu::BufferView::Iterator<const glm::vec3> vertexItr = vertexBuffer.cbegin<const glm::vec3>();
        while (vertexItr != vertexBuffer.cend<const glm::vec3>()) {
            glm::vec3 v = *vertexItr;
            out << "v ";
            out << formatFloat(v[0]) << " ";
            out << formatFloat(v[1]) << " ";
            out << formatFloat(v[2]);
            if (colorIndex < numColors) {
                glm::vec3 color = colorsBufferView.get<glm::vec3>(colorIndex);
                out << " " << formatFloat(color[0]);
                out << " " << formatFloat(color[1]);
                out << " " << formatFloat(color[2]);
                colorIndex++;
            }
            out << "\n";
            vertexItr++;
            vertexCount++;
        }
        currentVertexStartOffset += vertexCount;
    }
    out << "\n";

    // write out normals
    bool haveNormals = true;
    foreach (const MeshPointer& mesh, meshes) {
        meshNormalStartOffset.append(currentNormalStartOffset);
        const gpu::BufferView& normalsBufferView = mesh->getAttributeBuffer(gpu::Stream::InputSlot::NORMAL);
        gpu::BufferView::Index numNormals = (gpu::BufferView::Index)normalsBufferView.getNumElements();
        for (gpu::BufferView::Index i = 0; i < numNormals; i++) {
            glm::vec3 normal = normalsBufferView.get<glm::vec3>(i);
            out << "vn ";
            out << formatFloat(normal[0]) << " ";
            out << formatFloat(normal[1]) << " ";
            out << formatFloat(normal[2]) << "\n";
        }
        currentNormalStartOffset += numNormals;
    }
    out << "\n";

    // write out faces
    int nth = 0;
    foreach (const MeshPointer& mesh, meshes) {
        currentVertexStartOffset = meshVertexStartOffset.takeFirst();
        currentNormalStartOffset = meshNormalStartOffset.takeFirst();

        const gpu::BufferView& partBuffer = mesh->getPartBuffer();
        const gpu::BufferView& indexBuffer = mesh->getIndexBuffer();

        model::Index partCount = (model::Index)mesh->getNumParts();
        for (int partIndex = 0; partIndex < partCount; partIndex++) {
            const model::Mesh::Part& part = partBuffer.get<model::Mesh::Part>(partIndex);

            out << "g part-" << nth++ << "\n";

            // model::Mesh::TRIANGLES
            // TODO -- handle other formats
            gpu::BufferView::Iterator<const uint32_t> indexItr = indexBuffer.cbegin<uint32_t>();
            indexItr += part._startIndex;

            int indexCount = 0;
            while (indexItr != indexBuffer.cend<uint32_t>() && indexCount < part._numIndices) {
                uint32_t index0 = *indexItr;
                indexItr++;
                indexCount++;
                if (indexItr == indexBuffer.cend<uint32_t>() || indexCount >= part._numIndices) {
                    qCDebug(modelformat) << "OBJWriter -- index buffer length isn't multiple of 3";
                    break;
                }
                uint32_t index1 = *indexItr;
                indexItr++;
                indexCount++;
                if (indexItr == indexBuffer.cend<uint32_t>() || indexCount >= part._numIndices) {
                    qCDebug(modelformat) << "OBJWriter -- index buffer length isn't multiple of 3";
                    break;
                }
                uint32_t index2 = *indexItr;
                indexItr++;
                indexCount++;

                out << "f ";
                if (haveNormals) {
                    out << currentVertexStartOffset + index0 + 1 << "//" << currentVertexStartOffset + index0 + 1 << " ";
                    out << currentVertexStartOffset + index1 + 1 << "//" << currentVertexStartOffset + index1 + 1 << " ";
                    out << currentVertexStartOffset + index2 + 1 << "//" << currentVertexStartOffset + index2 + 1 << "\n";
                } else {
                    out << currentVertexStartOffset + index0 + 1 << " ";
                    out << currentVertexStartOffset + index1 + 1 << " ";
                    out << currentVertexStartOffset + index2 + 1 << "\n";
                }
            }
            out << "\n";
        }
    }

    return true;
}

bool writeOBJToFile(QString path, QList<MeshPointer> meshes) {
    if (QFileInfo(path).exists() && !QFile::remove(path)) {
        qCDebug(modelformat) << "OBJ writer failed, file exists:" << path;
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCDebug(modelformat) << "OBJ writer failed to open output file:" << path;
        return false;
    }

    QTextStream outStream(&file);

    bool success;
    success = writeOBJToTextStream(outStream, meshes);

    file.close();
    return success;
}

QString writeOBJToString(QList<MeshPointer> meshes) {
    QString result;
    QTextStream outStream(&result, QIODevice::ReadWrite);
    bool success;
    success = writeOBJToTextStream(outStream, meshes);
    if (success) {
        return result;
    }
    return QString("");
}
