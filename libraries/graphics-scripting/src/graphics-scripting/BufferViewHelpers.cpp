#include "./graphics-scripting/BufferViewHelpers.h"

#include <QDebug>
#include <QVariant>

#include <gpu/Buffer.h>
#include <gpu/Format.h>
#include <gpu/Stream.h>

#include <glm/gtc/packing.hpp>
namespace glm {
    using hvec2 = glm::tvec2<glm::detail::hdata>;
    using hvec4 = glm::tvec4<glm::detail::hdata>;
}

//#define DEBUG_BUFFERVIEW_SCRIPTING
#ifdef DEBUG_BUFFERVIEW_SCRIPTING
    #include "DebugNames.h"
    QLoggingCategory bufferview_helpers{"hifi.bufferview"};
#endif

namespace {
    const std::array<const char*, 4> XYZW = {{ "x", "y", "z", "w" }};
    const std::array<const char*, 4> ZERO123 = {{ "0", "1", "2", "3" }};
}

template <typename T>
QVariant getBufferViewElement(const gpu::BufferView& view, quint32 index, bool asArray = false) {
    return glmVecToVariant(view.get<T>(index), asArray);
}

template <typename T>
void setBufferViewElement(const gpu::BufferView& view, quint32 index, const QVariant& v) {
    view.edit<T>(index) = glmVecFromVariant<T>(v);
}

//FIXME copied from Model.cpp
static void packNormalAndTangent(glm::vec3 normal, glm::vec3 tangent, glm::uint32& packedNormal, glm::uint32& packedTangent) {
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

bool bufferViewElementFromVariant(const gpu::BufferView& view, quint32 index, const QVariant& v) {
    const auto& element = view._element;
    const auto vecN = element.getScalarCount();
    const auto dataType = element.getType();
    const auto byteLength = element.getSize();
    const auto BYTES_PER_ELEMENT = byteLength / vecN;
#ifdef DEBUG_BUFFERVIEW_SCRIPTING
    qCDebug(bufferview_helpers) << "bufferViewElementFromVariant" << index << DebugNames::stringFrom(dataType) << BYTES_PER_ELEMENT << vecN;
#endif
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

QVariant bufferViewElementToVariant(const gpu::BufferView& view, quint32 index, bool asArray, const char* hint) {
    const auto& element = view._element;
    const auto vecN = element.getScalarCount();
    const auto dataType = element.getType();
    const auto byteLength = element.getSize();
    const auto BYTES_PER_ELEMENT = byteLength / vecN;
    Q_ASSERT(index < view.getNumElements());
    Q_ASSERT(index * vecN * BYTES_PER_ELEMENT < (view._size - vecN * BYTES_PER_ELEMENT));
#ifdef DEBUG_BUFFERVIEW_SCRIPTING
    qCDebug(bufferview_helpers) << "bufferViewElementToVariant" << index << DebugNames::stringFrom(dataType) << BYTES_PER_ELEMENT << vecN;
#endif
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
QVariant glmVecToVariant(const T& v, bool asArray /*= false*/) {
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
const T glmVecFromVariant(const QVariant& v) {
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
        if (value != value) { // NAN
            qWarning().nospace()<< "vec" << len << "." << components[i] << " NAN received from script.... " << v.toString();
        }
        result[i] = value;
    }
    return result;
}

template <typename T>
gpu::BufferView bufferViewFromVector(QVector<T> elements, gpu::Element elementType) {
    auto vertexBuffer = std::make_shared<gpu::Buffer>(elements.size() * sizeof(T), (gpu::Byte*)elements.data());
    return { vertexBuffer, 0, vertexBuffer->getSize(),sizeof(T), elementType };
}

template<> gpu::BufferView bufferViewFromVector<unsigned int>(QVector<unsigned int> elements, gpu::Element elementType) { return bufferViewFromVector(elements, elementType); }
template<> gpu::BufferView bufferViewFromVector<glm::vec3>(QVector<glm::vec3> elements, gpu::Element elementType) { return bufferViewFromVector(elements, elementType); }

gpu::BufferView cloneBufferView(const gpu::BufferView& input) {
    return gpu::BufferView(
        std::make_shared<gpu::Buffer>(input._buffer->getSize(), input._buffer->getData()),
        input._offset, input._size, input._stride, input._element
        );
}

gpu::BufferView resizedBufferView(const gpu::BufferView& input, quint32 numElements) {
    auto effectiveSize = input._buffer->getSize() / input.getNumElements();
    qDebug() << "resize input" << input.getNumElements() << input._buffer->getSize() << "effectiveSize" << effectiveSize;
    auto vsize = input._element.getSize() * numElements;
    gpu::Byte *data = new gpu::Byte[vsize];
    memset(data, 0, vsize);
    auto buffer = new gpu::Buffer(vsize, (gpu::Byte*)data);
    delete[] data;
    auto output = gpu::BufferView(buffer, input._element);
    qDebug() << "resized output" << output.getNumElements() << output._buffer->getSize();
    return output;
}
