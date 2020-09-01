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

    template <typename T>
    QVariant glmVecToVariant(const T& v, bool asArray = false) {
        static const auto len = T().length();
        if (asArray) {
            QVariantList list;
            for (int i = 0; i < len; i++) {
                list << v[i];
            }
            return list;
        } else {
            QVariantMap obj;
            for (int i = 0; i < len; i++) {
                obj[XYZW[i]] = v[i];
            }
            return obj;
        }
    }

    template <typename T>
    const T glmVecFromVariant(const QVariant& v) {
        auto isMap = v.type() == (QVariant::Type)QMetaType::QVariantMap;
        static const auto len = T().length();
        const auto& components = isMap ? XYZW : ZERO123;
        T result;
        QVariantMap map;
        QVariantList list;
        if (isMap) {
            map = v.toMap();
        } else {
            list = v.toList();
        }
        for (int i = 0; i < len; i++) {
            float value;
            if (isMap) {
                value = map.value(components[i]).toFloat();
            } else {
                value = list.value(i).toFloat();
            }
#ifdef DEBUG_BUFFERVIEW_HELPERS
            if (value != value) { // NAN
                qWarning().nospace() << "vec" << len << "." << components[i] << " NAN received from script.... " << v.toString();
            }
#endif
            result[i] = value;
        }
        return result;
    }


    glm::uint32 forEachVariant(const gpu::BufferView& view, std::function<bool(glm::uint32 index, const QVariant& value)> func, const char* hint = "");
    template <typename T> glm::uint32 forEach(const gpu::BufferView& view, std::function<bool(glm::uint32 index, const T& value)> func);

    template <typename T> gpu::BufferView newFromVector(const QVector<T>& elements, const gpu::Element& elementType);
    template <typename T> gpu::BufferView newFromVariantList(const QVariantList& list, const gpu::Element& elementType);

    template <typename T> QVector<T> variantToVector(const QVariant& value);
    template <typename T> QVector<T> bufferToVector(const gpu::BufferView& view, const char *hint = "");

    // note: these do value conversions from the underlying buffer type into the template type
    template <typename T> T getValue(const gpu::BufferView& view, glm::uint32 index, const char* hint = "");
    template <typename T> bool setValue(const gpu::BufferView& view, glm::uint32 index, const T& value, const char* hint = "");

    gpu::BufferView clone(const gpu::BufferView& input);
    gpu::BufferView resized(const gpu::BufferView& input, glm::uint32 numElements);

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
