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
#include "graphics/Geometry.h"
#include "OBJWriter.h"
#include "ModelFormatLogging.h"

// FIXME: should this live in shared? (it depends on gpu/)
#include <../graphics-scripting/src/graphics-scripting/BufferViewHelpers.h>

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
    int subMeshIndex = 0;
    out << "# OBJWriter::writeOBJToTextStream\n";

    // write out vertices (and maybe colors)
    foreach (const MeshPointer& mesh, meshes) {
        out << "# vertices::subMeshIndex " << subMeshIndex++ << "\n";
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
                glm::vec3 color = glmVecFromVariant<glm::vec3>(buffer_helpers::toVariant(colorsBufferView, colorIndex));
                //glm::vec3 color = colorsBufferView.get<glm::vec3>(colorIndex);
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
    subMeshIndex = 0;
    foreach (const MeshPointer& mesh, meshes) {
        out << "# normals::subMeshIndex " << subMeshIndex++ << "\n";
        meshNormalStartOffset.append(currentNormalStartOffset);
        const gpu::BufferView& normalsBufferView = mesh->getAttributeBuffer(gpu::Stream::InputSlot::NORMAL);
        gpu::BufferView::Index numNormals = (gpu::BufferView::Index)normalsBufferView.getNumElements();
        for (gpu::BufferView::Index i = 0; i < numNormals; i++) {
            glm::vec3 normal = glmVecFromVariant<glm::vec3>(buffer_helpers::toVariant(normalsBufferView, i));
            //glm::vec3 normal = normalsBufferView.get<glm::vec3>(i);
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
    subMeshIndex = 0;
    foreach (const MeshPointer& mesh, meshes) {
        out << "# faces::subMeshIndex " << subMeshIndex++ << "\n";
        currentVertexStartOffset = meshVertexStartOffset.takeFirst();
        currentNormalStartOffset = meshNormalStartOffset.takeFirst();

        const gpu::BufferView& partBuffer = mesh->getPartBuffer();
        const gpu::BufferView& indexBuffer = mesh->getIndexBuffer();

        graphics::Index partCount = (graphics::Index)mesh->getNumParts();
        QString name = (!mesh->displayName.size() ? QString("mesh-%1-part").arg(nth) : QString::fromStdString(mesh->displayName))
            .replace(QRegExp("[^-_a-zA-Z0-9]"), "_");
        for (int partIndex = 0; partIndex < partCount; partIndex++) {
            const graphics::Mesh::Part& part = partBuffer.get<graphics::Mesh::Part>(partIndex);

            out << QString("g %1-%2-%3\n").arg(subMeshIndex, 3, 10, QChar('0')).arg(name).arg(partIndex);

            const bool shorts = indexBuffer._element == gpu::Element::INDEX_UINT16;
            auto face = [&](uint32_t i0, uint32_t i1, uint32_t i2) {
                uint32_t index0, index1, index2;
                if (shorts) {
                    index0 = indexBuffer.get<uint16_t>(i0);
                    index1 = indexBuffer.get<uint16_t>(i1);
                    index2 = indexBuffer.get<uint16_t>(i2);
                } else {
                    index0 = indexBuffer.get<uint32_t>(i0);
                    index1 = indexBuffer.get<uint32_t>(i1);
                    index2 = indexBuffer.get<uint32_t>(i2);
                }

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
            };

            // graphics::Mesh::TRIANGLES / graphics::Mesh::QUADS
            // TODO -- handle other formats
            uint32_t len = part._startIndex + part._numIndices;
            auto stringFromTopology = [&](graphics::Mesh::Topology topo) -> QString {
                return topo == graphics::Mesh::Topology::QUADS ? "QUADS" :
                topo == graphics::Mesh::Topology::QUAD_STRIP ? "QUAD_STRIP" :
                topo == graphics::Mesh::Topology::TRIANGLES ? "TRIANGLES" :
                topo == graphics::Mesh::Topology::TRIANGLE_STRIP ? "TRIANGLE_STRIP" :
                topo == graphics::Mesh::Topology::QUAD_STRIP ? "QUAD_STRIP" :
                QString("topo:%1").arg((int)topo);
            };

            qCDebug(modelformat) << "OBJWriter -- part" << partIndex << "topo" << stringFromTopology(part._topology) << "index elements" << (shorts ? "uint16_t" : "uint32_t");
            if (part._topology == graphics::Mesh::TRIANGLES && len % 3 != 0) {
                qCDebug(modelformat) << "OBJWriter -- index buffer length isn't a multiple of 3" << len;
            }
            if (part._topology == graphics::Mesh::QUADS && len % 4 != 0) {
                qCDebug(modelformat) << "OBJWriter -- index buffer length isn't a multiple of 4" << len;
            }
            if (len > indexBuffer.getNumElements()) {
                qCDebug(modelformat) << "OBJWriter -- len > index size" << len << indexBuffer.getNumElements();
            }
            if (part._topology == graphics::Mesh::QUADS) {
                for (uint32_t idx = part._startIndex; idx+3 < len; idx += 4) {
                    face(idx+0, idx+1, idx+3);
                    face(idx+1, idx+2, idx+3);
                }
            } else if (part._topology == graphics::Mesh::TRIANGLES) {
                for (uint32_t idx = part._startIndex; idx+2 < len; idx += 3) {
                    face(idx+0, idx+1, idx+2);
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
