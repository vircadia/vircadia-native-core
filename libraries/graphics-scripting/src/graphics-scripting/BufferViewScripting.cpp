#include "BufferViewScripting.h"

#include <QDebug>
#include <QVariant>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptEngine>

#include <gpu/Buffer.h>
#include <gpu/Format.h>
#include <gpu/Stream.h>

#include <graphics/BufferViewHelpers.h>

#ifdef DEBUG_BUFFERVIEW_SCRIPTING
    #include "DebugNames.h"
#endif

namespace {
    const std::array<const char*, 4> XYZW = {{ "x", "y", "z", "w" }};
    const std::array<const char*, 4> ZERO123 = {{ "0", "1", "2", "3" }};
}

template <typename T>
QScriptValue getBufferViewElement(QScriptEngine* js, const gpu::BufferView& view, quint32 index, bool asArray = false) {
    return glmVecToScriptValue(js, view.get<T>(index), asArray);
}

QScriptValue bufferViewElementToScriptValue(QScriptEngine* engine, const gpu::BufferView& view, quint32 index, bool asArray, const char* hint) {
    QVariant result = buffer_helpers::toVariant(view, index, asArray, hint);
    if (!result.isValid()) {
        return QScriptValue::NullValue;
    }
    return engine->toScriptValue(result);
}

template <typename T>
void setBufferViewElement(const gpu::BufferView& view, quint32 index, const QScriptValue& v) {
    view.edit<T>(index) = glmVecFromScriptValue<T>(v);
}

bool bufferViewElementFromScriptValue(const QScriptValue& v, const gpu::BufferView& view, quint32 index) {
    return buffer_helpers::fromVariant(view, index, v.toVariant());
}

template <typename T>
QScriptValue glmVecToScriptValue(QScriptEngine *js, const T& v, bool asArray) {
    static const auto len = T().length();
    const auto& components = asArray ? ZERO123 : XYZW;
    auto obj = asArray ? js->newArray() : js->newObject();
    for (int i = 0; i < len ; i++) {
        const auto key = components[i];
        const auto value = v[i];
        if (value != value) { // NAN
#ifdef DEV_BUILD
            qWarning().nospace()<< "vec" << len << "." << key << " converting NAN to javascript NaN.... " << value;
#endif
            obj.setProperty(key, js->globalObject().property("NaN"));
        } else {
            obj.setProperty(key, value);
        }
    }
    return obj;
}

template <typename T>
const T glmVecFromScriptValue(const QScriptValue& v) {
    static const auto len = T().length();
    const auto& components = v.property("x").isValid() ? XYZW : ZERO123;
    T result;
    for (int i = 0; i < len ; i++) {
        const auto key = components[i];
        const auto value = v.property(key).toNumber();
#ifdef DEV_BUILD
        if (value != value) { // NAN
            qWarning().nospace()<< "vec" << len << "." << key << " NAN received from script.... " << v.toVariant().toString();
        }
#endif
        result[i] = value;
    }
    return result;
}
