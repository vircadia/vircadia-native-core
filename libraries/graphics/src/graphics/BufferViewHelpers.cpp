//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BufferViewHelpers.h"

#include <QDebug>
#include <QVariant>

#include <gpu/Buffer.h>
#include <gpu/Format.h>
#include <gpu/Stream.h>

#include "Geometry.h"

#include <Extents.h>
#include <AABox.h>

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/detail/type_vec.hpp>

namespace glm {
    using hvec2 = glm::tvec2<glm::detail::hdata>;
    using hvec4 = glm::tvec4<glm::detail::hdata>;
}

namespace {
    QLoggingCategory bufferhelper_logging{ "hifi.bufferview" };
}


const std::array<const char*, 4> buffer_helpers::XYZW = { { "x", "y", "z", "w" } };
const std::array<const char*, 4> buffer_helpers::ZERO123 = { { "0", "1", "2", "3" } };

gpu::BufferView buffer_helpers::getBufferView(graphics::MeshPointer mesh, gpu::Stream::Slot slot) {
    return slot == gpu::Stream::POSITION ? mesh->getVertexBuffer() : mesh->getAttributeBuffer(slot);
}

QMap<QString,int> buffer_helpers::ATTRIBUTES{
    {"position", gpu::Stream::POSITION },
    {"normal", gpu::Stream::NORMAL },
    {"color", gpu::Stream::COLOR },
    {"tangent", gpu::Stream::TEXCOORD0 },
    {"skin_cluster_index", gpu::Stream::SKIN_CLUSTER_INDEX },
    {"skin_cluster_weight", gpu::Stream::SKIN_CLUSTER_WEIGHT },
    {"texcoord0", gpu::Stream::TEXCOORD0 },
    {"texcoord1", gpu::Stream::TEXCOORD1 },
    {"texcoord2", gpu::Stream::TEXCOORD2 },
    {"texcoord3", gpu::Stream::TEXCOORD3 },
    {"texcoord4", gpu::Stream::TEXCOORD4 },
};


namespace {
    bool boundsCheck(const gpu::BufferView& view, quint32 index) {
        const auto byteLength = view._element.getSize();
        return (
            index < view.getNumElements() &&
            index * byteLength < (view._size - 1) * byteLength
        );
    }

    template <typename T> QVariant getBufferViewElement(const gpu::BufferView& view, quint32 index, bool asArray = false) {
        return buffer_helpers::glmVecToVariant(view.get<T>(index), asArray);
    }

    template <typename T> void setBufferViewElement(const gpu::BufferView& view, quint32 index, const QVariant& v) {
        view.edit<T>(index) = buffer_helpers::glmVecFromVariant<T>(v);
    }
}

void buffer_helpers::packNormalAndTangent(glm::vec3 normal, glm::vec3 tangent, glm::uint32& packedNormal, glm::uint32& packedTangent) {
    auto absNormal = glm::abs(normal);
    auto absTangent = glm::abs(tangent);
    normal /= glm::max(1e-6f, glm::max(glm::max(absNormal.x, absNormal.y), absNormal.z));
    tangent /= glm::max(1e-6f, glm::max(glm::max(absTangent.x, absTangent.y), absTangent.z));
    normal = glm::clamp(normal, -1.0f, 1.0f);
    tangent = glm::clamp(tangent, -1.0f, 1.0f);
    normal *= 511.0f;
    tangent *= 511.0f;
    normal = glm::round(normal);
    tangent = glm::round(tangent);

    glm::detail::i10i10i10i2 normalStruct;
    glm::detail::i10i10i10i2 tangentStruct;
    normalStruct.data.x = int(normal.x);
    normalStruct.data.y = int(normal.y);
    normalStruct.data.z = int(normal.z);
    normalStruct.data.w = 0;
    tangentStruct.data.x = int(tangent.x);
    tangentStruct.data.y = int(tangent.y);
    tangentStruct.data.z = int(tangent.z);
    tangentStruct.data.w = 0;
    packedNormal = normalStruct.pack;
    packedTangent = tangentStruct.pack;
}

bool buffer_helpers::fromVariant(const gpu::BufferView& view, quint32 index, const QVariant& v) {
    const auto& element = view._element;
    const auto vecN = element.getScalarCount();
    const auto dataType = element.getType();
    const auto byteLength = element.getSize();
    const auto BYTES_PER_ELEMENT = byteLength / vecN;

    if (BYTES_PER_ELEMENT == 1) {
        switch(vecN) {
        case 2: setBufferViewElement<glm::u8vec2>(view, index, v); return true;
        case 3: setBufferViewElement<glm::u8vec3>(view, index, v); return true;
        case 4: {
            if (element == gpu::Element::COLOR_RGBA_32) {
                glm::uint32 rawColor;// = glm::packUnorm4x8(glm::vec4(glmVecFromVariant<glm::vec3>(v), 0.0f));
                glm::uint32 unused;
                packNormalAndTangent(glmVecFromVariant<glm::vec3>(v), glm::vec3(), rawColor, unused);
                view.edit<glm::uint32>(index) = rawColor;
                return true;
            } else if (element == gpu::Element::VEC4F_NORMALIZED_XYZ10W2) {
                glm::uint32 packedNormal;// = glm::packSnorm3x10_1x2(glm::vec4(glmVecFromVariant<glm::vec3>(v), 0.0f));
                glm::uint32 unused;
                packNormalAndTangent(glm::vec3(), glmVecFromVariant<glm::vec3>(v), unused, packedNormal);
                view.edit<glm::uint32>(index) = packedNormal;
                return true;
            }
            setBufferViewElement<glm::u8vec4>(view, index, v); return true;
        }
        }
    } else if (BYTES_PER_ELEMENT == 2) {
        if (dataType == gpu::HALF) {
            switch(vecN) {
            case 2: view.edit<glm::int16>(index) = glm::packSnorm2x8(glmVecFromVariant<glm::vec2>(v)); return true;
            case 4: view.edit<glm::int32>(index) = glm::packSnorm4x8(glmVecFromVariant<glm::vec4>(v)); return true;
            default: return false;
            }
        }
        switch(vecN) {
        case 2: setBufferViewElement<glm::u16vec2>(view, index, v); return true;
        case 3: setBufferViewElement<glm::u16vec3>(view, index, v); return true;
        case 4: setBufferViewElement<glm::u16vec4>(view, index, v); return true;
        }
    } else if (BYTES_PER_ELEMENT == 4) {
        if (dataType == gpu::FLOAT) {
            switch(vecN) {
            case 2: setBufferViewElement<glm::vec2>(view, index, v); return true;
            case 3: setBufferViewElement<glm::vec3>(view, index, v); return true;
            case 4: setBufferViewElement<glm::vec4>(view, index, v); return true;
            }
        } else {
            switch(vecN) {
            case 2: setBufferViewElement<glm::u32vec2>(view, index, v); return true;
            case 3: setBufferViewElement<glm::u32vec3>(view, index, v); return true;
            case 4: setBufferViewElement<glm::u32vec4>(view, index, v); return true;
            }
        }
    }
    return false;
}

QVariant buffer_helpers::toVariant(const gpu::BufferView& view, quint32 index, bool asArray, const char* hint) {
    const auto& element = view._element;
    const auto vecN = element.getScalarCount();
    const auto dataType = element.getType();
    const auto byteLength = element.getSize();
    const auto BYTES_PER_ELEMENT = byteLength / vecN;
    Q_ASSERT(index < view.getNumElements());
    if (!boundsCheck(view, index)) {
        // sanity checks
        auto byteOffset = index * vecN * BYTES_PER_ELEMENT;
        auto maxByteOffset = (view._size - 1) * vecN * BYTES_PER_ELEMENT;
        if (byteOffset > maxByteOffset) {
            qDebug() << "toVariant -- byteOffset out of range " << byteOffset << " < " << maxByteOffset;
            qDebug() << "toVariant -- index: " << index << "numElements" << view.getNumElements();
            qDebug() << "toVariant -- vecN: " << vecN << "byteLength" << byteLength << "BYTES_PER_ELEMENT" << BYTES_PER_ELEMENT;
        }
        Q_ASSERT(byteOffset <= maxByteOffset);
    }
    if (BYTES_PER_ELEMENT == 1) {
        switch(vecN) {
        case 2: return getBufferViewElement<glm::u8vec2>(view, index, asArray);
        case 3: return getBufferViewElement<glm::u8vec3>(view, index, asArray);
        case 4: {
            if (element == gpu::Element::COLOR_RGBA_32) {
                auto rawColor = view.get<glm::uint32>(index);
                return glmVecToVariant(glm::vec3(glm::unpackUnorm4x8(rawColor)));
            } else if (element == gpu::Element::VEC4F_NORMALIZED_XYZ10W2) {
                auto packedNormal = view.get<glm::uint32>(index);
                return glmVecToVariant(glm::vec3(glm::unpackSnorm3x10_1x2(packedNormal)));
            }

            return getBufferViewElement<glm::u8vec4>(view, index, asArray);
        }
        }
    } else if (BYTES_PER_ELEMENT == 2) {
        if (dataType == gpu::HALF) {
            switch(vecN) {
            case 2: return glmVecToVariant(glm::vec2(glm::unpackSnorm2x8(view.get<glm::int16>(index))));
            case 4: return glmVecToVariant(glm::vec4(glm::unpackSnorm4x8(view.get<glm::int32>(index))));
            }
        }
        switch(vecN) {
        case 2: return getBufferViewElement<glm::u16vec2>(view, index, asArray);
        case 3: return getBufferViewElement<glm::u16vec3>(view, index, asArray);
        case 4: return getBufferViewElement<glm::u16vec4>(view, index, asArray);
        }
    } else if (BYTES_PER_ELEMENT == 4) {
        if (dataType == gpu::FLOAT) {
            switch(vecN) {
            case 2: return getBufferViewElement<glm::vec2>(view, index, asArray);
            case 3: return getBufferViewElement<glm::vec3>(view, index, asArray);
            case 4: return getBufferViewElement<glm::vec4>(view, index, asArray);
            }
        } else {
            switch(vecN) {
            case 2: return getBufferViewElement<glm::u32vec2>(view, index, asArray);
            case 3: return getBufferViewElement<glm::u32vec3>(view, index, asArray);
            case 4: return getBufferViewElement<glm::u32vec4>(view, index, asArray);
            }
        }
    }
    return QVariant();
}

template <typename T>
QVariant buffer_helpers::glmVecToVariant(const T& v, bool asArray /*= false*/) {
    static const auto len = T().length();
    if (asArray) {
        QVariantList list;
        for (int i = 0; i < len ; i++) {
            list << v[i];
        }
        return list;
    } else {
        QVariantMap obj;
        for (int i = 0; i < len ; i++) {
            obj[XYZW[i]] = v[i];
        }
        return obj;
    }
}

template <typename T>
const T buffer_helpers::glmVecFromVariant(const QVariant& v) {
    auto isMap = v.type() == (QVariant::Type)QMetaType::QVariantMap;
    static const auto len = T().length();
    const auto& components = isMap ? XYZW : ZERO123;
    T result;
    QVariantMap map;
    QVariantList list;
    if (isMap) map = v.toMap(); else list = v.toList();
    for (int i = 0; i < len ; i++) {
        float value;
        if (isMap) {
            value = map.value(components[i]).toFloat();
        } else {
            value = list.value(i).toFloat();
        }
#ifdef DEBUG_BUFFERVIEW_SCRIPTING
        if (value != value) { // NAN
            qWarning().nospace()<< "vec" << len << "." << components[i] << " NAN received from script.... " << v.toString();
        }
#endif        
        result[i] = value;
    }
    return result;
}

template <typename T>
gpu::BufferView buffer_helpers::fromVector(const QVector<T>& elements, const gpu::Element& elementType) {
    auto vertexBuffer = std::make_shared<gpu::Buffer>(elements.size() * sizeof(T), (gpu::Byte*)elements.data());
    return { vertexBuffer, 0, vertexBuffer->getSize(),sizeof(T), elementType };
}

namespace {
    template <typename T>
    gpu::BufferView _fromVector(const QVector<T>& elements, const gpu::Element& elementType) {
        auto vertexBuffer = std::make_shared<gpu::Buffer>(elements.size() * sizeof(T), (gpu::Byte*)elements.data());
        return { vertexBuffer, 0, vertexBuffer->getSize(),sizeof(T), elementType };
    }
}

template<> gpu::BufferView buffer_helpers::fromVector<unsigned int>(
    const QVector<unsigned int>& elements, const gpu::Element& elementType
) { return _fromVector(elements, elementType); }

template<> gpu::BufferView buffer_helpers::fromVector<glm::vec3>(
    const QVector<glm::vec3>& elements, const gpu::Element& elementType
) { return _fromVector(elements, elementType); }

template <typename T> struct GpuVec4ToGlm;
template <typename T> struct GpuScalarToGlm;

struct GpuToGlmAdapter {
    static float error(const QString& name, const gpu::BufferView& view, quint32 index, const char *hint) {
        qDebug() << QString("GpuToGlmAdapter:: unhandled type=%1(element=%2) size=%3(per=%4) vec%5 hint=%6 #%7")
            .arg(name)
            .arg(view._element.getType())
            .arg(view._element.getSize())
            .arg(view._element.getSize() / view._element.getScalarCount())
            .arg(view._element.getScalarCount())
            .arg(hint)
            .arg(view.getNumElements());
        Q_ASSERT(false);
        assert(false);
        return NAN;
    }
};

template <typename T> struct GpuScalarToGlm : GpuToGlmAdapter {
    static T get(const gpu::BufferView& view, quint32 index, const char *hint) { switch(view._element.getType()) {
        case gpu::UINT32: return view.get<glm::uint32>(index);
        case gpu::UINT16: return view.get<glm::uint16>(index);
        case gpu::UINT8: return view.get<glm::uint8>(index);
        case gpu::INT32: return view.get<glm::int32>(index);
        case gpu::INT16: return view.get<glm::int16>(index);
        case gpu::INT8: return view.get<glm::int8>(index);
        case gpu::FLOAT: return view.get<glm::float32>(index);
        case gpu::HALF: return T(glm::unpackSnorm1x8(view.get<glm::int8>(index)));
        default: break;
        } return T(error("GpuScalarToGlm", view, index, hint));
    }
};

template <typename T> struct GpuVec2ToGlm : GpuToGlmAdapter { static T get(const gpu::BufferView& view, quint32 index, const char *hint) { switch(view._element.getType()) {
        case gpu::UINT32: return view.get<glm::u32vec2>(index);
        case gpu::UINT16: return view.get<glm::u16vec2>(index);
        case gpu::UINT8: return view.get<glm::u8vec2>(index);
        case gpu::INT32: return view.get<glm::i32vec2>(index);
        case gpu::INT16: return view.get<glm::i16vec2>(index);
        case gpu::INT8: return view.get<glm::i8vec2>(index);
        case gpu::FLOAT: return view.get<glm::fvec2>(index);
        case gpu::HALF: return glm::unpackSnorm2x8(view.get<glm::int16>(index));
        default: break;
        } return T(error("GpuVec2ToGlm", view, index, hint)); }};


template <typename T> struct GpuVec3ToGlm  : GpuToGlmAdapter  { static T get(const gpu::BufferView& view, quint32 index, const char *hint) { switch(view._element.getType()) {
        case gpu::UINT32: return view.get<glm::u32vec3>(index);
        case gpu::UINT16: return view.get<glm::u16vec3>(index);
        case gpu::UINT8: return view.get<glm::u8vec3>(index);
        case gpu::INT32: return view.get<glm::i32vec3>(index);
        case gpu::INT16: return view.get<glm::i16vec3>(index);
        case gpu::INT8: return view.get<glm::i8vec3>(index);
        case gpu::FLOAT: return view.get<glm::fvec3>(index);
        case gpu::HALF:
        case gpu::NUINT8:
        case gpu::NINT2_10_10_10:
            if (view._element.getSize() == sizeof(glm::int32)) {
                return GpuVec4ToGlm<T>::get(view, index, hint);
            }
        default: break;
        } return T(error("GpuVec3ToGlm", view, index, hint)); }};

template <typename T> struct GpuVec4ToGlm : GpuToGlmAdapter { static T get(const gpu::BufferView& view, quint32 index, const char *hint) {
    assert(view._element.getSize() == sizeof(glm::int32));
    switch(view._element.getType()) {
    case gpu::UINT32: return view.get<glm::u32vec4>(index);
    case gpu::UINT16: return view.get<glm::u16vec4>(index);
    case gpu::UINT8: return view.get<glm::u8vec4>(index);
    case gpu::INT32: return view.get<glm::i32vec4>(index);
    case gpu::INT16: return view.get<glm::i16vec4>(index);
    case gpu::INT8: return view.get<glm::i8vec4>(index);
    case gpu::NUINT32: break;
    case gpu::NUINT16: break;
    case gpu::NUINT8: return glm::unpackUnorm4x8(view.get<glm::uint32>(index));
    case gpu::NUINT2: break;
    case gpu::NINT32: break;
    case gpu::NINT16: break;
    case gpu::NINT8: break;
    case gpu::COMPRESSED: break;
    case gpu::NUM_TYPES: break;
    case gpu::FLOAT: return view.get<glm::fvec4>(index);
    case gpu::HALF: return glm::unpackSnorm4x8(view.get<glm::int32>(index));
    case gpu::NINT2_10_10_10: return glm::unpackSnorm3x10_1x2(view.get<glm::uint32>(index));
    } return T(error("GpuVec4ToGlm", view, index, hint)); }};


template <typename FUNC, typename T>
struct getVec {
    static QVector<T> __to_vector__(const gpu::BufferView& view, const char *hint) {
        QVector<T> result;
        const quint32 count = (quint32)view.getNumElements();
        result.resize(count);
        for (quint32 i = 0; i < count; i++) {
            result[i] = FUNC::get(view, i, hint);
        }
        return result;
    }
    static T __to_value__(const gpu::BufferView& view, quint32 index, const char *hint) {
        assert(boundsCheck(view, index));
        return FUNC::get(view, index, hint);
    }
};

// BufferView => QVector<T>
template <> QVector<int> buffer_helpers::toVector<int>(const gpu::BufferView& view, const char *hint) {
    return getVec<GpuScalarToGlm<int>,int>::__to_vector__(view, hint);
}
template <> QVector<glm::vec2> buffer_helpers::toVector<glm::vec2>(const gpu::BufferView& view, const char *hint) {
    return getVec<GpuVec2ToGlm<glm::vec2>,glm::vec2>::__to_vector__(view, hint);
}
template <> QVector<glm::vec3> buffer_helpers::toVector<glm::vec3>(const gpu::BufferView& view, const char *hint) {
    return getVec<GpuVec3ToGlm<glm::vec3>,glm::vec3>::__to_vector__(view, hint);
}
template <> QVector<glm::vec4> buffer_helpers::toVector<glm::vec4>(const gpu::BufferView& view, const char *hint) {
    return getVec<GpuVec4ToGlm<glm::vec4>,glm::vec4>::__to_vector__(view, hint);
}


// indexed conversion accessors (like the hypothetical "view.convert<T>(i)")
template <> int buffer_helpers::convert<int>(const gpu::BufferView& view, quint32 index, const char *hint) {
    return getVec<GpuScalarToGlm<int>,int>::__to_value__(view, index, hint);
}
template <> glm::vec2 buffer_helpers::convert<glm::vec2>(const gpu::BufferView& view, quint32 index, const char *hint) {
    return getVec<GpuVec2ToGlm<glm::vec2>,glm::vec2>::__to_value__(view, index, hint);
}
template <> glm::vec3 buffer_helpers::convert<glm::vec3>(const gpu::BufferView& view, quint32 index, const char *hint) {
    return getVec<GpuVec3ToGlm<glm::vec3>,glm::vec3>::__to_value__(view, index, hint);
}
template <> glm::vec4 buffer_helpers::convert<glm::vec4>(const gpu::BufferView& view, quint32 index, const char *hint) {
    return getVec<GpuVec4ToGlm<glm::vec4>,glm::vec4>::__to_value__(view, index, hint);
}

gpu::BufferView buffer_helpers::clone(const gpu::BufferView& input) {
    return gpu::BufferView(
        std::make_shared<gpu::Buffer>(input._buffer->getSize(), input._buffer->getData()),
        input._offset, input._size, input._stride, input._element
    );
}

// TODO: preserve existing data
gpu::BufferView buffer_helpers::resize(const gpu::BufferView& input, quint32 numElements) {
    auto effectiveSize = input._buffer->getSize() / input.getNumElements();
    qCDebug(bufferhelper_logging) << "resize input" << input.getNumElements() << input._buffer->getSize() << "effectiveSize" << effectiveSize;
    auto vsize = input._element.getSize() * numElements;
    std::unique_ptr<gpu::Byte[]> data{ new gpu::Byte[vsize] };
    memset(data.get(), 0, vsize);
    auto buffer = new gpu::Buffer(vsize, data.get());
    auto output = gpu::BufferView(buffer, input._element);
    qCDebug(bufferhelper_logging) << "resized output" << output.getNumElements() << output._buffer->getSize();
    return output;
}

graphics::MeshPointer buffer_helpers::cloneMesh(graphics::MeshPointer mesh) {
    auto clone = std::make_shared<graphics::Mesh>();
    clone->displayName = (QString::fromStdString(mesh->displayName) + "-clone").toStdString();
    clone->setIndexBuffer(buffer_helpers::clone(mesh->getIndexBuffer()));
    clone->setPartBuffer(buffer_helpers::clone(mesh->getPartBuffer()));
    auto attributeViews = buffer_helpers::gatherBufferViews(mesh);
    for (const auto& a : attributeViews) {
        auto& view = a.second;
        auto slot = buffer_helpers::ATTRIBUTES[a.first];
        auto points = buffer_helpers::clone(view);
        if (slot == gpu::Stream::POSITION) {
            clone->setVertexBuffer(points);
        } else {
            clone->addAttribute(slot, points);
        }
    }
    return clone;
}

namespace {
    // expand the corresponding attribute buffer (creating it if needed) so that it matches POSITIONS size and specified element type
    gpu::BufferView _expandedAttributeBuffer(const graphics::MeshPointer mesh, gpu::Stream::Slot slot) {
        gpu::BufferView bufferView = buffer_helpers::getBufferView(mesh, slot);
        const auto& elementType = bufferView._element;
        gpu::Size elementSize = elementType.getSize();
        auto nPositions = mesh->getNumVertices();
        auto vsize = nPositions * elementSize;
        auto diffTypes = (elementType.getType() != bufferView._element.getType()  ||
                          elementType.getSize() > bufferView._element.getSize() ||
                          elementType.getScalarCount() > bufferView._element.getScalarCount() ||
                          vsize > bufferView._size
            );
        QString hint = QString("%1").arg(slot);
#ifdef DEV_BUILD
        auto beforeCount = bufferView.getNumElements();
        auto beforeTotal = bufferView._size;
#endif
        if (bufferView.getNumElements() < nPositions || diffTypes) {
            if (!bufferView._buffer || bufferView.getNumElements() == 0) {
                qCInfo(bufferhelper_logging).nospace() << "ScriptableMesh -- adding missing mesh attribute '" << hint << "' for BufferView";
                std::unique_ptr<gpu::Byte[]> data{ new gpu::Byte[vsize] };
                memset(data.get(), 0, vsize);
                auto buffer = new gpu::Buffer(vsize, data.get());
                bufferView = gpu::BufferView(buffer, elementType);
                mesh->addAttribute(slot, bufferView);
            } else {
                qCInfo(bufferhelper_logging) << "ScriptableMesh -- resizing Buffer current:" << hint << bufferView._buffer->getSize() << "wanted:" << vsize;
                bufferView._element = elementType;
                bufferView._buffer->resize(vsize);
                bufferView._size = bufferView._buffer->getSize();
            }
        }
#ifdef DEV_BUILD
        auto afterCount = bufferView.getNumElements();
        auto afterTotal = bufferView._size;
        if (beforeTotal != afterTotal || beforeCount != afterCount) {
            QString typeName = QString("%1").arg(bufferView._element.getType());
            qCDebug(bufferhelper_logging, "NOTE:: _expandedAttributeBuffer.%s vec%d %s (before count=%lu bytes=%lu // after count=%lu bytes=%lu)",
                    hint.toStdString().c_str(), bufferView._element.getScalarCount(),
                    typeName.toStdString().c_str(), beforeCount, beforeTotal, afterCount, afterTotal);
        }
#endif
        return bufferView;
    }

    gpu::BufferView expandAttributeToMatchPositions(graphics::MeshPointer mesh, gpu::Stream::Slot slot) {
        if (slot == gpu::Stream::POSITION) {
            return buffer_helpers::getBufferView(mesh, slot);
        }
        return _expandedAttributeBuffer(mesh, slot);
    }
}

std::map<QString, gpu::BufferView> buffer_helpers::gatherBufferViews(graphics::MeshPointer mesh, const QStringList& expandToMatchPositions) {
    std::map<QString, gpu::BufferView> attributeViews;
    if (!mesh) {
        return attributeViews;
    }
    for (const auto& a : buffer_helpers::ATTRIBUTES.toStdMap()) {
        auto name = a.first;
        auto slot = a.second;
        auto view = getBufferView(mesh, slot);
        auto beforeCount = view.getNumElements();
#if DEV_BUILD
        auto beforeTotal = view._size;
#endif
        if (expandToMatchPositions.contains(name)) {
            expandAttributeToMatchPositions(mesh, slot);
        }
        if (beforeCount > 0) {
            auto element = view._element;
            QString typeName = QString("%1").arg(element.getType());

            attributeViews[name] = getBufferView(mesh, slot);

#if DEV_BUILD
            const auto vecN = element.getScalarCount();
            auto afterTotal = attributeViews[name]._size;
            auto afterCount = attributeViews[name].getNumElements();
            if (beforeTotal != afterTotal || beforeCount != afterCount) {
                qCDebug(bufferhelper_logging, "NOTE:: gatherBufferViews.%s vec%d %s (before count=%lu bytes=%lu // after count=%lu bytes=%lu)",
                        name.toStdString().c_str(), vecN, typeName.toStdString().c_str(), beforeCount, beforeTotal, afterCount, afterTotal);
            }
#endif
        }
    }
    return attributeViews;
}

bool buffer_helpers::recalculateNormals(graphics::MeshPointer mesh) {
    qCInfo(bufferhelper_logging) << "Recalculating normals" << !!mesh;
    if (!mesh) {
        return false;
    }
    buffer_helpers::gatherBufferViews(mesh, { "normal", "color" }); // ensures #normals >= #positions
    auto normals = mesh->getAttributeBuffer(gpu::Stream::NORMAL);
    auto verts = mesh->getVertexBuffer();
    auto indices = mesh->getIndexBuffer();
    auto esize = indices._element.getSize();
    auto numPoints = indices.getNumElements();
    const auto TRIANGLE = 3;
    quint32 numFaces = (quint32)numPoints / TRIANGLE;
    QVector<glm::vec3> faceNormals;
    QMap<QString,QVector<quint32>> vertexToFaces;
    faceNormals.resize(numFaces);
    auto numNormals = normals.getNumElements();
    qCInfo(bufferhelper_logging) << QString("numFaces: %1, numNormals: %2, numPoints: %3").arg(numFaces).arg(numNormals).arg(numPoints);
    if (normals.getNumElements() != verts.getNumElements()) {
        return false;
    }
    for (quint32 i = 0; i < numFaces; i++) {
        quint32 I = TRIANGLE * i;
        quint32 i0 = esize == 4 ? indices.get<quint32>(I+0) : indices.get<quint16>(I+0);
        quint32 i1 = esize == 4 ? indices.get<quint32>(I+1) : indices.get<quint16>(I+1);
        quint32 i2 = esize == 4 ? indices.get<quint32>(I+2) : indices.get<quint16>(I+2);

        Triangle face = {
            verts.get<glm::vec3>(i1),
            verts.get<glm::vec3>(i2),
            verts.get<glm::vec3>(i0)
        };
        faceNormals[i] = face.getNormal();
        if (glm::isnan(faceNormals[i].x)) {
#ifdef DEBUG_BUFFERVIEW_SCRIPTING
            qCInfo(bufferhelper_logging) << i << i0 << i1 << i2 << glmVecToVariant(face.v0) << glmVecToVariant(face.v1) << glmVecToVariant(face.v2);
#endif
            break;
        }
        vertexToFaces[glm::to_string(glm::dvec3(face.v0)).c_str()] << i;
        vertexToFaces[glm::to_string(glm::dvec3(face.v1)).c_str()] << i;
        vertexToFaces[glm::to_string(glm::dvec3(face.v2)).c_str()] << i;
    }
    for (quint32 j = 0; j < numNormals; j++) {
        //auto v = verts.get<glm::vec3>(j);
        glm::vec3 normal { 0.0f, 0.0f, 0.0f };
        QString key { glm::to_string(glm::dvec3(verts.get<glm::vec3>(j))).c_str() };
        const auto& faces = vertexToFaces.value(key);
        if (faces.size()) {
            for (const auto i : faces) {
                normal += faceNormals[i];
            }
            normal *= 1.0f / (float)faces.size();
        } else {
            static int logged = 0;
            if (logged++ < 10) {
                qCInfo(bufferhelper_logging) << "no faces for key!?" << key;
            }
            normal = verts.get<glm::vec3>(j);
        }
        if (glm::isnan(normal.x)) {
#ifdef DEBUG_BUFFERVIEW_SCRIPTING
            static int logged = 0;
            if (logged++ < 10) {
                qCInfo(bufferhelper_logging) << "isnan(normal.x)" << j << glmVecToVariant(normal);
            }
#endif
            break;
        }
        buffer_helpers::fromVariant(normals, j, glmVecToVariant(glm::normalize(normal)));
    }
    return true;
}

QVariant buffer_helpers::toVariant(const glm::mat4& mat4) {
    QVector<float> floats;
    floats.resize(16);
    memcpy(floats.data(), &mat4, sizeof(glm::mat4));
    QVariant v;
    v.setValue<QVector<float>>(floats);
    return v;
};

QVariant buffer_helpers::toVariant(const Extents& box) {
    return QVariantMap{
        { "center", glmVecToVariant(box.minimum + (box.size() / 2.0f)) },
        { "minimum", glmVecToVariant(box.minimum) },
        { "maximum", glmVecToVariant(box.maximum) },
        { "dimensions", glmVecToVariant(box.size()) },
    };
}

QVariant buffer_helpers::toVariant(const AABox& box) {
    return QVariantMap{
        { "brn", glmVecToVariant(box.getCorner()) },
        { "tfl", glmVecToVariant(box.calcTopFarLeft()) },
        { "center", glmVecToVariant(box.calcCenter()) },
        { "minimum", glmVecToVariant(box.getMinimumPoint()) },
        { "maximum", glmVecToVariant(box.getMaximumPoint()) },
        { "dimensions", glmVecToVariant(box.getDimensions()) },
    };
}
