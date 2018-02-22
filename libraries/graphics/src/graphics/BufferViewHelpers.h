//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QtCore>
#include <memory>
#include <glm/glm.hpp>

namespace gpu {
    class BufferView;
    class Element;
}

namespace graphics {
    class Mesh;
    using MeshPointer = std::shared_ptr<Mesh>;
}

class Extents;
class AABox;

struct buffer_helpers {
    template <typename T> static QVariant glmVecToVariant(const T& v, bool asArray = false);
    template <typename T> static const T glmVecFromVariant(const QVariant& v);

    static graphics::MeshPointer cloneMesh(graphics::MeshPointer mesh);
    static QMap<QString,int> ATTRIBUTES;
    static std::map<QString, gpu::BufferView> gatherBufferViews(graphics::MeshPointer mesh, const QStringList& expandToMatchPositions = QStringList());
    static bool recalculateNormals(graphics::MeshPointer meshProxy);
    static gpu::BufferView getBufferView(graphics::MeshPointer mesh, quint8 slot);

    static QVariant toVariant(const Extents& box);
    static QVariant toVariant(const AABox& box);
    static QVariant toVariant(const glm::mat4& mat4);
    static QVariant toVariant(const gpu::BufferView& view, quint32 index, bool asArray = false, const char* hint = "");

    static bool fromVariant(const gpu::BufferView& view, quint32 index, const QVariant& v);

    template <typename T> static gpu::BufferView fromVector(const QVector<T>& elements, const gpu::Element& elementType);

    template <typename T> static QVector<T> toVector(const gpu::BufferView& view, const char *hint = "");
    template <typename T> static T convert(const gpu::BufferView& view, quint32 index, const char* hint = "");

    static gpu::BufferView clone(const gpu::BufferView& input);
    static gpu::BufferView resize(const gpu::BufferView& input, quint32 numElements);

    static void packNormalAndTangent(glm::vec3 normal, glm::vec3 tangent, glm::uint32& packedNormal, glm::uint32& packedTangent);
};
