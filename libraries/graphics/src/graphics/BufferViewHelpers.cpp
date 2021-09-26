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
#include "GpuHelpers.h"

#include <AABox.h>
#include <Extents.h>

namespace glm {
    using hvec2 = glm::tvec2<glm::detail::hdata>;
    using hvec4 = glm::tvec4<glm::detail::hdata>;
}

namespace {
    QLoggingCategory bufferhelper_logging{ "hifi.bufferview" };
}

namespace buffer_helpers {

const std::array<const char*, 4> XYZW = { { "x", "y", "z", "w" } };
const std::array<const char*, 4> ZERO123 = { { "0", "1", "2", "3" } };

/*@jsdoc
 * <p>The type name of a graphics buffer.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"position"</code></td><td>Position buffer.</td></tr>
 *     <tr><td><code>"normal"</code></td><td>normal buffer.</td></tr>
 *     <tr><td><code>"tangent"</code></td><td>Tangent buffer.</td></tr>
 *     <tr><td><code>"color"</code></td><td>Color buffer.</td></tr>
 *     <tr><td><code>"skin_cluster_index"</code></td><td>Skin cluster index buffer.</td></tr>
 *     <tr><td><code>"skin_cluster_weight"</code></td><td>Skin cluster weight buffer.</td></tr>
 *     <tr><td><code>"texcoord0"</code></td><td>First UV coordinates buffer.</td></tr>
 *     <tr><td><code>"texcoord1"</code></td><td>Second UV coordinates buffer.</td></tr>
 *     <tr><td><code>"texcoord2"</code></td><td>Third UV coordinates buffer.</td></tr>
 *     <tr><td><code>"texcoord3"</code></td><td>Fourth UV coordinates buffer.</td></tr>
 *     <tr><td><code>"texcoord4"</code></td><td>Fifth UV coordinates buffer.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Graphics.BufferTypeName
 */
/*@jsdoc
 * <p>The type of a graphics buffer value as accessed by JavaScript.</p>
 * <table>
 *   <thead>
 *     <tr><th>Type</th><th>Name</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td>{@link Vec3}</td><td><code>"position"</code></td><td>Position buffer.</td></tr>
 *     <tr><td>{@link Vec3}</td><td><code>"normal"</code></td><td>normal buffer.</td></tr>
 *     <tr><td>{@link Vec3}</td><td><code>"tangent"</code></td><td>Tangent buffer.</td></tr>
 *     <tr><td>{@link Vec4}</td><td><code>"color"</code></td><td>Color buffer.</td></tr>
 *     <tr><td>{@link Vec4}</td><td><code>"skin_cluster_index"</code></td><td>Skin cluster index buffer.</td></tr>
 *     <tr><td>{@link Vec4}</td><td><code>"skin_cluster_weight"</code></td><td>Skin cluster weight buffer.</td></tr>
 *     <tr><td>{@link Vec2}</td><td><code>"texcoord0"</code></td><td>First UV coordinates buffer.</td></tr>
 *     <tr><td>{@link Vec2}</td><td><code>"texcoord1"</code></td><td>Second UV coordinates buffer.</td></tr>
 *     <tr><td>{@link Vec2}</td><td><code>"texcoord2"</code></td><td>Third UV coordinates buffer.</td></tr>
 *     <tr><td>{@link Vec2}</td><td><code>"texcoord3"</code></td><td>Fourth UV coordinates buffer.</td></tr>
 *     <tr><td>{@link Vec2}</td><td><code>"texcoord4"</code></td><td>Fifth UV coordinates buffer.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {Vec4|Vec3|Vec2} Graphics.BufferType
 */
QMap<QString,int> ATTRIBUTES{
    {"position", gpu::Stream::POSITION },
    {"normal", gpu::Stream::NORMAL },
    {"color", gpu::Stream::COLOR },
    {"tangent", gpu::Stream::TANGENT },
    {"skin_cluster_index", gpu::Stream::SKIN_CLUSTER_INDEX },
    {"skin_cluster_weight", gpu::Stream::SKIN_CLUSTER_WEIGHT },
    {"texcoord0", gpu::Stream::TEXCOORD0 },
    {"texcoord1", gpu::Stream::TEXCOORD1 },
    {"texcoord2", gpu::Stream::TEXCOORD2 },
    {"texcoord3", gpu::Stream::TEXCOORD3 },
    {"texcoord4", gpu::Stream::TEXCOORD4 },
};


namespace {
    bool boundsCheck(const gpu::BufferView& view, glm::uint32 index) {
        const auto byteLength = view._element.getSize();
        return (
            index < view.getNumElements() &&
            index * byteLength < (view._size - 1) * byteLength
        );
    }
}

template <typename T>
glm::uint32 forEachGlmVec(const gpu::BufferView& view, std::function<bool(glm::uint32 index, const T& value)> func) {
    QVector<glm::uint32> result;
    const glm::uint32 num = (glm::uint32)view.getNumElements();
    glm::uint32 i = 0;
    for (; i < num; i++) {
        if (!func(i, view.get<T>(i))) {
            break;
        }
    }
    return i;
}

template<> glm::uint32 forEach<glm::vec3>(const gpu::BufferView& view, std::function<bool(glm::uint32 index, const glm::vec3& value)> func) {
    return forEachGlmVec<glm::vec3>(view, func);
}

// QVector<T> => BufferView
template <typename T>
gpu::BufferView newFromVector(const QVector<T>& elements, const gpu::Element& elementType) {
    auto vertexBuffer = std::make_shared<gpu::Buffer>(elements.size() * sizeof(T), (gpu::Byte*)elements.data());
    return { vertexBuffer, 0, vertexBuffer->getSize(),sizeof(T), elementType };
}

template <typename T>
gpu::BufferView bufferViewFromVector(const QVector<T>& elements, const gpu::Element& elementType) {
    auto vertexBuffer = std::make_shared<gpu::Buffer>(elements.size() * sizeof(T), (gpu::Byte*)elements.data());
    return { vertexBuffer, 0, vertexBuffer->getSize(),sizeof(T), elementType };
}
template<> gpu::BufferView newFromVector<unsigned int>(const QVector<unsigned int>& elements, const gpu::Element& elementType) { return bufferViewFromVector(elements, elementType); }
template<> gpu::BufferView newFromVector<glm::vec2>(const QVector<glm::vec2>& elements, const gpu::Element& elementType) { return bufferViewFromVector(elements, elementType); }
template<> gpu::BufferView newFromVector<glm::vec3>( const QVector<glm::vec3>& elements, const gpu::Element& elementType) { return bufferViewFromVector(elements, elementType); }
template<> gpu::BufferView newFromVector<glm::vec4>(const QVector<glm::vec4>& elements, const gpu::Element& elementType) { return bufferViewFromVector(elements, elementType); }
template<> gpu::BufferView newFromVector<graphics::Mesh::Part>(const QVector<graphics::Mesh::Part>& elements, const gpu::Element& elementType) { return bufferViewFromVector(elements, elementType); }

struct GpuToGlmAdapter {
    static float error(const QString& name, const gpu::BufferView& view, glm::uint32 index, const char *hint) {
        qDebug() << QString("GpuToGlmAdapter:: unhandled type=%1(element=%2) size=%3(location=%4,per=%5) vec%6 hint=%7 #%8 %9 %10")
            .arg(name)
            .arg(gpu::toString(view._element.getType()))
            .arg(view._element.getSize())
            .arg(view._element.getLocationSize())
            .arg(view._element.getSize() / view._element.getScalarCount())
            .arg(view._element.getScalarCount())
            .arg(hint)
            .arg(view.getNumElements())
            .arg(gpu::toString(view._element.getSemantic()))
            .arg(gpu::toString(view._element.getDimension()));
        Q_ASSERT(false);
        assert(false);
        return NAN;
    }
};

#define CHECK_SIZE(T) if (view._element.getSize() != sizeof(T)) { qDebug() << "invalid elementSize" << hint << view._element.getSize() << "expected:" << sizeof(T); break; }

template <typename T> struct GpuScalarToGlm : GpuToGlmAdapter {
    static T get(const gpu::BufferView& view, glm::uint32 index, const char *hint) {
#ifdef DEBUG_BUFFERVIEW_HELPERS
    if(!boundsCheck(view, index))return T(error("GpuScalarToGlm::get::out of bounds", view, index, hint));
#endif
    switch(view._element.getType()) {
        case gpu::UINT32: return view.get<glm::uint32>(index);
        case gpu::UINT16: return view.get<glm::uint16>(index);
        case gpu::UINT8: return view.get<glm::uint8>(index);
        case gpu::INT32: return view.get<glm::int32>(index);
        case gpu::INT16: return view.get<glm::int16>(index);
        case gpu::INT8: return view.get<glm::int8>(index);
        case gpu::FLOAT: return view.get<glm::float32>(index);
        case gpu::HALF: return T(glm::unpackHalf1x16(view.get<glm::uint16>(index)));
        case gpu::NUINT8: return T(glm::unpackUnorm1x8(view.get<glm::uint8>(index)));
        default: break;
        } return T(error("GpuScalarToGlm::get", view, index, hint));
    }
    static bool set(const gpu::BufferView& view, glm::uint32 index, const T& value, const char *hint) {
#ifdef DEBUG_BUFFERVIEW_HELPERS
    if(!boundsCheck(view, index))return T(error("GpuScalarToGlm::set::out of bounds", view, index, hint));
#endif
    switch(view._element.getType()) {
    case gpu::UINT32: view.edit<glm::uint32>(index) = value; return true;
    case gpu::UINT16: view.edit<glm::uint16>(index) = value; return true;
    case gpu::UINT8: view.edit<glm::uint8>(index) = value; return true;
    case gpu::INT32: view.edit<glm::int32>(index) = value; return true;
    case gpu::INT16: view.edit<glm::int16>(index) = value; return true;
    case gpu::INT8: view.edit<glm::int8>(index) = value; return true;
    case gpu::FLOAT: view.edit<glm::float32>(index) = value; return true;
    case gpu::HALF:  view.edit<glm::uint16>(index) = glm::packHalf1x16(value); return true;
    case gpu::NUINT8: view.edit<glm::uint8>(index) = glm::packUnorm1x8(value); return true;
    default: break;
    } error("GpuScalarToGlm::set", view, index, hint); return false;
    }
};

template <typename T> struct GpuVec2ToGlm : GpuToGlmAdapter { static T get(const gpu::BufferView& view, glm::uint32 index, const char *hint) {
#ifdef DEBUG_BUFFERVIEW_HELPERS
    if(!boundsCheck(view, index))return T(error("GpuVec2ToGlm::get::out of bounds", view, index, hint));
#endif
    switch(view._element.getType()) {
    case gpu::UINT32: return view.get<glm::u32vec2>(index);
    case gpu::UINT16: return view.get<glm::u16vec2>(index);
    case gpu::UINT8: return view.get<glm::u8vec2>(index);
    case gpu::INT32: return view.get<glm::i32vec2>(index);
    case gpu::INT16: return view.get<glm::i16vec2>(index);
    case gpu::INT8: return view.get<glm::i8vec2>(index);
    case gpu::FLOAT: return view.get<glm::fvec2>(index);
    case gpu::HALF: CHECK_SIZE(glm::uint32); return glm::unpackHalf2x16(view.get<glm::uint32>(index));
    case gpu::NUINT16: CHECK_SIZE(glm::uint32); return glm::unpackUnorm2x16(view.get<glm::uint32>(index));
    case gpu::NUINT8: CHECK_SIZE(glm::uint16); return glm::unpackUnorm2x8(view.get<glm::uint16>(index));
    default: break;
    } return T(error("GpuVec2ToGlm::get", view, index, hint)); }

    static bool set(const gpu::BufferView& view, glm::uint32 index, const T& value, const char *hint) {
#ifdef DEBUG_BUFFERVIEW_HELPERS
    if(!boundsCheck(view, index))return T(error("GpuVec2ToGlm::set::out of bounds", view, index, hint));
#endif
    switch(view._element.getType()) {
        // TODO: flush out GpuVec2ToGlm<T>::set(value)
    case gpu::FLOAT: view.edit<glm::fvec2>(index) = value; return true;
    case gpu::HALF:  view.edit<glm::uint32>(index) = glm::packHalf2x16(value); return true;
    case gpu::NUINT16: view.edit<glm::uint32>(index) = glm::packUnorm2x16(value); return true;
    case gpu::NUINT8: view.edit<glm::uint16>(index) = glm::packUnorm2x8(value); return true;
    default: break;
    } error("GpuVec2ToGlm::set", view, index, hint); return false;
    }
};

template <typename T> struct GpuVec4ToGlm;

template <typename T> struct GpuVec3ToGlm  : GpuToGlmAdapter  { static T get(const gpu::BufferView& view, glm::uint32 index, const char *hint) {
#ifdef DEBUG_BUFFERVIEW_HELPERS
    if(!boundsCheck(view, index))return T(error("GpuVec3ToGlm::get::out of bounds", view, index, hint));
#endif
    switch(view._element.getType()) {
    case gpu::UINT32: return view.get<glm::u32vec3>(index);
    case gpu::UINT16: return view.get<glm::u16vec3>(index);
    case gpu::UINT8: return view.get<glm::u8vec3>(index);
    case gpu::INT32: return view.get<glm::i32vec3>(index);
    case gpu::INT16: return view.get<glm::i16vec3>(index);
    case gpu::INT8: return view.get<glm::i8vec3>(index);
    case gpu::FLOAT: return view.get<glm::fvec3>(index);
    case gpu::HALF: CHECK_SIZE(glm::uint64); return T(glm::unpackHalf4x16(view.get<glm::uint64>(index)));
    case gpu::NUINT8: CHECK_SIZE(glm::uint32); return T(glm::unpackUnorm4x8(view.get<glm::uint32>(index)));
    case gpu::NINT2_10_10_10: return T(glm::unpackSnorm3x10_1x2(view.get<glm::uint32>(index)));
    default: break;
    } return T(error("GpuVec3ToGlm::get", view, index, hint)); }
    static bool set(const gpu::BufferView& view, glm::uint32 index, const T& value, const char *hint) {
#ifdef DEBUG_BUFFERVIEW_HELPERS
    if(!boundsCheck(view, index))return T(error("GpuVec3ToGlm::set::out of bounds", view, index, hint));
#endif
    switch(view._element.getType()) {
        // TODO: flush out GpuVec3ToGlm<T>::set(value)
    case gpu::FLOAT: view.edit<glm::fvec3>(index) = value; return true;
    case gpu::NUINT8: CHECK_SIZE(glm::uint32); view.edit<glm::uint32>(index) = glm::packUnorm4x8(glm::fvec4(value,0.0f)); return true;
    case gpu::UINT8: view.edit<glm::u8vec3>(index) = value; return true;
    case gpu::NINT2_10_10_10: view.edit<glm::uint32>(index) = glm_packSnorm3x10_1x2(glm::fvec4(value,0.0f)); return true;
    default: break;
    } error("GpuVec3ToGlm::set", view, index, hint); return false;
    }
};

template <typename T> struct GpuVec4ToGlm : GpuToGlmAdapter { static T get(const gpu::BufferView& view, glm::uint32 index, const char *hint) {
#ifdef DEBUG_BUFFERVIEW_HELPERS
    if(!boundsCheck(view, index))return T(error("GpuVec4ToGlm::get::out of bounds", view, index, hint));
#endif
    switch(view._element.getType()) {
    case gpu::UINT32: return view.get<glm::u32vec4>(index);
    case gpu::UINT16: return view.get<glm::u16vec4>(index);
    case gpu::UINT8: return view.get<glm::u8vec4>(index);
    case gpu::INT32: return view.get<glm::i32vec4>(index);
    case gpu::INT16: return view.get<glm::i16vec4>(index);
    case gpu::INT8: return view.get<glm::i8vec4>(index);
    case gpu::NUINT32: break;
    case gpu::NUINT16: CHECK_SIZE(glm::uint64); return glm::unpackUnorm4x16(view.get<glm::uint64>(index));
    case gpu::NUINT8: CHECK_SIZE(glm::uint32); return glm::unpackUnorm4x8(view.get<glm::uint32>(index));
    case gpu::NUINT2: break;
    case gpu::NINT32: break;
    case gpu::NINT16: break;
    case gpu::NINT8: break;
    case gpu::COMPRESSED: break;
    case gpu::NUM_TYPES: break;
    case gpu::FLOAT: return view.get<glm::fvec4>(index);
    case gpu::HALF: CHECK_SIZE(glm::uint64); return glm::unpackHalf4x16(view.get<glm::uint64>(index));
    case gpu::NINT2_10_10_10: return glm::unpackSnorm3x10_1x2(view.get<glm::uint32>(index));
    } return T(error("GpuVec4ToGlm::get", view, index, hint)); }
    static bool set(const gpu::BufferView& view, glm::uint32 index, const T& value, const char *hint) {
#ifdef DEBUG_BUFFERVIEW_HELPERS
    if(!boundsCheck(view, index))return T(error("GpuVec4ToGlm::set::out of bounds", view, index, hint));
#endif
    switch(view._element.getType()) {
    case gpu::FLOAT: view.edit<glm::fvec4>(index) = value; return true;
    case gpu::HALF: CHECK_SIZE(glm::uint64); view.edit<glm::uint64_t>(index) = glm::packHalf4x16(value); return true;
    case gpu::UINT8: view.edit<glm::u8vec4>(index) = value; return true;
    case gpu::NINT2_10_10_10: view.edit<glm::uint32>(index) = glm_packSnorm3x10_1x2(value); return true;
    case gpu::NUINT16: CHECK_SIZE(glm::uint64); view.edit<glm::uint64>(index) = glm::packUnorm4x16(value); return true;
    case gpu::NUINT8: CHECK_SIZE(glm::uint32);  view.edit<glm::uint32>(index) = glm::packUnorm4x8(value); return true;
    default: break;
    } error("GpuVec4ToGlm::set", view, index, hint); return false;
    }
};
#undef CHECK_SIZE

template <typename FUNC, typename T>
struct GpuValueResolver {
    static QVector<T> toVector(const gpu::BufferView& view, const char *hint) {
        QVector<T> result;
        const glm::uint32 count = (glm::uint32)view.getNumElements();
        result.resize(count);
        for (glm::uint32 i = 0; i < count; i++) {
            result[i] = FUNC::get(view, i, hint);
        }
        return result;
    }
    static T toValue(const gpu::BufferView& view, glm::uint32 index, const char *hint) {
        return FUNC::get(view, index, hint);
    }
};

// BufferView => QVector<T>
template <typename U> QVector<U> bufferToVector(const gpu::BufferView& view, const char *hint) { return GpuValueResolver<GpuScalarToGlm<U>,U>::toVector(view, hint); }

template<> QVector<int> bufferToVector<int>(const gpu::BufferView& view, const char *hint) { return GpuValueResolver<GpuScalarToGlm<int>,int>::toVector(view, hint); }
template<> QVector<glm::uint16> bufferToVector<glm::uint16>(const gpu::BufferView& view, const char *hint) { return GpuValueResolver<GpuScalarToGlm<glm::uint16>,glm::uint16>::toVector(view, hint); }
template<> QVector<glm::uint32> bufferToVector<glm::uint32>(const gpu::BufferView& view, const char *hint) { return GpuValueResolver<GpuScalarToGlm<glm::uint32>,glm::uint32>::toVector(view, hint); }
template<> QVector<glm::vec2> bufferToVector<glm::vec2>(const gpu::BufferView& view, const char *hint) { return GpuValueResolver<GpuVec2ToGlm<glm::vec2>,glm::vec2>::toVector(view, hint); }
template<> QVector<glm::vec3> bufferToVector<glm::vec3>(const gpu::BufferView& view, const char *hint) { return GpuValueResolver<GpuVec3ToGlm<glm::vec3>,glm::vec3>::toVector(view, hint); }
template<> QVector<glm::vec4> bufferToVector<glm::vec4>(const gpu::BufferView& view, const char *hint) { return GpuValueResolver<GpuVec4ToGlm<glm::vec4>,glm::vec4>::toVector(view, hint); }

// view.get<T> with conversion between types
template<> int getValue<int>(const gpu::BufferView& view, glm::uint32 index, const char *hint) { return GpuScalarToGlm<int>::get(view, index, hint); }
template<> glm::uint32 getValue<glm::uint32>(const gpu::BufferView& view, glm::uint32 index, const char *hint) { return GpuScalarToGlm<glm::uint32>::get(view, index, hint); }
template<> glm::vec2 getValue<glm::vec2>(const gpu::BufferView& view, glm::uint32 index, const char *hint) { return GpuVec2ToGlm<glm::vec2>::get(view, index, hint); }
template<> glm::vec3 getValue<glm::vec3>(const gpu::BufferView& view, glm::uint32 index, const char *hint) { return GpuVec3ToGlm<glm::vec3>::get(view, index, hint); }
template<> glm::vec4 getValue<glm::vec4>(const gpu::BufferView& view, glm::uint32 index, const char *hint) { return GpuVec4ToGlm<glm::vec4>::get(view, index, hint); }

// bufferView => QVariant
template<> QVariant getValue<QVariant>(const gpu::BufferView& view, glm::uint32 index, const char* hint) {
    if (!boundsCheck(view, index)) {
        qDebug() << "getValue<QVariant> -- out of bounds" << index << hint;
        return false;
    }
    const auto dataType = view._element.getType();
    switch(view._element.getScalarCount()) {
    case 1:
        if (dataType == gpu::Type::FLOAT) {
            return GpuScalarToGlm<glm::float32>::get(view, index, hint);
        } else {
            switch(dataType) {
            case gpu::INT8: case gpu::INT16: case gpu::INT32:
            case gpu::NINT8: case gpu::NINT16: case gpu::NINT32:
            case gpu::NINT2_10_10_10:
                // signed
                return GpuScalarToGlm<glm::int32>::get(view, index, hint);
            default:
                // unsigned
                return GpuScalarToGlm<glm::uint32>::get(view, index, hint);
            }
        }
    case 2: return glmVecToVariant(GpuVec2ToGlm<glm::vec2>::get(view, index, hint));
    case 3: return glmVecToVariant(GpuVec3ToGlm<glm::vec3>::get(view, index, hint));
    case 4: return glmVecToVariant(GpuVec4ToGlm<glm::vec4>::get(view, index, hint));
    }
    return QVariant();
}

// view.edit<T> with conversion between types
template<> bool setValue<QVariant>(const gpu::BufferView& view, glm::uint32 index, const QVariant& v, const char* hint) {
    if (!boundsCheck(view, index)) {
        qDebug() << "setValue<QVariant> -- out of bounds" << index << hint;
        return false;
    }
    const auto dataType = view._element.getType();

    switch(view._element.getScalarCount()) {
    case 1:
        if (dataType == gpu::Type::FLOAT) {
            return GpuScalarToGlm<glm::float32>::set(view, index, v.toFloat(), hint);
        } else {
            switch(dataType) {
            case gpu::INT8: case gpu::INT16: case gpu::INT32:
            case gpu::NINT8: case gpu::NINT16: case gpu::NINT32:
            case gpu::NINT2_10_10_10:
                // signed
                return GpuScalarToGlm<glm::int32>::set(view, index, v.toInt(), hint);
            default:
                // unsigned
                return GpuScalarToGlm<glm::uint32>::set(view, index, v.toUInt(), hint);
            }
        }
        return false;
    case 2: return GpuVec2ToGlm<glm::vec2>::set(view, index, glmVecFromVariant<glm::vec2>(v), hint);
    case 3: return GpuVec3ToGlm<glm::vec3>::set(view, index, glmVecFromVariant<glm::vec3>(v), hint);
    case 4: return GpuVec4ToGlm<glm::vec4>::set(view, index, glmVecFromVariant<glm::vec4>(v), hint);
    }
    return false;
}

template<> bool setValue<glm::uint32>(const gpu::BufferView& view, glm::uint32 index, const glm::uint32& value, const char* hint) {
    return GpuScalarToGlm<glm::uint32>::set(view, index, value, hint);
}
template<> bool setValue<glm::uint16>(const gpu::BufferView& view, glm::uint32 index, const glm::uint16& value, const char* hint) {
    return GpuScalarToGlm<glm::uint16>::set(view, index, value, hint);
}
template<> bool setValue<glm::vec2>(const gpu::BufferView& view, glm::uint32 index, const glm::vec2& value, const char* hint) {
    return GpuVec2ToGlm<glm::vec2>::set(view, index, value, hint);
}
template<> bool setValue<glm::vec3>(const gpu::BufferView& view, glm::uint32 index, const glm::vec3& value, const char* hint) {
    return GpuVec3ToGlm<glm::vec3>::set(view, index, value, hint);
}
template<> bool setValue<glm::vec4>(const gpu::BufferView& view, glm::uint32 index, const glm::vec4& value, const char* hint) {
    return GpuVec4ToGlm<glm::vec4>::set(view, index, value, hint);
}

// QVariantList => QVector<T>
template <class T> QVector<T> qVariantListToGlmVector(const QVariantList& list) {
    QVector<T> output;
    output.resize(list.size());
    int i = 0;
    for (const auto& v : list) {
        output[i++] = glmVecFromVariant<T>(v);
    }
    return output;
}
template <typename T> QVector<T> qVariantListToScalarVector(const QVariantList& list) {
    QVector<T> output;
    output.resize(list.size());
    int i = 0;
    for (const auto& v : list) {
        output[i++] = v.value<T>();
    }
    return output;
}

template <class T> QVector<T> variantToVector(const QVariant& value) { qDebug() << "variantToVector[class]"; return qVariantListToGlmVector<T>(value.toList()); }
template<> QVector<glm::float32> variantToVector<glm::float32>(const QVariant& value) { return qVariantListToScalarVector<glm::float32>(value.toList()); }
template<> QVector<glm::uint32> variantToVector<glm::uint32>(const QVariant& value) { return qVariantListToScalarVector<glm::uint32>(value.toList()); }
template<> QVector<glm::int32> variantToVector<glm::int32>(const QVariant& value) { return qVariantListToScalarVector<glm::int32>(value.toList()); }
template<> QVector<glm::vec2> variantToVector<glm::vec2>(const QVariant& value) { return qVariantListToGlmVector<glm::vec2>(value.toList()); }
template<> QVector<glm::vec3> variantToVector<glm::vec3>(const QVariant& value) { return qVariantListToGlmVector<glm::vec3>(value.toList()); }
template<> QVector<glm::vec4> variantToVector<glm::vec4>(const QVariant& value) { return qVariantListToGlmVector<glm::vec4>(value.toList()); }

template<> gpu::BufferView newFromVector<QVariant>(const QVector<QVariant>& _elements, const gpu::Element& elementType) {
    glm::uint32 numElements = _elements.size();
    auto buffer = new gpu::Buffer();
    buffer->resize(elementType.getSize() * numElements);
    auto bufferView = gpu::BufferView(buffer, elementType);
    for (glm::uint32 i = 0; i < numElements; i++) {
        setValue<QVariant>(bufferView, i, _elements[i]);
    }
    return bufferView;
}


gpu::BufferView clone(const gpu::BufferView& input) {
    return gpu::BufferView(
        std::make_shared<gpu::Buffer>(input._buffer->getSize(), input._buffer->getData()),
        input._offset, input._size, input._stride, input._element
    );
}

gpu::BufferView resized(const gpu::BufferView& input, glm::uint32 numElements) {
#ifdef DEBUG_BUFFERVIEW_HELPERS
    auto effectiveSize = input._buffer->getSize() / input.getNumElements();
    qCDebug(bufferhelper_logging) << "resize input" << input.getNumElements() << input._buffer->getSize() << "effectiveSize" << effectiveSize;
#endif
    auto vsize = input._element.getSize() * numElements;
    std::unique_ptr<gpu::Byte[]> data{ new gpu::Byte[vsize] };
    memset(data.get(), 0, vsize);
    auto buffer = new gpu::Buffer(vsize, data.get());
    memcpy(data.get(), input._buffer->getData(), std::min(vsize, (glm::uint32)input._buffer->getSize()));
    auto output = gpu::BufferView(buffer, input._element);
#ifdef DEBUG_BUFFERVIEW_HELPERS
    qCDebug(bufferhelper_logging) << "resized output" << output.getNumElements() << output._buffer->getSize();
#endif
    return output;
}

// mesh helpers
namespace mesh {
    gpu::BufferView getBufferView(const graphics::MeshPointer& mesh, gpu::Stream::Slot slot) {
        return slot == gpu::Stream::POSITION ? mesh->getVertexBuffer() : mesh->getAttributeBuffer(slot);
    }

    glm::uint32 forEachVertex(const graphics::MeshPointer& mesh, std::function<bool(glm::uint32 index, const QVariantMap& values)> func) {
        glm::uint32 i = 0;
        auto attributeViews = getAllBufferViews(mesh);
        auto nPositions = mesh->getNumVertices();
        for (; i < nPositions; i++) {
            QVariantMap values;
            for (const auto& a : attributeViews) {
                values[a.first] = getValue<QVariant>(a.second, i, qUtf8Printable(a.first));
            }
            if (!func(i, values)) {
                break;
            }
        }
        return i;
    }
    bool setVertexAttributes(const graphics::MeshPointer& mesh, glm::uint32 index, const QVariantMap& attributes) {
        bool ok = true;
        for (auto& a : getAllBufferViews(mesh)) {
            const auto& name = a.first;
            if (attributes.contains(name)) {
                const auto& value = attributes.value(name);
                if (value.isValid()) {
                    auto& view = a.second;
                    setValue<QVariant>(view, index, value);
                } else {
                    ok = false;
                    //qCDebug(graphics_scripting) << "(skipping) setVertexAttributes" << vertexIndex << name;
                }
            }
        }
        return ok;
    }

    QVariant getVertexAttributes(const graphics::MeshPointer& mesh, glm::uint32 vertexIndex) {
        auto attributeViews = getAllBufferViews(mesh);
        QVariantMap values;
        for (const auto& a : attributeViews) {
            values[a.first] = getValue<QVariant>(a.second, vertexIndex, qUtf8Printable(a.first));
        }
        return values;
    }

    graphics::MeshPointer clone(const graphics::MeshPointer& mesh) {
        auto clonedMesh = std::make_shared<graphics::Mesh>();
        clonedMesh->displayName = (QString::fromStdString(mesh->displayName) + "-clone").toStdString();
        clonedMesh->setIndexBuffer(buffer_helpers::clone(mesh->getIndexBuffer()));
        clonedMesh->setPartBuffer(buffer_helpers::clone(mesh->getPartBuffer()));
        auto attributeViews = getAllBufferViews(mesh);
        for (const auto& a : attributeViews) {
            auto& view = a.second;
            auto slot = ATTRIBUTES[a.first];
            auto points = buffer_helpers::clone(view);
            if (slot == gpu::Stream::POSITION) {
                clonedMesh->setVertexBuffer(points);
            } else {
                clonedMesh->addAttribute(slot, points);
            }
        }
        return clonedMesh;
    }

    std::map<QString, gpu::BufferView> getAllBufferViews(const graphics::MeshPointer& mesh) {
        std::map<QString, gpu::BufferView> attributeViews;
        if (!mesh) {
            return attributeViews;
        }
        for (const auto& a : ATTRIBUTES.toStdMap()) {
            auto bufferView = getBufferView(mesh, a.second);
            if (bufferView.getNumElements()) {
                attributeViews[a.first] = bufferView;
            }
        }
        return attributeViews;
    }
} // mesh
} // buffer_helpers
