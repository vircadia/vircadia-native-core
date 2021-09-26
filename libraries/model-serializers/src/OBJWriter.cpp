//
//  OBJWriter.cpp
//  libraries/model-serializers/src
//
//  Created by Seth Alves on 2017-1-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OBJWriter.h"

#include <QFile>
#include <QFileInfo>
#include <graphics/BufferViewHelpers.h>
#include <graphics/Geometry.h>
#include <hfm/ModelFormatLogging.h>

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

        auto vertices = buffer_helpers::bufferToVector<glm::vec3>(mesh->getVertexBuffer(), "mesh.vertices");
        auto colors = buffer_helpers::mesh::attributeToVector<glm::vec3>(mesh, gpu::Stream::COLOR);

        gpu::BufferView::Index numColors = colors.size();

        int i = 0;
        for (const auto& v : vertices) {
            out << "v ";
            out << formatFloat(v[0]) << " ";
            out << formatFloat(v[1]) << " ";
            out << formatFloat(v[2]);
            if (i < numColors) {
                const glm::vec3& color = colors[i];
                out << " " << formatFloat(color[0]);
                out << " " << formatFloat(color[1]);
                out << " " << formatFloat(color[2]);
            }
            out << "\n";
            i++;
        }
        currentVertexStartOffset += i;
    }
    out << "\n";

    // write out normals
    bool haveNormals = true;
    subMeshIndex = 0;
    foreach (const MeshPointer& mesh, meshes) {
        out << "# normals::subMeshIndex " << subMeshIndex++ << "\n";
        meshNormalStartOffset.append(currentNormalStartOffset);
        auto normals = buffer_helpers::mesh::attributeToVector<glm::vec3>(mesh, gpu::Stream::NORMAL);
        for (const auto& normal : normals) {
            out << "vn ";
            out << formatFloat(normal[0]) << " ";
            out << formatFloat(normal[1]) << " ";
            out << formatFloat(normal[2]) << "\n";
        }
        currentNormalStartOffset += normals.size();
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

            auto indices = buffer_helpers::bufferToVector<gpu::uint32>(mesh->getIndexBuffer(), "mesh.indices");
            auto face = [&](uint32_t i0, uint32_t i1, uint32_t i2) {
                out << "f ";
                if (haveNormals) {
                    out << currentVertexStartOffset + indices[i0] + 1 << "//" << currentVertexStartOffset + indices[i0] + 1 << " ";
                    out << currentVertexStartOffset + indices[i1] + 1 << "//" << currentVertexStartOffset + indices[i1] + 1 << " ";
                    out << currentVertexStartOffset + indices[i2] + 1 << "//" << currentVertexStartOffset + indices[i2] + 1 << "\n";
                } else {
                    out << currentVertexStartOffset + indices[i0] + 1 << " ";
                    out << currentVertexStartOffset + indices[i1] + 1 << " ";
                    out << currentVertexStartOffset + indices[i2] + 1 << "\n";
                }
            };

            uint32_t len = part._startIndex + part._numIndices;
            qCDebug(modelformat) << "OBJWriter -- part" << partIndex << "topo" << part._topology << "index elements";
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
