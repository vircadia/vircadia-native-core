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
#include <glm/gtc/packing.hpp>

#include "GpuHelpers.h"
#include "GLMHelpers.h"

namespace graphics {
    class Mesh;
    using MeshPointer = std::shared_ptr<Mesh>;
}

class Extents;
class AABox;

namespace buffer_helpers {
    extern QMap<QString,int> ATTRIBUTES;
    extern const std::array<const char*, 4> XYZW;
    extern const std::array<const char*, 4> ZERO123;

    template <typename T> QVariant glmVecToVariant(const T& v, bool asArray = false);
    template <typename T> const T glmVecFromVariant(const QVariant& v);

    glm::uint32 forEachVariant(const gpu::BufferView& view, std::function<bool(glm::uint32 index, const QVariant& value)> func, const char* hint = "");
    template <typename T> glm::uint32 forEach(const gpu::BufferView& view, std::function<bool(glm::uint32 index, const T& value)> func);

    template <typename T> gpu::BufferView newFromVector(const QVector<T>& elements, const gpu::Element& elementType);
    template <typename T> gpu::BufferView newFromVariantList(const QVariantList& list, const gpu::Element& elementType);

    template <typename T> QVector<T> variantToVector(const QVariant& list);
    template <typename T> QVector<T> bufferToVector(const gpu::BufferView& view, const char *hint = "");

    // note: these do value conversions from the underlying buffer type into the template type
    template <typename T> T getValue(const gpu::BufferView& view, glm::uint32 index, const char* hint = "");
    template <typename T> bool setValue(const gpu::BufferView& view, glm::uint32 index, const T& value, const char* hint = "");

    gpu::BufferView clone(const gpu::BufferView& input);
    gpu::BufferView resized(const gpu::BufferView& input, glm::uint32 numElements);

    inline void packNormalAndTangent(glm::vec3 normal, glm::vec3 tangent, glm::uint32& packedNormal, glm::uint32& packedTangent) {
        auto absNormal = glm::abs(normal);
        auto absTangent = glm::abs(tangent);
        normal /= glm::max(1e-6f, glm::max(glm::max(absNormal.x, absNormal.y), absNormal.z));
        tangent /= glm::max(1e-6f, glm::max(glm::max(absTangent.x, absTangent.y), absTangent.z));
        normal = glm::clamp(normal, -1.0f, 1.0f);
        tangent = glm::clamp(tangent, -1.0f, 1.0f);
        normal *= 511.0f;
        tangent *= 511.0f;

        glm::detail::i10i10i10i2 normalStruct;
        glm::detail::i10i10i10i2 tangentStruct;
        normalStruct.data.x = fastLrintf(normal.x);
        normalStruct.data.y = fastLrintf(normal.y);
        normalStruct.data.z = fastLrintf(normal.z);
        normalStruct.data.w = 0;
        tangentStruct.data.x = fastLrintf(tangent.x);
        tangentStruct.data.y = fastLrintf(tangent.y);
        tangentStruct.data.z = fastLrintf(tangent.z);
        tangentStruct.data.w = 0;
        packedNormal = normalStruct.pack;
        packedTangent = tangentStruct.pack;
    }

    namespace mesh {
        glm::uint32 forEachVertex(const graphics::MeshPointer& mesh, std::function<bool(glm::uint32 index, const QVariantMap& attributes)> func);
        bool setVertexAttributes(const graphics::MeshPointer& mesh, glm::uint32 index, const QVariantMap& attributes);
        QVariant getVertexAttributes(const graphics::MeshPointer& mesh, glm::uint32 index);
        graphics::MeshPointer clone(const graphics::MeshPointer& mesh);
        gpu::BufferView getBufferView(const graphics::MeshPointer& mesh, quint8 slot);
        std::map<QString, gpu::BufferView> getAllBufferViews(const graphics::MeshPointer& mesh);
        template <typename T> QVector<T> attributeToVector(const graphics::MeshPointer& mesh, gpu::Stream::InputSlot slot) {
            return bufferToVector<T>(getBufferView(mesh, slot), qUtf8Printable(gpu::toString(slot)));
        }
    }
}
